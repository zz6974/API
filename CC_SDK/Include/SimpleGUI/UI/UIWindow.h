#ifndef SRCCTL_UI_WINDOW_H
#define SRCCTL_UI_WINDOW_H

#include "IWindow.h"
#include "GraphicsContext.h"
#include <GLFW/glfw3.h>
#include <functional>

#include "CCString.h"


namespace CTL {
    class GlfwWindow final : public IWindow {
    public:
        GlfwWindow(int width, int height, const std::string& title);
        ~GlfwWindow();

        GlfwWindow(const GlfwWindow&) = delete;
        GlfwWindow& operator=(const GlfwWindow&) = delete;

        // IWindow
        bool ShouldClose() const override;
        void SwapBuffers() const override;
        void PollEvents() const override;

        int GetWidth() const override;
        int GetHeight() const override;

        void Move(int x, int y) override;
        void Resize(int width, int height) override;

        void Minimize() override;
        void Maximize() override;
        void Restore() override;
        void Close() override;
        void SetVSync(bool enabled) override;
        bool IsVSync() const override;
        float GetContentScale() const override;

        void SetKeyCallback(KeyCallback cb) override;
        void SetCloseCallback(CloseCallback cb) override;
        void SetEventCallback(const UIEventCallbackFn &cb) override;
        GLFWwindow* GetHandle() const;
    private:
        static void FramebufferResizeCallback(GLFWwindow* window, int w, int h);
        static void KeyCallbackDispatch(GLFWwindow*, int key, int, int action, int);
        static void CloseCallbackDispatch(GLFWwindow* window);

    private:
        GLFWwindow* m_Handle = nullptr;
        int m_Width{};
        int m_Height{};
        GraphicsContext* m_Context = nullptr;

        KeyCallback m_KeyCallback = nullptr;
        CloseCallback m_CloseCallback = nullptr;
        UIEventCallbackFn m_EventCallback = nullptr;
    };
}

#endif
