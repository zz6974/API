#ifndef STREAM_HANDLER_H
#define STREAM_HANDLER_H

#include <iosfwd>
#include <iostream>
#include <sstream>

#ifdef __OHOS__
#define CC_OHOS_ 1
#include "hilog/log.h"
#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0x0728  // 全局domain宏，标识业务领域
#define LOG_TAG "CC_SDK"   // 全局tag宏，标识模块日志tag
#endif

class StreamHandler {
public:
    // 构造函数
    explicit StreamHandler(std::ostream& stream) : os(stream) {}
    // 重载 << 操作符，处理输入值
    template <typename T>
    StreamHandler& operator<<(const T& value) {
        os << value;
        return *this;
    }
    // 专门处理函数触发逻辑
    StreamHandler& operator<<(std::ostream& (*func)(std::ostream&)) {
        func(os);  // 调用传入的函数
        return *this;
    }
private:
    std::ostream& os;
};
static StreamHandler customCout(std::cout);

#endif
