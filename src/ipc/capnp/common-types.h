// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_COMMON_TYPES_H
#define BITCOIN_IPC_CAPNP_COMMON_TYPES_H

#include <clientversion.h>
<<<<<<< HEAD
#include <interfaces/types.h>
#include <primitives/transaction.h>
#include <serialize.h>
||||||| parent of 8a20be71f07c (multiprocess: Add bitcoin-mine test program)
=======
#include <common/settings.h>
#include <ipc/capnp/common.capnp.proxy.h>
#include <primitives/transaction.h>
#include <protocol.h>
>>>>>>> 8a20be71f07c (multiprocess: Add bitcoin-mine test program)
#include <streams.h>
#include <univalue.h>

#include <cstddef>
#include <mp/proxy-types.h>
#include <type_traits>
#include <utility>

namespace ipc {
namespace capnp {
<<<<<<< HEAD
//! Construct a ParamStream wrapping a data stream with serialization parameters
//! needed to pass transaction objects between bitcoin processes.
//! In the future, more params may be added here to serialize other objects that
//! require serialization parameters. Params should just be chosen to serialize
//! objects completely and ensure that serializing and deserializing objects
//! with the specified parameters produces equivalent objects. It's also
//! harmless to specify serialization parameters here that are not used.
template <typename S>
auto Wrap(S& s)
{
    return ParamsStream{s, TX_WITH_WITNESS};
}

//! Detect if type has a deserialize_type constructor, which is
//! used to deserialize types like CTransaction that can't be unserialized into
//! existing objects because they are immutable.
||||||| parent of 8a20be71f07c (multiprocess: Add bitcoin-mine test program)
//! Use SFINAE to define Serializeable<T> trait which is true if type T has a
//! Serialize(stream) method, false otherwise.
=======
//! Construct a ParamStream wrapping data stream with serialization parameters
//! needed to pass transaction and address objects between bitcoin processes.
template<typename S>
auto Wrap(S& s)
{
    return ParamsStream{s, TX_WITH_WITNESS, CAddress::V2_NETWORK};
}

//! Serialize bitcoin value.
template <typename T>
DataStream Serialize(const T& value)
{
    DataStream stream;
    auto wrapper{Wrap(stream)};
    value.Serialize(wrapper);
    return stream;
}

//! Deserialize bitcoin value.
template <typename T>
T Unserialize(const kj::ArrayPtr<const kj::byte>& data)
{
    SpanReader stream{{data.begin(), data.end()}};
    T value;
    auto wrapper{Wrap(stream)};
    value.Unserialize(wrapper);
    return value;
}

//! Use SFINAE to define Serializeable<T> trait which is true if type T has a
//! Serialize(stream) method, false otherwise.
>>>>>>> 8a20be71f07c (multiprocess: Add bitcoin-mine test program)
template <typename T>
<<<<<<< HEAD
concept Deserializable = std::is_constructible_v<T, ::deserialize_type, ::DataStream&>;
||||||| parent of 8a20be71f07c (multiprocess: Add bitcoin-mine test program)
struct Serializable {
private:
    template <typename C>
    static std::true_type test(decltype(std::declval<C>().Serialize(std::declval<std::nullptr_t&>()))*);
    template <typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(nullptr))::value;
};

//! Use SFINAE to define Unserializeable<T> trait which is true if type T has
//! an Unserialize(stream) method, false otherwise.
template <typename T>
struct Unserializable {
private:
    template <typename C>
    static std::true_type test(decltype(std::declval<C>().Unserialize(std::declval<std::nullptr_t&>()))*);
    template <typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(nullptr))::value;
};
=======
struct Serializable {
private:
    template <typename C>
    static std::true_type test(decltype(std::declval<C>().Serialize(std::declval<std::nullptr_t&>()))*);
    template <typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(nullptr))::value;
};

//! Use SFINAE to define Unserializeable<T> trait which is true if type T has
//! an Unserialize(stream) method, false otherwise.
template <typename T>
struct Unserializable {
private:
    template <typename C>
    static std::true_type test(decltype(std::declval<C>().Unserialize(std::declval<std::nullptr_t&>()))*);
    template <typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(nullptr))::value;
};

//! Trait which is true if type has a deserialize_type constructor, which is
//! used to deserialize types like CTransaction that can't be unserialized into
//! existing objects because they are immutable.
template <typename T>
using Deserializable = std::is_constructible<T, ::deserialize_type, ::DataStream&>;
>>>>>>> 8a20be71f07c (multiprocess: Add bitcoin-mine test program)
} // namespace capnp
} // namespace ipc

//! Functions to serialize / deserialize common bitcoin types.
namespace mp {
//! Overload multiprocess library's CustomBuildField hook to allow any
//! serializable object to be stored in a capnproto Data field or passed to a
//! capnproto interface. Use Priority<1> so this hook has medium priority, and
//! higher priority hooks could take precedence over this one.
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Value&& value, Output&& output)
// Enable if serializeable and if LocalType is not cv or reference qualified. If
// LocalType is cv or reference qualified, it is important to fall back to
// lower-priority Priority<0> implementation of this function that strips cv
// references, to prevent this CustomBuildField overload from taking precedence
// over more narrow overloads for specific LocalTypes.
requires Serializable<LocalType, DataStream> && std::is_same_v<LocalType, std::remove_cv_t<std::remove_reference_t<LocalType>>>
{
    DataStream stream;
    auto wrapper{ipc::capnp::Wrap(stream)};
    value.Serialize(wrapper);
    auto result = output.init(stream.size());
    memcpy(result.begin(), stream.data(), stream.size());
}

//! Overload multiprocess library's CustomReadField hook to allow any object
//! with an Unserialize method to be read from a capnproto Data field or
//! returned from capnproto interface. Use Priority<1> so this hook has medium
//! priority, and higher priority hooks could take precedence over this one.
template <typename LocalType, typename Input, typename ReadDest>
<<<<<<< HEAD
decltype(auto) CustomReadField(TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest)
requires Unserializable<LocalType, DataStream> && (!ipc::capnp::Deserializable<LocalType>)
||||||| parent of 8a20be71f07c (multiprocess: Add bitcoin-mine test program)
decltype(auto)
CustomReadField(TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest,
                std::enable_if_t<ipc::capnp::Unserializable<LocalType>::value>* enable = nullptr)
=======
decltype(auto)
CustomReadField(TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest,
                std::enable_if_t<ipc::capnp::Unserializable<LocalType>::value &&
                                 !ipc::capnp::Deserializable<LocalType>::value>* enable = nullptr)
>>>>>>> 8a20be71f07c (multiprocess: Add bitcoin-mine test program)
{
    return read_dest.update([&](auto& value) {
        if (!input.has()) return;
        auto data = input.get();
        SpanReader stream({data.begin(), data.end()});
        auto wrapper{ipc::capnp::Wrap(stream)};
        value.Unserialize(wrapper);
    });
}

<<<<<<< HEAD
//! Overload multiprocess library's CustomReadField hook to allow any object
//! with a deserialize constructor to be read from a capnproto Data field or
//! returned from capnproto interface. Use Priority<1> so this hook has medium
//! priority, and higher priority hooks could take precedence over this one.
template <typename LocalType, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest)
requires ipc::capnp::Deserializable<LocalType>
{
    assert(input.has());
    auto data = input.get();
    SpanReader stream({data.begin(), data.end()});
    auto wrapper{ipc::capnp::Wrap(stream)};
    return read_dest.construct(::deserialize, wrapper);
}

//! Overload CustomBuildField and CustomReadField to serialize std::chrono
//! parameters and return values as numbers.
template <class Rep, class Period, typename Value, typename Output>
void CustomBuildField(TypeList<std::chrono::duration<Rep, Period>>, Priority<1>, InvokeContext& invoke_context, Value&& value,
                      Output&& output)
{
    static_assert(std::numeric_limits<decltype(output.get())>::lowest() <= std::numeric_limits<Rep>::lowest(),
                  "capnp type does not have enough range to hold lowest std::chrono::duration value");
    static_assert(std::numeric_limits<decltype(output.get())>::max() >= std::numeric_limits<Rep>::max(),
                  "capnp type does not have enough range to hold highest std::chrono::duration value");
    output.set(value.count());
}

template <class Rep, class Period, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::chrono::duration<Rep, Period>>, Priority<1>, InvokeContext& invoke_context,
                               Input&& input, ReadDest&& read_dest)
{
    return read_dest.construct(input.get());
}

//! Overload CustomBuildField and CustomReadField to serialize UniValue
//! parameters and return values as JSON strings.
||||||| parent of 8a20be71f07c (multiprocess: Add bitcoin-mine test program)
=======
//! Overload multiprocess library's CustomReadField hook to allow any object
//! with an deserialize constructor to be read from a capnproto Data field or
//! returned from canproto interface. Use Priority<1> so this hook has medium
//! priority, and higher priority hooks could take precedence over this one.
template <typename LocalType, typename Input, typename ReadDest>
decltype(auto)
CustomReadField(TypeList<LocalType>, Priority<1>, InvokeContext& invoke_context, Input&& input, ReadDest&& read_dest,
                std::enable_if_t<ipc::capnp::Deserializable<LocalType>::value>* enable = nullptr)
{
    assert(input.has());
    auto data = input.get();
    SpanReader stream({data.begin(), data.end()});
    // TODO: instead of always preferring Deserialize implementation over
    // Unserialize should prefer Deserializing when emplacing, unserialize when
    // updating. Can implement by adding read_dest.alreadyConstructed()
    // constexpr bool method in libmultiprocess.
    auto wrapper{ipc::capnp::Wrap(stream)};
    return read_dest.construct(deserialize, wrapper);
}

//! Overload CustomBuildField and CustomReadField to serialize UniValue
//! parameters and return values as JSON strings.
>>>>>>> 8a20be71f07c (multiprocess: Add bitcoin-mine test program)
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
<<<<<<< HEAD

//! Generic ::capnp::Data field builder for any C++ type that can be converted
//! to a span of bytes, like std::vector<char> or std::array<uint8_t>, or custom
//! blob types like uint256 or PKHash with data() and size() methods pointing to
//! bytes.
//!
//! Note: it might make sense to move this function into libmultiprocess, since
//! it is fairly generic. However this would require decreasing its priority so
//! it can be overridden, which would require more changes inside
//! libmultiprocess to avoid conflicting with the Priority<1> CustomBuildField
//! function it already provides for std::vector. Also, it might make sense to
//! provide a CustomReadField counterpart to this function, which could be
//! called to read C++ types that can be constructed from spans of bytes from
//! ::capnp::Data fields. But so far there hasn't been a need for this.
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<LocalType>, Priority<2>, InvokeContext& invoke_context, Value&& value, Output&& output)
requires
    (std::is_same_v<decltype(output.get()), ::capnp::Data::Builder>) &&
    (std::convertible_to<Value, std::span<const std::byte>> ||
     std::convertible_to<Value, std::span<const char>> ||
     std::convertible_to<Value, std::span<const unsigned char>> ||
     std::convertible_to<Value, std::span<const signed char>>)
{
    auto data = std::span{value};
    auto result = output.init(data.size());
    memcpy(result.begin(), data.data(), data.size());
}
||||||| parent of 8a20be71f07c (multiprocess: Add bitcoin-mine test program)
=======

//! Overload CustomBuildField and CustomReadField to serialize other types of
//! objects that have CustomBuildMessage and CustomReadMessage overloads.
//! Defining BuildMessage and ReadMessage overloads is simpler than defining
//! BuildField and ReadField overloads because these overloads can be normal
//! functions instead of template functions, and defined in a .cpp file instead
//! of a header.
template <typename LocalType, typename Value, typename Output>
void CustomBuildField(TypeList<LocalType>, Priority<2>, InvokeContext& invoke_context, Value&& value, Output&& output,
                      decltype(CustomBuildMessage(invoke_context, value, std::move(output.get())))* enable = nullptr)
{
    CustomBuildMessage(invoke_context, value, std::move(output.init()));
}

template <typename LocalType, typename Reader, typename ReadDest>
decltype(auto) CustomReadField(TypeList<LocalType>, Priority<2>, InvokeContext& invoke_context, Reader&& reader,
                               ReadDest&& read_dest,
                               decltype(CustomReadMessage(invoke_context, reader.get(),
                                                          std::declval<LocalType&>()))* enable = nullptr)
{
    return read_dest.update([&](auto& value) { if (reader.has()) CustomReadMessage(invoke_context, reader.get(), value); });
}

//! Generic ::capnp::Data field builder for any class that a Span can be
//! constructed from, particularly BaseHash and base_blob classes and
//! subclasses. It's also used to serialize vector<unsigned char> set elements
//! in GCSFilter::ElementSet and CBlockTemplate::vchCoinbaseCommitment.
//!
//! There is currently no corresponding ::capnp::Data CustomReadField function
//! that works using Spans, because the bitcoin classes in the codebase like
//! BaseHash and blob_blob that can converted /to/ Span don't currently have
//! Span constructors that allow them to be constructed /from/ Span. If they
//! did, it would simplify things. For example, a generic CustomReadField
//! function could be written that would allow dropping specialized
//! CustomReadField functions for types like PKHash.
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
    typename std::enable_if_t<!ipc::capnp::Serializable<typename std::remove_cv<
        typename std::remove_reference<Value>::type>::type>::value>* enable_not_serializable = nullptr,
    typename std::enable_if_t<std::is_same_v<decltype(output.get()), ::capnp::Data::Builder>>* enable_output =
        nullptr,
    decltype(Span{value})* enable_value = nullptr)
{
    auto data = Span{value};
    auto result = output.init(data.size());
    memcpy(result.begin(), data.data(), data.size());
}
<<<<<<< HEAD
>>>>>>> 8a20be71f07c (multiprocess: Add bitcoin-mine test program)
||||||| parent of 4e1a4342f3b2 (multiprocess: Expand mining interface)
=======

//! Overload CustomBuildField and CustomReadField to serialize std::chrono
//! parameters and return values as numbers.
//! TODO: Could add compile time checks to make sure types are compatible and
//! precision is not lost.
template <class Rep, class Period, typename Value, typename Output>
void CustomBuildField(TypeList<std::chrono::duration<Rep, Period>>, Priority<1>, InvokeContext& invoke_context, Value&& value,
                      Output&& output)
{
    output.set(value.count());
}

template <class Rep, class Period, typename Input, typename ReadDest>
decltype(auto) CustomReadField(TypeList<std::chrono::duration<Rep, Period>>, Priority<1>, InvokeContext& invoke_context,
                               Input&& input, ReadDest&& read_dest)
{
    return read_dest.construct(input.get());
}
>>>>>>> 4e1a4342f3b2 (multiprocess: Expand mining interface)
} // namespace mp

#endif // BITCOIN_IPC_CAPNP_COMMON_TYPES_H
