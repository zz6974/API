#ifndef SRCCTL_I_WINDOW_H
#define SRCCTL_I_WINDOW_H

#include <string>
#include <functional>
#include "UI/UIEvent.h"

namespace CTL {
    using UIEventCallbackFn = std::function<void(CTL::Event&)>;
    class IWindow {
    public:
        virtual ~IWindow() = default;

        // 基础
        virtual bool ShouldClose() const = 0;
        virtual void SwapBuffers() const = 0;
        virtual void PollEvents() const = 0;

        virtual int GetWidth() const = 0;
        virtual int GetHeight() const = 0;

        // 窗口操作
        virtual void Move(int x, int y) = 0;
        virtual void Resize(int width, int height) = 0;

        virtual void Minimize() = 0;
        virtual void Maximize() = 0;
        virtual void Restore() = 0;

        virtual void Close() = 0;

        // 回调
        using KeyCallback     = void(*)(int key, int action);
        using CloseCallback   = std::function<void()>;

        virtual void SetKeyCallback(KeyCallback cb) = 0;
        virtual void SetCloseCallback(CloseCallback cb) = 0;
        virtual void SetEventCallback(const UIEventCallbackFn& cb) = 0;
        virtual void SetVSync(bool enabled) = 0;
        virtual bool IsVSync() const = 0;
        virtual float GetContentScale() const = 0; // DPI
    };
}

#endif
