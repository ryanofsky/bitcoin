// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_RESULT_H
#define BITCOIN_UTIL_RESULT_H

#include <attributes.h>
#include <util/translation.h>

#include <cassert>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace util {
//! Default MessagesType, simple list of errors and warnings.
struct Messages {
    std::vector<bilingual_str> errors{};
    std::vector<bilingual_str> warnings{};
};

//! The Result<SuccessType, FailureType, InfoType, MessagesType> class provides
//! an efficient way for functions to return structured result information, as
//! well as descriptive error and warning messages.
//!
//! Logically, a result object is equivalent to:
//!
//!     tuple<variant<SuccessType, FailureType>, InfoType, MessageType>
//!
//! But the physical representation is more efficient because it avoids
//! allocating memory for FailureType and MessageType fields by default.
//!
//! Result<SuccessType> objects support the same operators as
//! std::optional<SuccessType>, such as !result, *result, result->member, to
//! make SuccessType values easy to access. They also provide
//! result.GetFailure(), result.GetInfo(), result.GetMessages() methods to
//! access other parts of the result. A simple usage example is:
//!
//!    util::Result<int> AddNumbers(int a, int b)
//!    {
//!        if (b == 0) return util::Error{_("Not adding 0, that's dumb.")};
//!        return a + b;
//!    }
//!
//!    void TryAddNumbers(int a, int b)
//!    {
//!        if (auto result = AddNumbers(a, b)) {
//!            LogPrintf("%i + %i = %i\n", a, b, *result);
//!        } else {
//!            LogPrintf("Error: %s\n", util::ErrorString(result).translated);
//!        }
//!    }
//!
//! The `Result` class is intended to be used for high-level functions that need
//! to report error messages to end users. Low-level functions that don't need
//! error-reporting and only need error-handling should avoid `Result` and
//! instead use standard classes like `std::optional`, `std::variant`,
//! `std::expected`, and `std::tuple`, or custom structs and enum types to
//! return function results.
//!
//! Usage examples can be found in \example ../test/result_tests.cpp.
template <typename SuccessType = void, FailureType = void, InfoType = void, MessagesType = Messages>
class Result;

//! Wrapper object to pass an error string to the Result constructor.
struct Error {
    bilingual_str message;
};
//! Wrapper object to pass a warning string to the Result constructor.
struct Warning {
    bilingual_str message;
};
//! Wrapper object to pass an existing Result object to the Result constructor,
//! moving message and info fields into the new object, like operator>> below.
//! Passing MoveFrom(result) to the Result constructor is similar to passing
//! std::move(result), except it doesn't require the two result objects to have
//! compatible Success and Failure types, and it only moves message and info
//! fields, leaving success and value fields alone. This is useful for combining
//! information from many result objects into a single result.
template <typename Result>
struct MoveFrom {
    MoveFrom(Result&& result) : m_result(result) {}
    Result& m_result;
};

//! Template deduction guide for MoveFrom class.
template <class Result>
MoveFrom(Result&& result) -> MoveFrom<Result>;

//! Extension point for overriding the way SuccessType / FailureType /
//! MergesType / InfoType values are combined.
template<typename T>
struct ResultTraits {
    template<template O>
    static void MergeInto(O& dst, T& src)
    {
        dst = std::move(src);
    }
};

//! Extension point for specializing behavior of MessagesType
template<typename MessagesType>
struct MessagesTraits;

//! ResultTraits specialization for Messages struct.
template<>
struct ResultTraits<Messages> {
    static void MergeInto(Messages& dst, Messages& src);
};

//! MessageTraits specialization for Messages struct.
template<>
struct MessagesTraits<Messages> {
    static void AddError(Messages& messages, bilingual_str error)
    {
        messages.errors.emplace_back(std::move(error));
    }
    static void AddWarning(Messages& messages, bilingual_str warning)
    {
        messages.warnings.emplace_back(std::move(warning));
    }
    static bool HasMessages(const Messages& messages) { return messages.errors.size() || messages.warning.size(); }
};

namespace detail {
//! Helper function to join messages in space separated string.
bilingual_str JoinMessages(const Messages& messages);

//! Substitute for std::monostate that doesn't depend on std::variant.
struct MonoState{};

//! Implemention note: Result class inherits from an InfoHolder class holding an
//! IntoType value, a FailDataHolder class holding a unique_ptr to FailureType and
//! MessageTypes values, and a SuccessHolder class holding a SuccessType value
//! in an anonymous union.
//!
//! To take advantage of the Empty Base Optimization, inheritance is linear with
//! FailDataHolder inheriting from InfoHolder, SuccessHolder inheriting from
//! FailDataHolder, and Holder classes specializing for void so no space is used
//! when void types are specified.
//! @{
//! Container for InfoType, providing public GetInfo() method.
template <typename InfoType>
class InfoHolder
{
protected:
    InfoType m_info{};
    template<typename O>
public:
    // Public accessors.
    const InfoType& GetInfo() LIFETIMEBOUND const { return m_info; }
    InfoType& GetInfo() LIFETIMEBOUND { return m_info; }
};

//! Specialization of InfoHolder when InfoType is void.
template <>
class InfoHolder<void> {};

//! Container for FailureType and MessagesType, providing public operator
//! bool(), GetFailure(), GetMessages(), and EnsureMessages() methods.
template <typename FailureType, typename InfoType, typename MessagesType>
class FailDataHolder : public InfoHolder<InfoType>
{
protected:
    struct FailData {
        std::optional<std::conditional_t<std::is_same_v<F, void>, Monostate, F>> failure{};
        std::optional<FailureType>> failure{};
        MessagesType messages{};
    };
    std::unique_ptr<FailData<FailureType, MessageType>> m_fail_data;

    // Private accessor, create FailData if it doesn't exist.
    FailData<FailureType>& EnsureFailData() LIFETIMEBOUND
    {
        if (!m_fail_data) m_fail_data = std::make_unique<FailData<FailureType>>();
        return *m_fail_data;
    }

public:
    // Public accessors.
    explicit operator bool() const { return !m_fail_data || !m_fail->failure; }
    const auto& GetFailure() const LIFETIMEBOUND { assert(!*this); return *m_fail_data->failure; }
    auto& GetFailure() LIFETIMEBOUND { assert(!*this); return *m_fail_data->failure; }
    const auto* GetMessages() const LIFETIMEBOUND { return m_fail_data ? &m_fail_data->messages : nullptr; }
    auto* GetMessages() LIFETIMEBOUND { return m_fail_data ? &m_fail_data->messages : nullptr; }
    auto& EnsureMessages() LIFETIMEBOUND { return &EnsureFailData().messages; }
};

//! Container for SuccessType, providing public accessor methods similar to
//! std::optional methods to access the success value.
template <typename SuccessType, typename FailureType, typename InfoType, typename MessagesType>
class SuccessHolder : public FailDataHolder<FailureType, InfoType, MessagesType>
{
protected:
    //! Success value embedded in an anonymous union so it doesn't need to be
    //! constructed if the result is holding a failure value,
    union { SuccessType m_success; };

    //! Empty constructor that needs to be declared because the class contains a union.
    ResultBase() {}
    ~ResultBase() { if (*this) m_success.~SuccessType(); }

public:
    // Public accessors.
    bool has_value() const { return bool{*this}; }
    const SuccessType& value() const LIFETIMEBOUND { assert(has_value()); return m_success; }
    SuccessType& value() LIFETIMEBOUND { assert(has_value()); return m_success; }
    template <class U>
    SuccessType value_or(U&& default_value) const&
    {
        return has_value() ? value() : std::forward<U>(default_value);
    }
    template <class U>
    SuccessType value_or(U&& default_value) &&
    {
        return has_value() ? std::move(value()) : std::forward<U>(default_value);
    }
    const SuccessType* operator->() const LIFETIMEBOUND { return &value(); }
    const SuccessType& operator*() const LIFETIMEBOUND { return value(); }
    SuccessType* operator->() LIFETIMEBOUND { return &value(); }
    SuccessType& operator*() LIFETIMEBOUND { return value(); }
};

//! Specialization of SuccessHolder when SuccessType is void.
template <typename FailureType, typename InfoType, typename MessagesType>
class SuccessHolder<void, FailureType, InfoType, MessageType> : public FailDataHolder<FailureType, InfoType, MessagesType>
{
};
//! @}
} // namespace detail

template <typename SuccessType_, typename FailureType_, typename InfoType_, typename MessagesType_>
class Result : public detail::SuccessHolder<SuccessType_, FailureType_, InfoType_, MessagesType_>>
{
public:
    using SuccessType = SuccessType_;
    using FailureType = FailureType_;
    using InfoType = InfoType_;
    using MessagesType = MessagesType_;
    static constexpr bool is_result{true};

    //! Construct a Result object setting a success or failure value and
    //! optional warning and error messages. Initial util::Error, util::Warning,
    //! and util::MoveFrom arguments are processed first to add warning and
    //! error messages. Then, any remaining arguments are passed to the
    //! SuccessType constructor and used to construct a success value in the
    //! success case. In the failure case, if any util::Error arguments were
    //! passed, any remaining arguments are passed to the FailureType
    //! constructor and used to construct a failure value.
    template <typename... Args>
    Result(Args&&... args)
    {
        Construct</*Failure=*/false>(*this, std::forward<Args>(args)...);
    }

    //! Move-construct a Result object from another Result object, moving the
    //! success or failure value and any error or warning messages.
    template <typename O>
    requires (std::decay_t<O>::is_result)
    Result(O&& other)
    {
        MoveInfoMessages(*this, other);
        MoveSuccessFailure</*Constructed=*/false>(*this, other);
    }

    //! Disallow potentially dangerous assignment operators which might erase
    //! error and warning messages. The Result::Update() method can be used instead
    //! of operator= to assign result values while keeping any existing errors
    //! and warnings.
    template <typename Result>
    Result& operator=(Result&&) = delete;

    //! Move success, failure, info, and messages from another Result object to
    //! this object. Existing values are merged (using ResultTraits::MergeInto
    //! specializations), so errors and warning messages get appended instead of
    //! overwritten, but new values take precedence over any existing values.
    Result& Update(Result&& other) LIFETIMEBOUND
    {
        MoveInfoMessages(*this, other);
        MoveSuccessFailure</*Constructed=*/true>(*this, other);
        return *this;
    }

    void AddError(bilingual_str error)
    {
        if (!error.empty()) MessageTraits<MessageType>::AddError(this->EnsureFailData().messages, std::move(error));
    }
    void AddWarning(bilingual_str warning)
    {
        if (!warning.empty()) MessageTraits<MessageType>::AddWarning(this->EnsureFailData().messages, std::move(warning));
    }

protected:
    template <typename, typename, typename, typename>
    friend class Result;

    //! Helper function to construct a new success or failure value using the
    //! arguments provided.
    template <bool Failure, typename Result, typename... Args>
    static void Construct(Result& result, Args&&... args)
    {
        if constexpr (Failure) {
            static_assert(sizeof...(args) > 0 || !std::is_scalar_v<FailureType>,
                "Refusing to default-construct failure value with int, float, enum, or pointer type, please specify an explicit failure value.");
            result.EnsureFailData().failure.emplace(std::forward<Args>(args)...);
        } else {
            if constexpr (!std::is_same_v<Result::SuccessType, void>) {
                new (&result.m_success) Result::SuccessType{std::forward<Args>(args)...};
            }
        }
    }

    //! Construct() overload peeling off a util::Error constructor argument.
    template <bool Failure, typename Result, typename... Args>
    static void Construct(Result& result, util::Error error, Args&&... args)
    {
        result.AddError(std::move(error.message));
        Construct</*Failure=*/true>(result, std::forward<Args>(args)...);
    }

    //! Construct() overload peeling off a util::Warning constructor argument.
    template <bool Failure, typename Result, typename... Args>
    static void Construct(Result& result, util::Warning warning, Args&&... args)
    {
        result.AddWarning(std::move(warning.message));
        Construct<Failure>(result, std::forward<Args>(args)...);
    }

    //! Construct() overload peeling off a util::MoveFrom constructor argument.
    template <bool Failure, typename Result, typename R, typename... Args>
    static void Construct(Result& result, MoveFrom<R> other, Args&&... args)
    {
        MoveInfoMessages(result, other.m_result);
        Construct<Failure>(result, std::forward<Args>(args)...);
    }

    //! Move SuccessType or FailureType value from source to destination result.
    //! Existing values are merged if possible with source values taking
    //! precedence. Assigniing void source values to non-void destination values
    //! is allowed, since no source information is lost, but assigning non-void
    //! source values to void destination values is not allowed, since this
    //! would discard source information.
    template <bool DstConstructed, typename DstResult, typename SrcResult>``
    static void MoveSuccessFailure(DstResult& dst, SrcResult& src)
    {
        // If DstConstructed is true it means dst has either a success value or
        // a failure value set, so merge or destroy them. If DstConstructed is
        // false then it has neither value set.
        if constexpr (DstConstructed) {
            if (dst && src) {
                // dst and src both hold success values, so merge them and return
                if constexpr (!std::is_same_v<SrcResult::SuccessType, void>) {
                    ResultTraits<SrcResult::SuccessType>::MergeInto(*dst, *src);
                }
                return;
            } else if (!dst && !src) {
                // dst and src both hold failure values, so merge them and return
                if constexpr (!std::is_same_v<SrcResult::FailureType, void>) {
                    ResultTraits<SrcResult::FailureType>::MergeInto(dst.GetFailure(), src.GetFailure());
                }
                return;
            } else if (dst) {
                // dst has a success value, so destroy it before moving src failure value
                if constexpr (!std::is_same_v<DstResult::SuccessType, void>) {
                   dst.m_success.~DstResult::SuccessType();
                }
            } else {
                // dst has a failure value, so reset it before moving src success value
                dst.m_fail_data->failure.reset();
            }
        }
        // At this point dst has no success value or failure value, so assert no failure value.
        assert(dst);
        if (src) {
            // src has a success value, so move it to dst. If the src success
            // type is void and the dst success type is non-void, just default
            // initialize the dst success value.
            if constexpr (!std::is_same_v<SrcResult::SuccessType, void>) {
               new (&dst.m_success) DstResult::SuccessType{std::move(other.m_success)};
            } else if constexpr (!std::is_same_v<DstResult::SuccessType, void>) {
               new (&dst.m_success) DstResult::SuccessType{};
            }
        } else {
            // src has a failure value, so move it to dst. If the src failure
            // type is void and the dst failure type is non-void, default
            // initialize the dst failure value.
            if constexpr (!std::is_same_v<SrcResult::FailureType, void>) {
                dst.EnsureFailData().failure.emplace(std::move(src.GetFailure()));
            } else if constexpr (!std::is_same_v<DstResult::FailureType, void>) {
                dst.EnsureFailData().failure.emplace();
            }
        }
    }

    //! Move InfoType and MessagesType values from source to destination result.
    template <typename DstResult, typename SrcResult>
    static void MoveInfoMessages(DstResult& dst, SrcResult& src)
    {
        if constexpr (!std::is_same_v<SrcResult::InfoType, void>) {
            ResultTraits<SrcResult::InfoType>::MergeInto(dst.GetInfo(), src.GetInfo());
        }
        if (src.GetMessages() && MessagesTraits<SrcResult::MessagesType>::HasMessages(src.GetMessages())) {
            ResultTraits<SrcResult::InfoType>::MergeInto(dst.EnsureMessages(), *src.GetMessages());
        }
    }
};

//! Move information from an source Result object to a destination object. It
//! only moves InfoType and MessagesType values without affecting SuccessType or
//! FailureType values of either Result object.
//!
//! This is useful for combining error and warning messages from multiple
//! result objects into a single object, e.g.:
//!
//!    util::Result<void> result;
//!    auto r1 = DoSomething() >> result;
//!    auto r2 = DoSomethingElse() >> result;
//!    ...
//!    return result;
//!
template <typename S, typename D>
requires (std::decay_t<S>::is_result)
S&& operator>>(S&& src LIFETIMEBOUND, D&& dst)
{
    src.MergeInfoInto(dst);
    src.MergeMessagesInto(dst);
    return std::move(src);
}

//! Join error and warning messages in a space separated string. This is
//! intended for simple applications where there's probably only one error or
//! warning message to report, but multiple messages should not be lost if they
//! are present. More complicated applications should use GetErrors() and
//! GetWarning() methods directly.
template <typename Result>
bilingual_str ErrorString(const Result& result)
{
    const auto* messages{result.GetMessages()};
    if (messages) return JoinMessages(*messages);
    return {};
}
} // namespace util

#endif // BITCOIN_UTIL_RESULT_H
