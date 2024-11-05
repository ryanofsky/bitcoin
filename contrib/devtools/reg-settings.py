import collections
import enum
import os
import re
import subprocess
from dataclasses import dataclass, field
from typing import Literal

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

DefaultValue = str | None | Literal[True]

def from_default_value(default_value: DefaultValue, data_type: DataType) -> str:
    if default_value is True:
        if data_type == DataType.STRING_LIST: return "std::vector<std::string>{}"
        if data_type == DataType.STRING: return '""'
        if data_type == DataType.PATH: return "fs::path{}"
        if data_type == DataType.INT: return "0"
        if data_type == DataType.BOOL: return "false"
    return default_value

def to_default_value(expr: str, data_type: DataType) -> DefaultValue:
    if data_type == DataType.STRING_LIST and expr == "std::vector<std::string>{}": return True
    if data_type == DataType.STRING and expr == '""': return True
    if data_type == DataType.PATH and expr == "fs::path{}": return True
    if data_type == DataType.INT and expr == "0": return True
    if data_type == DataType.BOOL and expr == "false": return True
    return expr

@dataclass
class SettingType:
    name: str
    primary: bool = False
    defaults: set[str | None] = field(default_factory=set)
    default_value: DefaultValue = None

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
    optional: bool = False
    extern_args: list[str] = field(default_factory=list)

@dataclass
class GetArg:
    call: Call
    function_name: str
    data_type: DataType | None
    default_value: DefaultValue = None
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
            "git", "grep", "-l", "AddArg(\|GetArg(\|GetIntArg(\|GetBoolArg(\|GetArgs", "--", src_dir
        ],
        capture_output=True,
        text=True
    )
    return result.stdout.splitlines()

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
        for match in re.finditer(r'\b([\w.]+)(\.|->)(GetArg|GetPathArg|GetIntArg|GetBoolArg|GetArgs|IsArgSet|IsArgNegated)\((.)', content):
            call_len, call_args = parse_function_args(content[match.start(4):])
            obj_name = match.group(1)
            if match.group(2) == "->":
                obj_name = f"*{obj_name}"
            call = Call(
                file=file_path,
                position=match.start(),
                call_text=content[match.start():match.start(4)+call_len+1],
                obj_name=obj_name,
                arg_name=call_args[0].strip().strip('"'),
                context=context,
                namespace=namespace,
            )
            function_name = match.group(3)
            data_type = (DataType.STRING_LIST if function_name == "GetArgs" else
                         DataType.STRING if function_name == "GetArg" else
                         DataType.PATH if function_name == "GetPathArg" else
                         DataType.INT if function_name == "GetIntArg" else
                         DataType.BOOL if function_name == "GetBoolArg" else
                         DataType.DISABLED if function_name == "IsArgNegated" else
                         None)
            gets.append(GetArg(
                call=call,
                function_name=function_name,
                data_type=data_type,
                default_value=to_default_value(call_args[1].strip(), data_type) if len(call_args) > 1 else
                              True if function_name == "GetPathArg" else
                              True if function_name == "GetArgs" else None,
            ))
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
                if get.data_type is None:
                    add.optional = True
                else:
                    if get.data_type in add.data_types:
                        setting_type = add.data_types[get.data_type]
                    else:
                        setting_type = add.data_types[get.data_type] = SettingType(add_setting_name)
                    setting_type.defaults.add(get.default_value)

            if len(add.data_types) == 0:
                add.data_types[None] = SettingType(add_setting_name)

            # If same setting is retrieved as different types, add suffixes to distinguish setting names
            add.data_types[min(add.data_types.keys())].primary = True
            for data_type, setting_type in add.data_types.items():
                if not setting_type.primary:
                    setting_type.name += (
                        "List" if data_type == DataType.STRING_LIST else
                        "Str" if data_type == DataType.STRING else
                        "Path" if data_type == DataType.PATH else
                        "Int" if data_type == DataType.INT else
                        "Bool" if data_type == DataType.BOOL else
                        "Disabled" if data_type == DataType.DISABLED else
                        None
                    )
                if not (add.optional or None in setting_type.defaults or len(setting_type.defaults) != 1):
                    default_value = next(iter(setting_type.defaults))
                    assert default_value is not None
                    if default_value is not True:
                        for pattern, help_arg in HELP_ARGS.items():
                            if pattern in default_value and help_arg.extern:
                                default_value = False
                                break
                            if pattern == default_value and help_arg.namespace:
                                default_value = f"{help_arg.namespace}::{default_value}"
                    if default_value is not False:
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
            for pattern, help_arg in HELP_ARGS.items():
                if pattern in add.help_text or any(pattern in a for a in add.help_args):
                    if help_arg.include_path:
                        header.includes.add(help_arg.include_path)
                    help_runtime = help_runtime or help_arg.runtime
                    if help_arg.extern:
                        extern.append(pattern)
                        add.extern_args.append(pattern)

            for data_type, setting_type in sorted(add.data_types.items(), key=lambda p: p[0]):
                ctype = ("std::vector<std::string>" if data_type == DataType.STRING_LIST else
                         "std::string" if data_type == DataType.STRING else
                         "fs::path" if data_type == DataType.PATH else
                         "int64_t" if data_type == DataType.INT else
                         "bool" if data_type == DataType.BOOL else
                         "common::Disabled" if data_type == DataType.DISABLED else
                         "char")
                if setting_type.default_value is None:
                    ctype = f"std::optional<{ctype}>"
                help_str = ""
                if setting_type.primary:
                    help_str = f",\n    {add.help_text}"
                extra = ""
                help_args = ', '.join(a for a in add.help_args)
                default_arg = from_default_value(setting_type.default_value, data_type) or ""
                if setting_type.default_value is True and (not help_args or help_args != default_arg):
                    default_arg = ""
                if default_arg:
                    default_runtime = False
                    for pattern, help_arg in HELP_ARGS.items():
                        if setting_type.default_value is not True and pattern in setting_type.default_value:
                            if help_arg.include_path:
                                header.includes.add(help_arg.include_path)
                            default_runtime = default_runtime or help_arg.runtime
                            assert not help_arg.extern
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
                if add.category != "OptionsCategory::OPTIONS":
                        extra += f"\n    ::Category<{add.category}>"
                options = flags_to_options(add.flags)
                options_str = f"{{{', '.join(options)}}}" if options else ""
                setting_definition = f"\nusing {setting_type.name} = common::Setting<\n    {add.summary}, {add.help_text if setting_type.primary else 'nullptr'},\n    {ctype}, {add.category}{options_str}>{extra};\n"
                setting_definition = f"\nusing {setting_type.name} = common::Setting<\n    {add.summary}, {ctype}, {options_str}{help_str}>{extra};\n"
                header.defs.append(setting_definition)

    for header_file_path, header in headers_content.items():
        if not os.path.exists(header_file_path):
            guard = re.sub("^src/", "", header_file_path).replace('/', '_').replace('.', '_').replace('-', '_').upper()
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
    first = last = None
    for i, line in enumerate(lines):
        #print(f"{i=!r} {line=!r}")
        if line.startswith('#include') and "IWYU pragma: keep" not in line and not line.startswith(self_include):
            if local_include_start is None:
                local_include_start = i
            elif system_include_start is None and local_include_end is not None:
                system_include_start = i
        elif system_include_start is not None and system_include_end is None:
            system_include_end = i
        elif local_include_start is not None and local_include_end is None:
            local_include_end = i
        if first is None and not line.startswith("//") and not line.startswith("#ifndef") and not line.startswith("#define") and line != "\n":
            first = i
        if line != "\n" and not line.startswith("#endif") and not line.startswith("} // namespace "):
            last = i + 1

    lines[last:last] = defs

    if system_includes:
        head = []
        tail = []
        if system_include_start is None and system_include_end is None:
            system_include_start = system_include_end = min(first, last)
            head = ["\n"]
            if first < last + 1: tail = ["\n"]
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
            setting_type = get.add.data_types[get.data_type or min(get.add.data_types.keys())]
            suffix = ""
            if get.default_value is not None and not setting_type.default_value:
                suffix = f".value_or({from_default_value(get.default_value, get.data_type)})"
            prefix = ""
            if get.add.call.namespace and get.call.namespace != get.add.call.namespace:
                prefix = f"{get.add.call.namespace}::"
            new_content = content.replace(
                get.call.call_text,
                f"{prefix}{setting_type.name}::Get({get.call.obj_name}){suffix}"
            )
            with open(file_path, 'w') as f:
                f.write(new_content)
    # Add necessary includes to files
    for file_path, includes in includes_to_add.items():
        add_to_file(file_path, includes)

@dataclass
class HelpArg:
    include_path: str | None = None
    runtime: bool = False
    extern: bool = False
    namespace: str | None = None

HELP_ARGS = {
    "defaultChainParams": HelpArg(extern=True),
    "testnetChainParams": HelpArg(extern=True),
    "testnet4ChainParams": HelpArg(extern=True),
    "signetChainParams": HelpArg(extern=True),
    "regtestChainParams": HelpArg(extern=True),
    "defaultBaseParams": HelpArg(extern=True),
    "testnetBaseParams": HelpArg(extern=True),
    "testnet4BaseParams": HelpArg(extern=True),
    "signetBaseParams": HelpArg(extern=True),
    "regtestBaseParams": HelpArg(extern=True),
    "nDefaultDbBatchSize": HelpArg(include_path="txdb.h"),
    "DEFAULT_MAX_TRIES": HelpArg(include_path="rpc/mining.h"),
    "DEFAULT_COLOR_SETTING": HelpArg(runtime=True),
    "DEFAULT_AVOIDPARTIALSPENDS": HelpArg(include_path="wallet/coincontrol.h", runtime=True),
    "DatabaseOptions": HelpArg(include_path="wallet/db.h", runtime=True),
    "DEFAULT_ADDRESS_TYPE": HelpArg(include_path="wallet/wallet.h"),
    #"": HelpArg(include_path="", runtime=True),
    #"": HelpArg(include_path="", runtime=True),
    #"": HelpArg(include_path="", runtime=True),
    "DEFAULT_DEBUGLOGFILE": HelpArg(include_path="logging.h", runtime=True),
    "BITCOIN_CONF_FILENAME": HelpArg(include_path="common/args.h", runtime=True),
    "BITCOIN_PID_FILENAME": HelpArg(include_path="init.h", runtime=True),
    "CURRENCY_UNIT": HelpArg(include_path="policy/feerate.h", runtime=True),
    "DEFAULT_ACCEPT_STALE_FEE_ESTIMATES": HelpArg(include_path="policy/fees.h"),
    "DEFAULT_ADDRMAN_CONSISTENCY_CHECKS": HelpArg(include_path="addrman.h"),
    "DEFAULT_ASMAP_FILENAME": HelpArg(include_path="init.h", runtime=True),
    "DEFAULT_BLOCKFILTERINDEX": HelpArg(include_path="index/blockfilterindex.h"),
    "DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN": HelpArg(include_path="net_processing.h"),
    "DEFAULT_COINSTATSINDEX": HelpArg(include_path="index/coinstatsindex.h"),
    "DEFAULT_DAEMON": HelpArg(include_path="init.h"),
    "DEFAULT_HTTP_SERVER_TIMEOUT": HelpArg(include_path="httpserver.h"),
    "DEFAULT_LISTEN": HelpArg(include_path="net.h"),
    "DEFAULT_MAX_MEMPOOL_SIZE_MB": HelpArg(include_path="kernel/mempool_options.h"),
    "DEFAULT_MAX_UPLOAD_TARGET": HelpArg(include_path="net.h", runtime=True),
    "DEFAULT_MISBEHAVING_BANTIME": HelpArg(include_path="banman.h"),
    "DEFAULT_NATPMP": HelpArg(include_path="mapport.h"),
    "DEFAULT_PERSIST_MEMPOOL": HelpArg(include_path="node/mempool_persist_args.h", namespace="node"),
    "DEFAULT_PRINT_MODIFIED_FEE": HelpArg(include_path="node/miner.h"),
    "DEFAULT_STOPATHEIGHT": HelpArg(include_path="node/kernel_notifications.h"),
    "DEFAULT_NBLOCKS": HelpArg(runtime=True),
    "DEFAULT_TOR_CONTROL": HelpArg(include_path="torcontrol.h", runtime=True),
    "DEFAULT_TOR_CONTROL_PORT": HelpArg(include_path="torcontrol.h"),
    "DEFAULT_TXINDEX": HelpArg(include_path="index/txindex.h"),
    "DEFAULT_VALIDATION_CACHE_BYTES": HelpArg(include_path="script/sigcache.h"),
    "DEFAULT_XOR_BLOCKSDIR": HelpArg(include_path="kernel/blockmanager_opts.h"),
    "DEFAULT_ZMQ_SNDHWM": HelpArg(include_path="zmq/zmqabstractnotifier.h"),
    "LIST_CHAIN_NAMES": HelpArg(include_path="chainparamsbase.h"),
    "MAX_SCRIPTCHECK_THREADS": HelpArg(include_path="node/chainstatemanager_args.h"),
    "UNIX_EPOCH_TIME": HelpArg(include_path="rpc/util.h"),
    "UNIX_EPOCH_TIME": HelpArg(include_path="rpc/util.h", runtime=True),
    "FormatMoney(": HelpArg(include_path="util/moneystr.h", runtime=True),
    "Join(": HelpArg(include_path="util/string.h", runtime=True),
    "ListBlockFilterTypes()": HelpArg(include_path="blockfilter.h", runtime=True),
    "LogInstance()": HelpArg(include_path="logging.h", runtime=True),
    "PathToString(": HelpArg(include_path="util/fs.h", runtime=True),
    "FormatOutputType(": HelpArg(include_path="outputtype.h", runtime=True),
    '"regtest only; "': HelpArg(runtime=True),
    "BaseParams()": HelpArg(include_path="chainparamsbase.h", runtime=True),
    "gArgs": HelpArg(include_path="common/args.h", runtime=True),
    "pblock->nVersion": HelpArg(extern=True),
    "options.": HelpArg(extern=True),
    "mempool_opts.": HelpArg(extern=True),
    "mempool_limits.": HelpArg(extern=True),
    "nBytesPerSigOp": HelpArg(include_path="policy/settings.h", runtime=True),
    "DEFAULT_BLOCKFILTERINDEX": HelpArg(include_path="index/blockfilterindex.h", runtime=True),
}

if __name__ == "__main__":
    src_dir = "src/"
    settings = collect_argument_information(src_dir)
    generate_setting_headers(settings)
    modify_source_files(settings)
