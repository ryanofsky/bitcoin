// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_CAPNP_COMMON_TYPES_H
#define BITCOIN_INTERFACES_CAPNP_COMMON_TYPES_H

#include <chainparams.h>
#include <interfaces/capnp/chain.capnp.proxy.h>
#include <interfaces/capnp/common.capnp.proxy.h>
#include <interfaces/capnp/handler.capnp.proxy.h>
#include <interfaces/capnp/init.capnp.proxy.h>
#include <interfaces/capnp/node.capnp.proxy.h>
#include <interfaces/capnp/wallet.capnp.proxy.h>
#include <mp/proxy-types.h>
#include <net_processing.h>
#include <netbase.h>
#include <validation.h>
#include <wallet/coincontrol.h>

#include <consensus/validation.h>

namespace interfaces {
namespace capnp {

//! Convert kj::StringPtr to std::string.
inline std::string ToString(const kj::StringPtr& str) { return {str.cStr(), str.size()}; }

//! Convert kj::ArrayPtr to std::string.
inline std::string ToString(const kj::ArrayPtr<const kj::byte>& data)
{
    return {reinterpret_cast<const char*>(data.begin()), data.size()};
}

//! Convert array object to kj::ArrayPtr.
template <typename Array>
inline kj::ArrayPtr<const kj::byte> ToArray(const Array& array)
{
    return {reinterpret_cast<const kj::byte*>(array.data()), array.size()};
}

//! Convert base_blob to kj::ArrayPtr.
template <typename Blob>
inline kj::ArrayPtr<const kj::byte> FromBlob(const Blob& blob)
{
    return {blob.begin(), blob.size()};
}

//! Convert kj::ArrayPtr to base_blob
template <typename Blob>
inline Blob ToBlob(kj::ArrayPtr<const kj::byte> data)
{
    // TODO: Avoid temp vector.
    return Blob(std::vector<unsigned char>(data.begin(), data.begin() + data.size()));
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
T Unserialize(T& value, const kj::ArrayPtr<const kj::byte>& data)
{
    // Could optimize, it unnecessarily copies the data into a temporary vector.
    CDataStream stream(reinterpret_cast<const char*>(data.begin()), reinterpret_cast<const char*>(data.end()),
        SER_NETWORK, CLIENT_VERSION);
    value.Unserialize(stream);
    return value;
}

//! Deserialize bitcoin value.
template <typename T>
T Unserialize(const kj::ArrayPtr<const kj::byte>& data)
{
    T value;
    Unserialize(value, data);
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
} // namespace interfaces

namespace mp {
//!@{
//! Functions to serialize / deserialize bitcoin objects that don't
//! already provide their own serialization.
void CustomBuildMessage(InvokeContext& invoke_context,
    const UniValue& univalue,
    interfaces::capnp::messages::UniValue::Builder&& builder);
void CustomReadMessage(InvokeContext& invoke_context,
    const interfaces::capnp::messages::UniValue::Reader& reader,
    UniValue& univalue);
//!@}

template <typename LocalType, typename Reader, typename Value>
void ReadFieldUpdate(mp::TypeList<LocalType>,
    mp::InvokeContext& invoke_context,
    Reader&& reader,
    Value&& value,
    decltype(CustomReadMessage(invoke_context, reader.get(), value))* enable = nullptr)
{
    CustomReadMessage(invoke_context, reader.get(), value);
}

template <typename LocalType, typename Input, typename Emplace>
void ReadFieldNew(mp::TypeList<LocalType>,
    InvokeContext& invoke_context,
    Input&& input,
    Emplace&& emplace,
    typename std::enable_if<interfaces::capnp::Deserializable<LocalType>::value>::type* enable = nullptr)
{
    if (!input.has()) return;
    auto data = input.get();
    // Note: stream copy here is unnecessary, and can be avoided in the future
    // when `VectorReader` from #12254 is added.
    CDataStream stream(CharCast(data.begin()), CharCast(data.end()), SER_NETWORK, CLIENT_VERSION);
    emplace(deserialize, stream);
}

template <typename LocalType, typename Input, typename Value>
void ReadFieldUpdate(mp::TypeList<LocalType>,
    InvokeContext& invoke_context,
    Input&& input,
    Value& value,
    typename std::enable_if<interfaces::capnp::Unserializable<Value>::value>::type* enable = nullptr)
{
    if (!input.has()) return;
    auto data = input.get();
    // Note: stream copy here is unnecessary, and can be avoided in the future
    // when `VectorReader` from #12254 is added.
    CDataStream stream(CharCast(data.begin()), CharCast(data.end()), SER_NETWORK, CLIENT_VERSION);
    value.Unserialize(stream);
}

template <typename Input, typename Emplace>
void ReadFieldNew(mp::TypeList<SecureString>, InvokeContext& invoke_context, Input&& input, Emplace&& emplace)
{
    auto data = input.get();
    // Copy input into SecureString. Caller needs to be responsible for calling
    // memory_cleanse on the input.
    emplace(CharCast(data.begin()), data.size());
}

template <typename Value, typename Output>
void CustomBuildField(mp::TypeList<SecureString>,
    mp::Priority<1>,
    InvokeContext& invoke_context,
    Value&& str,
    Output&& output)
{
    auto result = output.init(str.size());
    // Copy SecureString into output. Caller needs to be responsible for calling
    // memory_cleanse later on the output after it is sent.
    memcpy(result.begin(), str.data(), str.size());
}

template <typename LocalType, typename Value, typename Output>
void CustomBuildField(mp::TypeList<LocalType>,
    mp::Priority<2>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output,
    decltype(CustomBuildMessage(invoke_context, value, output.init()))* enable = nullptr)
{
    CustomBuildMessage(invoke_context, value, output.init());
}

template <typename LocalType, typename Value, typename Output>
void CustomBuildField(mp::TypeList<LocalType>,
    mp::Priority<1>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output,
    typename std::enable_if<interfaces::capnp::Serializable<
        typename std::remove_cv<typename std::remove_reference<Value>::type>::type>::value>::type* enable = nullptr)
{
    CDataStream stream(SER_NETWORK, CLIENT_VERSION);
    value.Serialize(stream);
    auto result = output.init(stream.size());
    memcpy(result.begin(), stream.data(), stream.size());
}

template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
auto CustomPassField(mp::TypeList<>, ServerContext& server_context, const Fn& fn, Args&&... args) ->
    typename std::enable_if<std::is_same<decltype(Accessor::get(server_context.call_context.getParams())),
        interfaces::capnp::messages::GlobalArgs::Reader>::value>::type
{
    interfaces::capnp::ReadGlobalArgs(server_context, Accessor::get(server_context.call_context.getParams()));
    return fn.invoke(server_context, std::forward<Args>(args)...);
}

template <typename Output>
void CustomBuildField(mp::TypeList<>,
    mp::Priority<1>,
    InvokeContext& invoke_context,
    Output&& output,
    typename std::enable_if<std::is_same<decltype(output.init()),
        interfaces::capnp::messages::GlobalArgs::Builder>::value>::type* enable = nullptr)
{
    interfaces::capnp::BuildGlobalArgs(invoke_context, output.init());
}

template <typename Value>
struct OptionalEmplace
{
    Value& m_value;

    OptionalEmplace(Value& value) : m_value(value) {}

    template <typename... Params>
    auto operator()(Params&&... params) const -> decltype(*m_value)&
    {
        m_value.emplace(std::forward<Params>(params)...);
        return *m_value;
    }
};

template <typename LocalType, typename Input, typename DestValue>
void ReadFieldUpdate(TypeList<Optional<LocalType>>, InvokeContext& invoke_context, Input&& input, DestValue&& value)
{
    if (!input.has()) {
        value.reset();
        return;
    }
    if (value) {
        ReadFieldUpdate(TypeList<LocalType>(), invoke_context, input, *value);
    } else {
        ReadField(TypeList<LocalType>(), invoke_context, input, OptionalEmplace<DestValue>(value));
    }
}

template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<Optional<LocalType>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Value&& value,
    Output&& output)
{
    if (value) {
        output.setHas();
        BuildField(TypeList<LocalType>(), invoke_context, output, *value);
    }
}

} // namespace mp

#endif // BITCOIN_INTERFACES_CAPNP_COMMON_TYPES_H
