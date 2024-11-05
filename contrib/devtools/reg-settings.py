import collections
import enum
import os
import re
import subprocess
from dataclasses import dataclass, field

@dataclass
class Call:
    file: str
    position: int
    call_text: str
    obj_name: str
    arg_name: str
    context: str
    namespace: str

class DataType(int, enum.Enum):
    STRING_LIST = 1
    STRING = 2
    PATH = 3
    INT = 4
    BOOL = 5
    DISABLED = 6
    UNSET = 7

# What default value is returned by a particular GetArg call when the setting is
# unset. Either a string containing a c++ expression, or None to indicate that
# call returns std::nullopt, or True to indicate it returns whatever the
# default-constructed value for the setting type is (e.g. the empty vector, or
# string, or path) or False to indicate it doesn't specify any default.
DefaultValue = str | bool | None

@dataclass
class SettingType:
    name: str
    primary: bool = False
    defaults: set[str | None] = field(default_factory=set)
    default_value: DefaultValue = False

@dataclass
class AddArg:
    call: Call
    summary: str
    help_text: str
    help_args: tuple[str, ...]
    flags: str
    category: str
    include_path: str | None = None
    data_types: dict[DataType, SettingType] = field(default_factory=dict)
    extern_args: list[str] = field(default_factory=list)

@dataclass
class GetArg:
    call: Call
    function_name: str
    data_type: DataType
    default_value: DefaultValue
    nots: int
    add: AddArg | None = None

@dataclass
class Setting:
    arg_name: str
    adds: list[AddArg] = field(default_factory=list)
    gets: list[GetArg] = field(default_factory=list)

def get_files_with_args(src_dir):
    # Run git grep to find files containing AddArg/GetArg/GetIntArg/GetBoolArg/GetArgs
    result = subprocess.run(
        [
            "git", "grep", "-l", "AddArg(\|GetArg(\|GetPathArg(\|GetIntArg(\|GetBoolArg(\|GetArgs(\|IsArgSet(\|IsArgNegated(", "--", src_dir
        ],
        capture_output=True,
        text=True
    )
    return result.stdout.splitlines()

def parse_function_args(arg_str):
    args = []
    parens = 0
    quot = False
    for pos, c in enumerate(arg_str):
        if c == '"':
            quot = not quot
        if quot:
            pass
        elif c == "(":
            parens += 1
        elif c == ")":
            if parens == 0: break
            parens -= 1
        elif c == "," and parens == 0:
            args.append("")
            continue
        if not args: args.append("")
        args[-1] += c
    return pos, args

def parse_calls(file_path):
    adds = []
    gets = []
    context = get_file_context(file_path)
    namespace = get_file_namespace(file_path)
    with open(file_path, 'r') as f:
        content = f.read()
        for match in re.finditer(r'\b(\w+)\.AddArg\((")', content):
            call_len, (summary, help_text, flags, category) = parse_function_args(content[match.start(2):])
            call = Call(
                file=file_path,
                position=match.start(),
                call_text=content[match.start():match.start(2)+call_len+1],
                obj_name=match.group(1),
                arg_name=re.match(r'"([^"=(]+).*', summary).group(1),
                context=context,
                namespace=namespace,
            )
            help_text=help_text.strip()
            help_args = []
            if m := re.match(r"strprintf\(", help_text):
                _, help_args = parse_function_args(help_text[m.end():])
                help_text = help_args[0].strip()
                help_args = [a.strip() for a in help_args[1:]]
            adds.append(AddArg(
                call=call,
                summary=summary.strip(),
                help_text=help_text,
                help_args=tuple(help_args),
                flags=flags.strip(),
                category=category.strip(),
            ))
        for match in re.finditer(r'(!*)\b((?:\w|\.|->)+)(\.|->)(GetArg|GetPathArg|GetIntArg|GetBoolArg|GetArgs|IsArgSet|IsArgNegated)\((.)', content):
            call_len, call_args = parse_function_args(content[match.start(5):])
            obj_name = match.group(2)
            if match.group(3) == "->":
                obj_name = f"*{obj_name}"
            call = Call(
                file=file_path,
                position=match.start(),
                call_text=content[match.start():match.start(5)+call_len+1],
                obj_name=obj_name,
                arg_name=call_args[0].strip().strip('"'),
                context=context,
                namespace=namespace,
            )
            function_name = match.group(4)
            data_type = (DataType.STRING_LIST if function_name == "GetArgs" else
                         DataType.STRING if function_name == "GetArg" else
                         DataType.PATH if function_name == "GetPathArg" else
                         DataType.INT if function_name == "GetIntArg" else
                         DataType.BOOL if function_name == "GetBoolArg" else
                         DataType.DISABLED if function_name == "IsArgNegated" else
                         DataType.UNSET if function_name == "IsArgSet" else
                         None)
            default_arg = call_args[1].strip() if len(call_args) > 1 else None
            default_value = (
                True if data_type == DataType.STRING and default_arg == '""' else
                True if data_type == DataType.INT and default_arg == "0" else
                True if data_type == DataType.BOOL and default_arg == "false" else
                default_arg if default_arg is not None else
                None if data_type in (DataType.STRING, DataType.INT, DataType.BOOL) else
                True if data_type in (DataType.PATH, DataType.STRING_LIST) else
                False)
            gets.append(GetArg(call=call, function_name=function_name, data_type=data_type, default_value=default_value, nots=len(match.group(1))))
    return adds, gets

def make_setting(settings, call):
    name = call.arg_name.lstrip("-")
    if name in settings:
        setting = settings[name]
    else:
        setting = settings[name] = Setting(call.arg_name)
    return setting

def flags_to_options(flag_str):
    flags = set()
    for flag in flag_str.split("|"):
        flags.add(flag.strip())

    def pop(flag):
        if flag in flags:
            flags.remove(flag)
            return True
        return False

    options = [".legacy = true"]
    if pop("ArgsManager::DEBUG_ONLY"):
        options.append(".debug_only = true")
    if pop("ArgsManager::NETWORK_ONLY"):
        options.append(".network_only = true")
    if pop("ArgsManager::SENSITIVE"):
        options.append(".sensitive = true")
    if pop("ArgsManager::DISALLOW_NEGATION"):
      options.append(".disallow_negation = true")
    if pop("ArgsManager::DISALLOW_ELISION"):
        options.append(".disallow_elision = true")
    pop("ArgsManager::ALLOW_ANY")
    if flags:
        raise Exception("Unknown flags {flags!r}")
    return options

def collect_argument_information(src_dir):
    files = get_files_with_args(src_dir)
    settings: Dict[str, Setting] = {}
    for file in files:
        adds, gets = parse_calls(file)
        for add in adds:
            setting = make_setting(settings, add.call)
            setting.adds.append(add)
        for get in gets:
            setting = make_setting(settings, get.call)
            setting.gets.append(get)

    for arg_name, setting in settings.items():
        setting_name = ''.join(word.capitalize() for word in arg_name.split('-')) + "Setting"
        counter = collections.Counter()

        for add in setting.adds:
            add.include_path = add.call.file.replace(".cpp", "_settings.h")
            key = add.call.context, add.call.arg_name
            add_setting_name = setting_name
            counter[key] += 1
            if counter[key] > 1: add_setting_name += str(counter[key])

            for get in setting.gets:
                if not add.call.context == get.call.context and not add.call.context == "main":
                    continue
                if get.add is None:
                    get.add = add
                if get.data_type in add.data_types:
                    setting_type = add.data_types[get.data_type]
                else:
                    setting_type = add.data_types[get.data_type] = SettingType(add_setting_name)
                if get.default_value is not False:
                    setting_type.defaults.add(get.default_value)

            # Clean up data types. Add if empty, remove if redundant, set primary
            if len(add.data_types) == 0:
                add.data_types[DataType.UNSET] = SettingType(add_setting_name)
            add.data_types[min(add.data_types.keys())].primary = True
            for data_type in (DataType.DISABLED, DataType.UNSET):
                if data_type in add.data_types and not add.data_types[data_type].primary:
                    del add.data_types[data_type]

            for data_type, setting_type in add.data_types.items():
                # If same setting is retrieved as different types, add suffixes to distinguish setting names
                if not setting_type.primary:
                    setting_type.name += (
                        "List" if data_type == DataType.STRING_LIST else
                        "Str" if data_type == DataType.STRING else
                        "Path" if data_type == DataType.PATH else
                        "Int" if data_type == DataType.INT else
                        "Bool" if data_type == DataType.BOOL else
                        "Disabled" if data_type == DataType.DISABLED else
                        "Unset" if data_type == DataType.UNSET else
                        None
                    )
                # Only set ::Default<> if there are no GetArg calls returning
                # std::optional and there is a single consistent default.
                if None not in setting_type.defaults and len(setting_type.defaults) == 1:
                    default_value = next(iter(setting_type.defaults))
                    assert default_value is not None
                    if default_value is not True:
                        for pattern, options in ARG_PATTERNS.items():
                            if pattern in default_value and options.extern:
                                default_value = False
                                break
                            if pattern == default_value and options.namespace:
                                default_value = f"{options.namespace}::{default_value}"
                    setting_type.default_value = default_value
    return settings

@dataclass
class SettingsHeader:
    includes: set[str] = field(default_factory=set)
    defs: list[str] = field(default_factory=list)

def generate_setting_headers(settings):
    headers_content = collections.defaultdict(SettingsHeader)
    for setting in settings.values():
        for add in setting.adds:
            header = headers_content[add.include_path]
            help_runtime = False
            extern = []
            for pattern, options in ARG_PATTERNS.items():
                if pattern in add.help_text or any(pattern in a for a in add.help_args):
                    if options.include_path:
                        header.includes.add(options.include_path)
                    help_runtime = help_runtime or options.runtime
                    if options.extern:
                        extern.append(pattern)
                        add.extern_args.append(pattern)

            for data_type, setting_type in sorted(add.data_types.items(), key=lambda p: p[0]):
                ctype = ("std::vector<std::string>" if data_type == DataType.STRING_LIST else
                         "std::string" if data_type == DataType.STRING else
                         "fs::path" if data_type == DataType.PATH else
                         "int64_t" if data_type == DataType.INT else
                         "bool" if data_type == DataType.BOOL else
                         "common::Disabled" if data_type == DataType.DISABLED else
                         "common::Unset" if data_type == DataType.UNSET else
                         None)
                if None in setting_type.defaults:
                    ctype = f"std::optional<{ctype}>"
                help_str = ""
                if setting_type.primary:
                    help_str = f",\n    {add.help_text}"
                extra = ""
                help_args = ', '.join(a for a in add.help_args)
                default_arg = (setting_type.default_value if setting_type.default_value is not True else
                               '""' if data_type == DataType.STRING else
                               "0" if data_type == DataType.INT else
                               "false" if data_type == DataType.BOOL else
                               f"{ctype}{{}}")
                if setting_type.default_value is True and (not help_args or help_args != default_arg):
                    default_arg = False
                if default_arg:
                    default_runtime = False
                    for pattern, options in ARG_PATTERNS.items():
                        if setting_type.default_value is not True and pattern in setting_type.default_value:
                            if options.include_path:
                                header.includes.add(options.include_path)
                            default_runtime = default_runtime or options.runtime
                            assert not options.extern
                    if default_runtime:
                        extra += f"\n    ::DefaultFn<[] {{ return {default_arg}; }}>"
                    else:
                        extra += f"\n    ::Default<{default_arg}>"
                if ((help_args and setting_type.primary) or default_arg) and help_args != default_arg:
                    if help_runtime or extern:
                        lambda_args = ", ".join(f"const auto& {a}" for a in ["fmt"] + extern)
                        extra += f"\n    ::HelpFn<[]({lambda_args}) {{ return strprintf(fmt, {help_args}); }}>"
                    else:
                        extra += f"\n    ::HelpArgs<{help_args}>"
                if add.category != "OptionsCategory::OPTIONS" and setting_type.primary:
                        extra += f"\n    ::Category<{add.category}>"
                options = flags_to_options(add.flags)
                # Writing common::SettingOptions{...} instead of just {...}
                # should be unneccesary because latter is valid initialization
                # syntax in C++20, but unfortunately it is only supported as of
                # clang 18. clang 17 and early versions do not seem to allow
                # using designated initializers to initialize template
                # parameters.
                options_str = f"common::SettingOptions{{{', '.join(options)}}}" if options else ""
                setting_definition = f"\nusing {setting_type.name} = common::Setting<\n    {add.summary}, {ctype}, {options_str}{help_str}>{extra};\n"
                header.defs.append(setting_definition)

    for header_file_path, header in headers_content.items():
        if not os.path.exists(header_file_path):
            guard = "BITCOIN_" + re.sub("^src/", "", header_file_path).replace('/', '_').replace('.', '_').replace('-', '_').upper()
            namespace = get_file_namespace(header_file_path)
            namespace_str = ""
            if namespace:
                namespace_str = f"namespace {namespace} {{\n}} // namespace {namespace}\n"
            with open(header_file_path, 'w') as f:
                f.write(f"#ifndef {guard}\n#define {guard}\n{namespace_str}\n#endif // {guard}\n")
        add_to_file(
            header_file_path,
            [f"#include <{include}>\n" for include in header.includes | {"common/setting.h"}],
            ["#include <string>\n", "#include <vector>\n"],
            header.defs)

def add_to_file(file_path, local_includes, system_includes=(), defs=()):
    with open(file_path, 'r') as f:
        lines = f.readlines()
    # Identify the include blocks and their positions
    local_include_start, local_include_end = None, None
    system_include_start, system_include_end = None, None
    self_include = f"#include <{file_path.replace('src/', '').replace('.cpp', '.h')}>"
    first = last = self = None
    for i, line in enumerate(lines):
        if line.startswith('#include') and "IWYU pragma: keep" not in line and not line.startswith(self_include):
            if local_include_start is None:
                local_include_start = i
            elif system_include_start is None and local_include_end is not None:
                system_include_start = i
        elif system_include_start is not None and system_include_end is None:
            system_include_end = i
        elif local_include_start is not None and local_include_end is None:
            local_include_end = i
        elif line.startswith('#include'):
            self = True
        if first is None and not line.startswith("//") and not line.startswith("#ifndef") and not line.startswith("#define") and line != "\n":
            first = i
        if line != "\n" and not line.startswith("#endif") and not line.startswith("} // namespace "):
            last = i + 1

    if system_include_start is None and system_include_end is None and not self:
        system_include_start, system_include_end = local_include_start, local_include_end
        local_include_end = system_include_start

    lines[last:last] = defs

    if system_includes:
        head = []
        tail = []
        if system_include_start is None and system_include_end is None:
            system_include_start = system_include_end = min(first, last)
            head += ["\n"]
            if first < last + 1: tail += ["\n"]
        elif local_include_end == system_include_start:
            head += ["\n"]
        existing_includes = lines[system_include_start:system_include_end]
        lines[system_include_start:system_include_end] = head + sorted(set(system_includes) | set(existing_includes)) + tail

    if local_includes:
        head = []
        if local_include_start is None and local_include_end is None:
            local_include_start = local_include_end = min(first, last)
            if lines[local_include_start-1:local_include_start+1] != ["\n", "\n"]: head = ["\n"]
        existing_includes = lines[local_include_start:local_include_end]
        lines[local_include_start:local_include_end] = head + sorted(set(local_includes) | set(existing_includes))

    with open(file_path, 'w') as f:
        f.writelines(lines)

def modify_source_files(settings):
    includes_to_add = {}
    for setting in settings.values():
        for add in setting.adds:
            header_file_path = add.include_path
            relative_include = os.path.relpath(header_file_path, start="src/").replace(os.sep, '/')
            file_path = add.call.file
            if file_path not in includes_to_add:
                includes_to_add[file_path] = set()
            includes_to_add[file_path].add(f"#include <{relative_include}>\n")
            with open(file_path, 'r') as f:
                content = f.read()
            register_args = ", ".join([add.call.obj_name] + add.extern_args)
            default_data_type = min(add.data_types.keys())
            new_content = content.replace(
                add.call.call_text,
                f"{add.data_types[default_data_type].name}::Register({register_args})"
            )
            with open(file_path, 'w') as f:
                f.write(new_content)
    # map file path -> list (old, new) replacement tuples made so far
    replacements = collections.defaultdict(list)
    for setting in settings.values():
        for get in setting.gets:
            # FIXME handle these by generating synthetic AddArg calls without corresponding Register()
            if get.add is None:
                #import pprint
                #print("*"*80)
                #print(f"Bad get call with no corresponding type")
                #pprint.pprint(get)
                continue
            header_file_path = get.add.include_path
            relative_include = os.path.relpath(header_file_path, start="src/").replace(os.sep, '/')
            file_path = get.call.file
            if file_path not in includes_to_add:
                includes_to_add[file_path] = set()
            includes_to_add[file_path].add(f"#include <{relative_include}>\n")
            with open(file_path, 'r') as f:
                content = f.read()
            if get.data_type == DataType.UNSET:
                method = "Value"
                suffix = ".isNull()"
            elif get.data_type == DataType.DISABLED:
                method = "Value"
                suffix = ".isFalse()"
            else:
                method = "Get"
                suffix = ""
            setting_type = get.add.data_types.get(get.data_type) or get.add.data_types[min(get.add.data_types.keys())]
            default_arg = ""
            if get.default_value and not setting_type.default_value:
                default_arg = (get.default_value if get.default_value is not True else
                               '""' if get.data_type == DataType.STRING else
                               "0" if get.data_type == DataType.INT else
                               "false" if get.data_type == DataType.BOOL else "{}")
                default_arg = f", {default_arg}"
            old = get.call.call_text
            new = ((get.nots + (get.data_type == DataType.UNSET)) % 2) * "!"
            if get.add.call.namespace and get.call.namespace != get.add.call.namespace:
                new += f"{get.add.call.namespace}::"
            new += f"{setting_type.name}::{method}({get.call.obj_name}{default_arg}){suffix}"
            for o, n in replacements[file_path]:
                old = old.replace(o, n)
                new = new.replace(o, n)
            replacements[file_path].append((old, new))
            new_content = content.replace(old, new)
            with open(file_path, 'w') as f:
                f.write(new_content)
    # Add necessary includes to files
    for file_path, includes in includes_to_add.items():
        add_to_file(file_path, includes)

def get_file_context(path):
    if path in ["src/bitcoin-cli.cpp"]: return "cli"
    if path in ["src/bitcoin-tx.cpp"]: return "tx"
    if path in ["src/bitcoin-util.cpp"]: return "util"
    if path in ["src/bitcoin-wallet.cpp", "src/wallet/wallettool.cpp"]: return "wallet"
    if path in ["src/test/argsman_tests.cpp", "src/test/logging_tests.cpp,", "src/test/fuzz/system.cpp", "src/test/getarg_tests.cpp"]: return "test"
    if path in ["src/zmq/zmqnotificationinterface.cpp"]: return "test" # FIX
    return "main"

def get_file_namespace(path):
    if path.startswith("src/wallet/"): return "wallet"
    return ""

@dataclass
class PatternOptions:
    include_path: str | None = None
    runtime: bool = False
    extern: bool = False
    namespace: str | None = None

# Expression patterns to look for in AddArg / GetArg calls, and options
# controlling what to do when patterns are matched.
ARG_PATTERNS = {
    # Constants
    "BITCOIN_CONF_FILENAME": PatternOptions(include_path="common/args.h", runtime=True),
    "BITCOIN_PID_FILENAME": PatternOptions(include_path="init.h", runtime=True),
    "COOKIEAUTH_FILE": PatternOptions(extern=True),
    "CURRENCY_UNIT": PatternOptions(include_path="policy/feerate.h", runtime=True),
    "DEFAULT_ACCEPT_STALE_FEE_ESTIMATES": PatternOptions(include_path="policy/fees.h"),
    "DEFAULT_ADDRESS_TYPE": PatternOptions(include_path="wallet/wallet.h"),
    "DEFAULT_ADDRMAN_CONSISTENCY_CHECKS": PatternOptions(include_path="addrman.h"),
    "DEFAULT_ASMAP_FILENAME": PatternOptions(include_path="init.h", runtime=True),
    "DEFAULT_AVOIDPARTIALSPENDS": PatternOptions(include_path="wallet/coincontrol.h", runtime=True),
    "DEFAULT_BENCH_FILTER": PatternOptions(runtime=True),
    "DEFAULT_BLOCKFILTERINDEX": PatternOptions(include_path="index/blockfilterindex.h"),
    "DEFAULT_BLOCKFILTERINDEX": PatternOptions(include_path="index/blockfilterindex.h", runtime=True),
    "DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN": PatternOptions(include_path="net_processing.h"),
    "DEFAULT_CHOOSE_DATADIR": PatternOptions(include_path="qt/intro.h"),
    "DEFAULT_COINSTATSINDEX": PatternOptions(include_path="index/coinstatsindex.h"),
    "DEFAULT_COLOR_SETTING": PatternOptions(runtime=True),
    "DEFAULT_DAEMON": PatternOptions(include_path="init.h"),
    "DEFAULT_DEBUGLOGFILE": PatternOptions(include_path="logging.h", runtime=True),
    "DEFAULT_HTTP_SERVER_TIMEOUT": PatternOptions(include_path="httpserver.h"),
    "DEFAULT_LISTEN": PatternOptions(include_path="net.h"),
    "DEFAULT_MAX_MEMPOOL_SIZE_MB": PatternOptions(include_path="kernel/mempool_options.h"),
    "DEFAULT_MAX_TRIES": PatternOptions(include_path="rpc/mining.h"),
    "DEFAULT_MAX_UPLOAD_TARGET": PatternOptions(include_path="net.h", runtime=True),
    "DEFAULT_MISBEHAVING_BANTIME": PatternOptions(include_path="banman.h"),
    "DEFAULT_NATPMP": PatternOptions(include_path="mapport.h"),
    "DEFAULT_NBLOCKS": PatternOptions(runtime=True),
    "DEFAULT_PERSIST_MEMPOOL": PatternOptions(include_path="node/mempool_persist_args.h", namespace="node"),
    "DEFAULT_PRINT_MODIFIED_FEE": PatternOptions(include_path="node/miner.h"),
    "DEFAULT_PRIORITY": PatternOptions(runtime=True),
    "DEFAULT_SPLASHSCREEN": PatternOptions(include_path="qt/guiconstants.h"),
    "DEFAULT_STOPATHEIGHT": PatternOptions(include_path="node/kernel_notifications.h"),
    "DEFAULT_TOR_CONTROL": PatternOptions(include_path="torcontrol.h", runtime=True),
    "DEFAULT_TOR_CONTROL_PORT": PatternOptions(include_path="torcontrol.h"),
    "DEFAULT_TXINDEX": PatternOptions(include_path="index/txindex.h"),
    "DEFAULT_UIPLATFORM": PatternOptions(include_path="qt/bitcoingui.h", runtime=True),
    "DEFAULT_VALIDATION_CACHE_BYTES": PatternOptions(include_path="script/sigcache.h"),
    "DEFAULT_XOR_BLOCKSDIR": PatternOptions(include_path="kernel/blockmanager_opts.h"),
    "DEFAULT_ZMQ_SNDHWM": PatternOptions(include_path="zmq/zmqabstractnotifier.h"),
    "LIST_CHAIN_NAMES": PatternOptions(include_path="chainparamsbase.h"),
    "MAX_SCRIPTCHECK_THREADS": PatternOptions(include_path="node/chainstatemanager_args.h"),
    "UNIX_EPOCH_TIME": PatternOptions(include_path="rpc/util.h"),
    "UNIX_EPOCH_TIME": PatternOptions(include_path="rpc/util.h", runtime=True),

    # Chain parameters
    "defaultChainParams": PatternOptions(extern=True),
    "testnetChainParams": PatternOptions(extern=True),
    "testnet4ChainParams": PatternOptions(extern=True),
    "signetChainParams": PatternOptions(extern=True),
    "regtestChainParams": PatternOptions(extern=True),
    "defaultBaseParams": PatternOptions(extern=True),
    "testnetBaseParams": PatternOptions(extern=True),
    "testnet4BaseParams": PatternOptions(extern=True),
    "signetBaseParams": PatternOptions(extern=True),
    "regtestBaseParams": PatternOptions(extern=True),

    # Misc expressions used as defaults
    "args.": PatternOptions(extern=True),
    "BaseParams()": PatternOptions(include_path="chainparamsbase.h", runtime=True),
    "DatabaseOptions": PatternOptions(include_path="wallet/db.h", runtime=True),
    "FormatMoney(": PatternOptions(include_path="util/moneystr.h", runtime=True),
    "FormatOutputType(": PatternOptions(include_path="outputtype.h", runtime=True),
    "gArgs": PatternOptions(include_path="common/args.h", runtime=True),
    "Join(": PatternOptions(include_path="util/string.h", runtime=True),
    "lang_territory": PatternOptions(extern=True),
    "ListBlockFilterTypes()": PatternOptions(include_path="blockfilter.h", runtime=True),
    "LogInstance()": PatternOptions(include_path="logging.h", runtime=True),
    "mempool_limits.": PatternOptions(extern=True),
    "mempool_opts.": PatternOptions(extern=True),
    "nBytesPerSigOp": PatternOptions(include_path="policy/settings.h", runtime=True),
    "nDefaultDbBatchSize": PatternOptions(include_path="txdb.h"),
    "options.": PatternOptions(extern=True),
    "PathToString(": PatternOptions(include_path="util/fs.h", runtime=True),
    "pblock->nVersion": PatternOptions(extern=True),
    '"regtest only; "': PatternOptions(runtime=True),
}

if __name__ == "__main__":
    src_dir = "src/"
    settings = collect_argument_information(src_dir)
    generate_setting_headers(settings)
    modify_source_files(settings)
