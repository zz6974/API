#pragma once
#include <optional>
#include <string>
#include <utility>
#include "api/common/ErrorCode.h"

namespace api::common {

/**
 * @brief 统一结果类型
 *
 * 业务层不抛异常穿越层边界，一切以 Result 传递。
 * 成功携带返回值，失败携带错误码与描述。
 * @tparam T 成功时的返回值类型，void 有专门特化
 */
template <typename T>
class Result {
public:
    /**
     * @brief 成功构造
     * @param value 返回值
     * @return 携带 value 的成功结果
     */
    static Result Ok(T value) {
        Result r;
        r.ok_ = true;
        r.value_ = std::move(value);
        return r;
    }
    /**
     * @brief 失败构造
     * @param code 错误码
     * @param message 错误描述（默认空串）
     * @return 携带错误码与描述的失败结果
     */
    static Result Fail(ErrorCode code, std::string message = {}) {
        Result r;
        r.ok_ = false;
        r.code_ = code;
        r.message_ = std::move(message);
        return r;
    }

    /// @brief 是否成功
    bool IsOk() const { return ok_; }
    /// @brief 是否失败
    bool IsFail() const { return !ok_; }
    /// @brief 隐式转换：可直接用于 if 判断
    explicit operator bool() const { return ok_; }

    /// @brief 错误码（仅失败时有效）
    ErrorCode Code() const { return code_; }
    /// @brief 错误描述（仅失败时有效）
    const std::string& Message() const { return message_; }

    /// @brief 取值（仅成功时可调用，否则抛 optional 异常）
    T& Value() { return value_.value(); }
    /// @brief 取值（仅成功时可调用，否则抛 optional 异常）
    const T& Value() const { return value_.value(); }
    /// @brief 移交值所有权（仅成功时可调用）
    T Take() { return std::move(value_.value()); }
    /// @brief 取值，失败时返回 fallback 兜底值
    const T& ValueOr(const T& fallback) const { return ok_ ? *value_ : fallback; }

private:
    Result() = default;              ///< 默认构造为失败状态，只能经 Ok/Fail 得到实例
    bool ok_ = false;                ///< 成功标志
    std::optional<T> value_;         ///< 成功值（失败时无值）
    ErrorCode code_ = ErrorCode::kOk; ///< 错误码
    std::string message_;            ///< 错误描述
};

/**
 * @brief Result 的 void 特化
 *
 * 无返回值操作的结果：只有成功/失败，无数据。
 */
template <>
class Result<void> {
public:
    /// @brief 成功构造
    static Result Ok() { return {}; }
    /**
     * @brief 失败构造
     * @param code 错误码
     * @param message 错误描述（默认空串）
     * @return 携带错误码与描述的失败结果
     */
    static Result Fail(ErrorCode code, std::string message = {}) {
        Result r;
        r.ok_ = false;
        r.code_ = code;
        r.message_ = std::move(message);
        return r;
    }

    /// @brief 是否成功
    bool IsOk() const { return ok_; }
    /// @brief 是否失败
    bool IsFail() const { return !ok_; }
    /// @brief 隐式转换：可直接用于 if 判断
    explicit operator bool() const { return ok_; }

    /// @brief 错误码（仅失败时有效）
    ErrorCode Code() const { return code_; }
    /// @brief 错误描述（仅失败时有效）
    const std::string& Message() const { return message_; }

private:
    Result() = default;              ///< 默认构造为成功状态
    bool ok_ = true;                 ///< 成功标志
    ErrorCode code_ = ErrorCode::kOk; ///< 错误码
    std::string message_;            ///< 错误描述
};

} // namespace api::common
