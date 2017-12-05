// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_COMMON_TYPES_H
#define BITCOIN_IPC_CAPNP_COMMON_TYPES_H

#include <chainparams.h>
#include <consensus/validation.h>
#include <ipc/capnp/common.capnp.proxy.h>
#include <mp/proxy-types.h>
#include <net_processing.h>
#include <net_types.h>
#include <netbase.h>
#include <univalue.h>
#include <util/result.h>
#include <util/translation.h>
#include <validation.h>
#include <wallet/coincontrol.h>

namespace ipc {
namespace capnp {
//! Convert kj::StringPtr to std::string.
inline std::string ToString(const kj::StringPtr& str) { return {str.cStr(), str.size()}; }

//! Convert kj::ArrayPtr to std::string.
inline std::string ToString(const kj::ArrayPtr<const kj::byte>& array)
{
    return {reinterpret_cast<const char*>(array.begin()), array.size()};
}

//! Convert kj::ArrayPtr to base_blob.
template <typename T>
inline T ToBlob(const kj::ArrayPtr<const kj::byte>& array)
{
    return T({array.begin(), array.begin() + array.size()});
}

//! Convert base_blob to kj::ArrayPtr.
template <typename T>
inline kj::ArrayPtr<const kj::byte> ToArray(const T& blob)
{
    return {reinterpret_cast<const kj::byte*>(blob.data()), blob.size()};
}

//! Serialize bitcoin value.
template <typename T>
CDataStream Serialize(const T& value)
{
    CDataStream stream(SER_NETWORK, CLIENT_VERSION);
    value.Serialize(stream);
    return stream;
}

//! Deserialize bitcoin value.
template <typename T>
T Unserialize(const kj::ArrayPtr<const kj::byte>& data)
{
    SpanReader stream{SER_NETWORK, CLIENT_VERSION, {data.begin(), data.end()}};
    T value;
    value.Unserialize(stream);
    return value;
}

template <typename T>
using Deserializable = std::is_constructible<T, ::deserialize_type, ::CDataStream&>;

template <typename T>
struct Unserializable
{
private:
    template <typename C>
    static std::true_type test(decltype(std::declval<C>().Unserialize(std::declval<C&>()))*);
    template <typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(nullptr))::value;
};

template <typename T>
struct Serializable
{
private:
    template <typename C>
    static std::true_type test(decltype(std::declval<C>().Serialize(std::declval<C&>()))*);
    template <typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(nullptr))::value;
};
} // namespace capnp
} // namespace ipc

//! Functions to serialize / deserialize common bitcoin types.
namespace mp {
template <typename Value, typename Output>
void CustomBuildField(TypeList<std::chrono::seconds>, Priority<1>, InvokeContext& invoke_context, Value&& value,
                      Output&& output)
{
    output.set(value.count());
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::chrono::seconds>, Priority<1>, InvokeContext& invoke_context,
                               Input&& input, ReadDest&& read_dest)
{
    return read_dest.construct(input.get());
}

template <typename Value, typename Output>
void CustomBuildField(TypeList<std::chrono::microseconds>, Priority<1>, InvokeContext& invoke_context, Value&& value,
                      Output&& output)
{
    output.set(value.count());
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::chrono::microseconds>, Priority<1>, InvokeContext& invoke_context,
                               Input&& input, ReadDest&& read_dest)
{
    return read_dest.construct(input.get());
}

template <typename Value, typename Output>
void CustomBuildField(TypeList<fs::path>, Priority<1>, InvokeContext& invoke_context, Value&& path, Output&& output)
{
    std::string str = fs::PathToString(path);
    auto result = output.init(str.size());
    memcpy(result.begin(), str.data(), str.size());
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<fs::path>, Priority<1>, InvokeContext& invoke_context, Input&& input,
                               ReadDest&& read_dest)
{
    auto data = input.get();
    return read_dest.construct(fs::PathFromString({CharCast(data.begin()), data.size()}));
}

template <typename Value, typename Output>
void CustomBuildField(TypeList<SecureString>, Priority<1>, InvokeContext& invoke_context, Value&& str, Output&& output)
{
    auto result = output.init(str.size());
    // Copy SecureString into output. Caller needs to be responsible for calling
    // memory_cleanse later on the output after it is sent.
    memcpy(result.begin(), str.data(), str.size());
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<SecureString>, Priority<1>, InvokeContext& invoke_context, Input&& input,
                               ReadDest&& read_dest)
{
    auto data = input.get();
    // Copy input into SecureString. Caller needs to be responsible for calling
    // memory_cleanse on the input.
    return read_dest.construct(CharCast(data.begin()), data.size());
}

template <typename Value, typename Output>
void CustomBuildField(TypeList<UniValue>, Priority<1>, InvokeContext& invoke_context, Value&& value, Output&& output)
{
    std::string str = value.write();
    auto result = output.init(str.size());
    memcpy(result.begin(), str.data(), str.size());
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<UniValue>, Priority<1>, InvokeContext& invoke_context, Input&& input,
                               ReadDest&& read_dest)
{
    return read_dest.update([&](auto& value) {
        auto data = input.get();
        value.read(std::string_view{data.begin(), data.size()});
    });
}

template <typename Output>
void CustomBuildField(TypeList<UniValue::type_error>, Priority<1>, InvokeContext& invoke_context,
                      const UniValue::type_error& value, Output&& output)
{
    BuildField(TypeList<std::string>(), invoke_context, output, std::string(value.what()));
}

template <typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<UniValue::type_error>, Priority<1>, InvokeContext& invoke_context,
                               Input&& input, ReadDest&& read_dest)
{
    read_dest.construct(ReadField(TypeList<std::string>(), invoke_context, input, mp::ReadDestTemp<std::string>()));
}

template <typename Output>
void CustomBuildField(
    TypeList<>, Priority<1>, InvokeContext& invoke_context, Output&& output,
    typename std::enable_if<std::is_same<decltype(output.get()),
                                         ipc::capnp::messages::GlobalArgs::Builder>::value>::type* enable = nullptr)
{
    ipc::capnp::BuildGlobalArgs(invoke_context, output.init());
}

template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
auto CustomPassField(TypeList<>, ServerContext& server_context, const Fn& fn, Args&&... args) ->
    typename std::enable_if<std::is_same<decltype(Accessor::get(server_context.call_context.getParams())),
                                         ipc::capnp::messages::GlobalArgs::Reader>::value>::type
{
    ipc::capnp::ReadGlobalArgs(server_context, Accessor::get(server_context.call_context.getParams()));
    return fn.invoke(server_context, std::forward<Args>(args)...);
}

template <typename LocalType, typename Value, typename Output>
void CustomBuildField(
    TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Value&& value, Output&& output,
    typename std::enable_if<ipc::capnp::Serializable<
        typename std::remove_cv<typename std::remove_reference<Value>::type>::type>::value>::type* enable = nullptr)
{
    CDataStream stream(SER_NETWORK, CLIENT_VERSION);
    value.Serialize(stream);
    auto result = output.init(stream.size());
    memcpy(result.begin(), stream.data(), stream.size());
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto)
CustomReadField(TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest,
                typename std::enable_if<ipc::capnp::Deserializable<LocalType>::value>::type* enable = nullptr)
{
    assert(input.has());
    auto data = input.get();
    SpanReader stream(SER_NETWORK, CLIENT_VERSION, {data.begin(), data.end()});
    // TODO: instead of always preferring Deserialize implementation over
    // Unserialize should prefer Deserializing when emplacing, unserialize when
    // updating. Can implement by adding read_dest.alreadyConstructed()
    // constexpr bool method in libmultiprocess.
    return read_dest.construct(deserialize, stream);
}

template <typename LocalType, typename Input, typename ReadDest>
decltype(auto)
CustomReadField(TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest,
                typename std::enable_if<ipc::capnp::Unserializable<LocalType>::value &&
                                        !ipc::capnp::Deserializable<LocalType>::value>::type* enable = nullptr)
{
    return read_dest.update([&](auto& value) {
        if (!input.has()) return;
        auto data = input.get();
        SpanReader stream(SER_NETWORK, CLIENT_VERSION, {data.begin(), data.end()});
        value.Unserialize(stream);
    });
}

//! util::Result builder.
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<util::Result<LocalType>>, Priority<1>, InvokeContext& invoke_context, Value&& value,
                      Output&& output)
{
    auto result = output.init();
    if (value) {
        using ValueAccessor = typename ProxyStruct<typename decltype(result)::Builds>::ValueAccessor;
        BuildField(TypeList<LocalType>(), invoke_context, Make<StructField, ValueAccessor>(result), *value);
    } else {
        BuildField(TypeList<bilingual_str>(), invoke_context, Make<ValueField>(result.initError()),
                   util::ErrorString(value));
    }
}

//! util::Result reader.
template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<util::Result<LocalType>>, Priority<1>, InvokeContext& invoke_context,
                               Input&& input, ReadDest&& read_dest)
{
    auto result = input.get();
    return read_dest.update([&](auto& value) {
        if (result.hasValue()) {
            ReadField(mp::TypeList<LocalType>(), invoke_context, mp::Make<mp::ValueField>(result.getValue()),
                      mp::ReadDestEmplace(
                          mp::TypeList<LocalType>(), [&](auto&&... args) -> auto& {
                              value = LocalType{std::forward<decltype(args)>(args)...};
                              return *value;
                          }));
        } else {
            bilingual_str error;
            ReadField(mp::TypeList<bilingual_str>(), invoke_context, mp::Make<mp::ValueField>(result.getError()),
                      mp::ReadDestValue(error));
            value = util::Error{std::move(error)};
        }
    });
}

//! Forwarder to CustomBuildMessage.
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<LocalType>, Priority<2>, InvokeContext& invoke_context, Value&& value, Output&& output,
                      decltype(CustomBuildMessage(invoke_context, value, output.get()))* enable = nullptr)
{
    CustomBuildMessage(invoke_context, value, output.init());
}

//! Forwarder to CustomReadMessage.
template <typename LocalType, typename Reader, typename ReadDest>
decltype(auto) CustomReadField(TypeList<LocalType>, Priority<2>, InvokeContext& invoke_context, Reader&& reader,
                               ReadDest&& read_dest,
                               decltype(CustomReadMessage(invoke_context, reader.get(),
                                                          std::declval<LocalType&>()))* enable = nullptr)
{
    return read_dest.update([&](auto& value) { CustomReadMessage(invoke_context, reader.get(), value); });
}

template <typename Accessor, typename Message>
decltype(auto) MaybeGet(Message&& message, decltype(Accessor::get(message))* enable = nullptr)
{
    return Accessor::get(message);
}

template <typename Accessor>
::capnp::Void MaybeGet(...)
{
    return {};
}

template <typename Accessor, typename Message>
decltype(auto) MaybeInit(Message&& message, decltype(Accessor::get(message))* enable = nullptr)
{
    return Accessor::init(message);
}

template <typename Accessor>
::capnp::Void MaybeInit(...)
{
    return {};
}

//! Forwarder to CustomPassMessage.
template <typename Accessor, typename... LocalTypes, typename ServerContext, typename Fn, typename... Args>
auto CustomPassField(TypeList<LocalTypes...>, ServerContext& server_context, Fn&& fn, Args&&... args)
    -> decltype(CustomPassMessage(server_context, MaybeGet<Accessor>(server_context.call_context.getParams()),
                                  MaybeGet<Accessor>(server_context.call_context.getResults()), nullptr))
{
    CustomPassMessage(server_context, MaybeGet<Accessor>(server_context.call_context.getParams()),
                      MaybeInit<Accessor>(server_context.call_context.getResults()),
                      [&](LocalTypes... param) { fn.invoke(server_context, std::forward<Args>(args)..., param...); });
}

//! Generic ::capnp::Data field builder for any class that a Span can be
//! constructed from, particularly BaseHash and base_blob classes and
//! subclasses. It's also used to serialize vector<unsigned char>
//! GCSFilter::ElementSet set elements.
//!
//! There is currently no corresponding ::capnp::Data CustomReadField function
//! that works using Spans, because the bitcoin classes in the codebase like
//! BaseHash and blob_blob that can converted /to/ Span don't currently have
//! Span constructors that allow them to be constructed /from/ Span. For example
//! the PKHash class only has a constructor expecting a uint160 argument. If
//! these classes were given constructors accepting Span arguments, a generic
//! CustomReadField function could be written that would allow dropping
//! specialized CustomReadField functions for types like PKHash.
//!
//! For the LocalType = vector<unsigned char> case, it's also not necessary to
//! have ::capnp::Data CustomReadField function corresponding to this
//! CustomBuildField function because ::capnp::Data inherits from
//! ::capnp::ArrayPtr, and libmultiprocess already provides a generic
//! CustomReadField function that can read from ::capnp::ArrayPtr into
//! std::vector.
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(
    TypeList<LocalType>, Priority<2>, InvokeContext& invoke_context, Value&& value, Output&& output,
    typename std::enable_if<!ipc::capnp::Serializable<typename std::remove_cv<
        typename std::remove_reference<Value>::type>::type>::value>::type* enable_not_serializable = nullptr,
    typename std::enable_if<std::is_same<decltype(output.get()), ::capnp::Data::Builder>::value>::type* enable_output =
        nullptr,
    decltype(Span{value}) * enable_value = nullptr)
{
    auto data = Span{value};
    auto result = output.init(data.size());
    memcpy(result.begin(), data.data(), data.size());
}

// libmultiprocess only provides read/build functions for std::set, not
// std::unordered_set, so copy and paste those functions here.
// TODO: Move these to libmultiprocess and dedup std::set, std::unordered_set,
// and std::vector implementations.
template <typename LocalType, typename Hash, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::unordered_set<LocalType, Hash>>, Priority<1>,
                               InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest)
{
    return read_dest.update([&](auto& value) {
        auto data = input.get();
        value.clear();
        for (auto item : data) {
            ReadField(TypeList<LocalType>(), invoke_context, Make<ValueField>(item),
                      ReadDestEmplace(
                          TypeList<const LocalType>(), [&](auto&&... args) -> auto& {
                              return *value.emplace(std::forward<decltype(args)>(args)...).first;
                          }));
        }
    });
}

template <typename LocalType, typename Hash, typename Value, typename Output>
void CustomBuildField(TypeList<std::unordered_set<LocalType, Hash>>, Priority<1>, InvokeContext& invoke_context,
                      Value&& value, Output&& output)
{
    auto list = output.init(value.size());
    size_t i = 0;
    for (const auto& elem : value) {
        BuildField(TypeList<LocalType>(), invoke_context, ListOutput<typename decltype(list)::Builds>(list, i), elem);
        ++i;
    }
}
} // namespace mp

#endif // BITCOIN_IPC_CAPNP_COMMON_TYPES_H
