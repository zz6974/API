#pragma once
#include <optional>
#include <string>
#include <utility>
#include "api/common/ErrorCode.h"

namespace api::common {

// 统一结果类型：业务层不抛异常穿越层边界，一切以 Result 传递
template <typename T>
class Result {
public:
    static Result Ok(T value) {
        Result r;
        r.ok_ = true;
        r.value_ = std::move(value);
        return r;
    }
    static Result Fail(ErrorCode code, std::string message = {}) {
        Result r;
        r.ok_ = false;
        r.code_ = code;
        r.message_ = std::move(message);
        return r;
    }

    bool IsOk() const { return ok_; }
    bool IsFail() const { return !ok_; }
    explicit operator bool() const { return ok_; }

    ErrorCode Code() const { return code_; }
    const std::string& Message() const { return message_; }

    T& Value() { return value_.value(); }
    const T& Value() const { return value_.value(); }
    T Take() { return std::move(value_.value()); }
    const T& ValueOr(const T& fallback) const { return ok_ ? *value_ : fallback; }

private:
    Result() = default;
    bool ok_ = false;
    std::optional<T> value_;
    ErrorCode code_ = ErrorCode::kOk;
    std::string message_;
};

template <>
class Result<void> {
public:
    static Result Ok() { return {}; }
    static Result Fail(ErrorCode code, std::string message = {}) {
        Result r;
        r.ok_ = false;
        r.code_ = code;
        r.message_ = std::move(message);
        return r;
    }

    bool IsOk() const { return ok_; }
    bool IsFail() const { return !ok_; }
    explicit operator bool() const { return ok_; }

    ErrorCode Code() const { return code_; }
    const std::string& Message() const { return message_; }

private:
    Result() = default;
    bool ok_ = true;
    ErrorCode code_ = ErrorCode::kOk;
    std::string message_;
};

} // namespace api::common
