#ifndef BITCOIN_INTERFACES_CAPNP_PROXY_DECL_H
#define BITCOIN_INTERFACES_CAPNP_PROXY_DECL_H

#include <interfaces/base.h>
#include <interfaces/capnp/util.h>
#include <util.h>

#include <capnp/blob.h> // for capnp::Text
#include <capnp/common.h>
#include <functional>
#include <kj/async.h>
#include <list>
#include <memory>

// FIXME Rename Impl->Class everywhere

namespace interfaces {
namespace capnp {

class AsyncThread;
class EventLoop;

using CleanupList = std::list<std::function<void()>>;
using CleanupIt = typename CleanupList::iterator;

//! Mapping from capnp interface type to proxy client implementation (specializations are generated by
//! proxy-codegen.cpp).
template <typename Interface>
struct ProxyClient;

//! Mapping from capnp interface type to proxy server implementation (specializations are generated by
//! proxy-codegen.cpp).
template <typename Interface>
struct ProxyServer;

//! Mapping from capnp method params type to method traits (specializations are generated by proxy-codegen.cpp).
template <typename Params>
struct ProxyMethod;

//! Mapping from capnp struct type to struct traits (specializations are generated by proxy-codegen.cpp).
template <typename Struct>
struct ProxyStruct;

//! Mapping from local c++ type to capnp type and traits (specializations are generated by proxy-codegen.cpp).
template <typename Type>
struct ProxyType;

//! Wrapper around std::function for passing std::function objects between client and servers.
// FIXME: Get rid of specialization, just make this take straight <Result, Args> template params.
template <typename Fn>
class ProxyCallback;

template <typename Result, typename... Args>
class ProxyCallback<std::function<Result(Args...)>> : public Base
{
public:
    virtual Result call(Args&&... args) = 0;
};

// FIXME: Move to -impl.h
template <typename Result, typename... Args>
class ProxyCallbackImpl : public ProxyCallback<std::function<Result(Args...)>>
{
    using Fn = std::function<Result(Args...)>;
    Fn m_fn;

public:
    ProxyCallbackImpl(Fn fn) : m_fn(std::move(fn)) {}
    Result call(Args&&... args) override { return m_fn(std::forward<Args>(args)...); }
};

template <typename Interface, typename Class>
class ProxyClientBase : public Class
{
public:
    ProxyClientBase(typename Interface::Client client, EventLoop& loop);
    // FIXME: should make cleanup and invoke standalone methods to ungoop header
    void cleanup(bool remote);
    ~ProxyClientBase() noexcept;

    // FIXME: Remove these overloads and need for subclasses to inherit. Just detect methods with sfinae
    template <typename... Args>
    void buildParams(Args&&...)
    {
    }

    template <typename... Args>
    void readResults(Args&&...)
    {
    }

    ProxyClient<Interface>& client() { return static_cast<ProxyClient<Interface>&>(*this); }

    typename Interface::Client m_client;
    EventLoop* m_loop;
    CleanupIt m_cleanup;
};

template <typename MethodTraits, typename GetRequest, typename ProxyClient, typename... _Params>
void clientInvoke(MethodTraits, const GetRequest& get_request, ProxyClient& proxy_client, _Params&&... params);

template <typename Interface, typename Class>
class ProxyServerBase : public Interface::Server
{
public:
    ProxyServerBase(Class* impl, bool owned, EventLoop& loop) : m_impl(impl), m_owned(owned), m_loop(&loop) {}
    ~ProxyServerBase()
    {
        if (m_owned) delete m_impl;
    }

    Class* m_impl;
    /**
     * Whether or not to delete native interface pointer when this capnp server
     * goes out of scope. This is true for servers created to wrap
     * unique_ptr<Impl> method arguments, but false for servers created to wrap
     * Impl& method arguments.
     *
     * In the case of Impl& arguments, custom code is required on other side of
     * the connection to delete the capnp client & server objects since native
     * code on that side of the connection will just be taking a plain reference
     * rather than a pointer, so won't be able to do its own cleanup. Right now
     * this is implemented with addCloseHook callbacks to delete clients at
     * appropriate times depending on semantics of the particular method being
     * wrapped. */
    bool m_owned;
    EventLoop* m_loop;

    template <typename Callback>
    std::future<bool> async(Callback&& callback)
    {
        return std::async(std::launch::async, std::move(callback));
    }
};

template <typename Interface, typename Class>
class ProxyServerCustom : public ProxyServerBase<Interface, Class>
{
    using ProxyServerBase<Interface, Class>::ProxyServerBase;
};

template <typename Interface, typename Class>
class ProxyClientCustom : public ProxyClientBase<Interface, Class>
{
    using ProxyClientBase<Interface, Class>::ProxyClientBase;
};

//! Function traits class.
// FIXME Get rid of Fields logic, should be consolidated and moved to implementation
// FIXME get rid of underscores
template <class Fn>
struct FunctionTraits;

template <class _Class, class _Result, class... _Params>
struct FunctionTraits<_Result (_Class::*const)(_Params...)>
{
    using Result = _Result;
    template <size_t N>
    using Param = typename std::tuple_element<N, std::tuple<_Params...>>::type;
    using Fields = typename std::conditional<std::is_same<void, Result>::value,
        TypeList<_Params...>,
        TypeList<_Params..., _Result>>::type;
};

template <class _Class, class _Result, class... _Params>
struct FunctionTraits<_Result (_Class::*)(_Params...)>
{
    using Result = _Result;
};

template <class _Class, class _Result, class... _Params>
struct FunctionTraits<_Result (_Class::*)(_Params...) const>
{
    using Result = _Result;
};

template <>
struct FunctionTraits<std::nullptr_t>
{
    using Result = std::nullptr_t;
};

//! Specializable
template <typename Params>
struct ProxyMethodTraits : public FunctionTraits<decltype(ProxyMethod<Params>::method)>
{
};

// Create or update field value using reader.
// template <typename ParamTypes, typename Proxy, typename Reader, typename... Values>
// void ReadField(LocalTypes, Proxy& proxy, Reader&& reader, Values&&... values);

// Read field value using reader, then call fn(fn_params..., field_value_params...),
// then update field value using builder. Skip updating field value if argument
// is input-only, and skip reading field value if argument is output-only.
// template <typename CapTypes, typename ParamTypes, typename Reader, typename Builder, typename Fn, typename...
// FnParams>
// booln bbsField(CapTypes, ParamTypes, Proxy& proxy, Input&& input, Output&& output, Fn&& fn, FnParams&&...
// fn_params);

template <typename CapValue, typename Enable = void>
struct CapValueTraits
{
    using CapType = CapValue;
};

template <typename CapValue>
struct CapValueTraits<CapValue, Void<typename CapValue::Reads>>
{
    using CapType = typename CapValue::Reads;
};

#if 0
template<typename CapValue>
struct CapValueTraits<CapValue, Void<typename CapValue::Builds>> {
    using CapType = typename CapValue::Builds;
};
#endif

template <>
struct CapValueTraits<::capnp::Text::Builder, void>
{
    // Workaround for missing Builds typedef in capnp 0.?? FIXME
    using CapType = ::capnp::Text;
};

template <typename Reader>
class ValueInput
{
public:
    constexpr static bool can_get = true;
    using CapType = typename CapValueTraits<Plain<Reader>>::CapType;

    ValueInput(Reader&& reader) : m_reader(std::forward<Reader>(reader)) {}
    Reader&& get() { return std::forward<Reader>(m_reader); }
    bool has() { return true; }
    Reader m_reader;
};

template <typename Reader>
ValueInput<Reader> MakeValueInput(Reader&& reader)
{
    return ValueInput<Reader>(std::forward<Reader>(reader));
}

// FIXME: Move to impl.h after buildfield priority arguments removed
template <int priority>
struct Priority : Priority<priority - 1>
{
};

template <>
struct Priority<0>
{
};

using BuildFieldPriority = Priority<3>;

// FIXME: probably move to impl.h after buildfield revamp
template <typename Setter, typename Enable = void>
struct CapSetterMethodTraits;

template <typename Builder, typename FieldType>
struct CapSetterMethodTraits<FieldType (Builder::*)(), void>
{
    using Type = FieldType;
    using CapType = typename Type::Builds;
    static constexpr bool pointer = true;
};

template <typename Builder, typename FieldType>
struct CapSetterMethodTraits<FieldType (Builder::*)(unsigned int),
    typename std::enable_if<!std::is_same<FieldType, void>::value>::type>
{
    using Type = FieldType;
    using CapType = typename CapValueTraits<Type>::CapType;
    static constexpr bool pointer = true;
};

template <typename Builder, typename FieldType>
struct CapSetterMethodTraits<void (Builder::*)(FieldType), void>
{
    using Type = FieldType;
    using CapType = FieldType;
    static constexpr bool pointer = false;
};

template <>
struct CapSetterMethodTraits<std::nullptr_t, void>
{
    using Type = void;
};

//! Call method given method pointer and object.
// FIXME Try to get rid of callmethod entirely by detecting nullptr_t setting in FieldOutput, other places
template <typename Result = void,
    typename Class,
    typename MethodResult,
    typename MethodClass,
    typename... MethodParams,
    typename... Params>
MethodResult CallMethod(Class& object, MethodResult (MethodClass::*method)(MethodParams...), Params&&... params)
{
    return (object.*method)(std::forward<Params>(params)...);
}

template <typename Result = void,
    typename Class,
    typename MethodResult,
    typename MethodClass,
    typename... MethodParams,
    typename... Params>
MethodResult CallMethod(Class& object, MethodResult (MethodClass::*method)(MethodParams...) const, Params&&... params)
{
    return (object.*method)(std::forward<Params>(params)...);
}

template <typename Result, typename Class, typename... Params>
Result CallMethod(Class&, std::nullptr_t, Params&&...)
{
    return Result();
}

// FIXME: Try to move to impl.h
template <typename Builder, typename Accessor>
struct FieldOutput
{
public:
    using Setter = decltype(std::declval<Accessor>().setter);
    using Type = Plain<typename CapSetterMethodTraits<Setter>::Type>;
    static constexpr bool can_set = !std::is_same<Setter, std::nullptr_t>::value;

    Builder& m_builder;
    const Accessor& m_accessor;

    FieldOutput(Builder& builder, const Accessor& accessor) : m_builder(builder), m_accessor(accessor) {}

    template <typename Result = void, typename... Params>
    auto set(Params&&... params) -> AUTO_DO_RETURN(CallMethod<bool>(this->m_builder, m_accessor.has_setter, true),
        CallMethod<Result>(this->m_builder, m_accessor.setter, std::forward<Params>(params)...))

        void setWant()
    {
        CallMethod<bool>(this->m_builder, m_accessor.want_setter, true);
    }
};

template <typename CapType, ::capnp::Kind = ::capnp::kind<CapType>()>
struct CapTypeTraits
{
    template <typename Class>
    using Setter = void (Class::*)(CapType);
};

template <typename CapType>
struct CapTypeTraits<CapType, ::capnp::Kind::BLOB>
{
    template <typename Class>
    using Setter = ::capnp::BuilderFor<CapType> (Class::*)(unsigned);
};

template <typename CapType>
struct CapTypeTraits<CapType, ::capnp::Kind::LIST>
{
    template <typename Class>
    using Setter = ::capnp::BuilderFor<CapType> (Class::*)(unsigned);
};

template <typename CapType>
struct CapTypeTraits<CapType, ::capnp::Kind::STRUCT>
{
    template <typename Class>
    using Setter = ::capnp::BuilderFor<CapType> (Class::*)();
};

// FIXME: probably move to impl.h after buildfield revamp
// Adapter to let BuildField overloads methods work set & init list elements as
// if they were fields of a struct. If BuildField is changed to use some kind of
// accessor class instead of calling method pointers, then then maybe this could
// go away or be simplified, because would no longer be a need to return
// ListOutput method pointers emulating capnp struct method pointers..
template <typename ListType>
struct ListOutput;

template <typename T, ::capnp::Kind kind>
struct ListOutput<::capnp::List<T, kind>>
{
    using List = ::capnp::List<T, kind>;
    using Type = ::capnp::BuilderFor<T>;
    static constexpr bool can_set = true;

    ListOutput(typename List::Builder& builder, size_t index) : m_builder(builder), m_index(index) {}

    template <typename Result = void, typename Value>
    typename std::enable_if<kind != ::capnp::Kind::BLOB && kind != ::capnp::Kind::STRUCT, Result>::type set(
        Value&& value)
    {
        m_builder.set(m_index, std::forward<Value>(value));
    }

    // FIXME: Drop result? Replace U::Builder with Type?
    template <typename Result = void, typename U = T>
    typename std::enable_if<kind == ::capnp::Kind::BLOB, typename U::Builder>::type set(size_t size)
    {
        return m_builder.init(m_index, size);
    }

    template <typename Result = void, typename U = T>
    typename std::enable_if<kind == ::capnp::Kind::STRUCT, typename U::Builder>::type set()
    {
        return m_builder[m_index];
    }

    typename List::Builder& m_builder;
    size_t m_index;
};

template <typename Getter,
    typename Setter,
    typename HasGetter,
    typename HasSetter,
    typename WantGetter,
    typename WantSetter>
struct Accessor
{
    Getter getter;
    Setter setter;
    HasGetter has_getter;
    HasSetter has_setter;
    WantGetter want_getter;
    WantSetter want_setter;
};

template <typename Getter,
    typename Setter,
    typename HasGetter,
    typename HasSetter,
    typename WantGetter,
    typename WantSetter>
Accessor<Getter, Setter, HasGetter, HasSetter, WantGetter, WantSetter> MakeAccessor(Getter getter,
    Setter setter,
    HasGetter has_getter,
    HasSetter has_setter,
    WantGetter want_getter,
    WantSetter want_setter)
{
    return {getter, setter, has_getter, has_setter, want_getter, want_setter};
}

} // namespace capnp
} // namespace interfaces

#endif // BITCOIN_INTERFACES_CAPNP_PROXY_DECL_H
