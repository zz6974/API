#include "UI/UIWindow.h"
#include "UI/IWindow.h"
#include <stdexcept>
#include <iostream>

#include "CCSystem.h"
#include "UI/InputEvent.h"
#include "UI/WindowEvent.h"

using namespace CTL;

static bool s_GlfwInitialized = false;

GlfwWindow::GlfwWindow(int width, int height, const std::string& title)
    : m_Width(width), m_Height(height) {

    if (!s_GlfwInitialized) {
        if (!glfwInit())
            throw std::runtime_error("GLFW init failed");
        s_GlfwInitialized = true;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_Handle)
        throw std::runtime_error("GLFW window creation failed");

    glfwMakeContextCurrent(m_Handle);
    glfwSetWindowUserPointer(m_Handle, this);

    glfwSetFramebufferSizeCallback(m_Handle, FramebufferResizeCallback);
    glfwSetKeyCallback(m_Handle, KeyCallbackDispatch);
    glfwSetWindowCloseCallback(m_Handle, CloseCallbackDispatch);
    m_Context = new GraphicsContext(m_Handle);
}

GlfwWindow::~GlfwWindow() {
    if (m_Handle) {
        glfwDestroyWindow(m_Handle);
        m_Handle = nullptr;
    }
    if (m_Context) {
        delete m_Context;
        m_Context = nullptr;
    }
}

// ---------------- 基础 ----------------
bool GlfwWindow::ShouldClose() const {
    return glfwWindowShouldClose(m_Handle);
}

void GlfwWindow::SwapBuffers() const {
    glfwSwapBuffers(m_Handle);
}

void GlfwWindow::PollEvents() const {
    glfwPollEvents();
}

int GlfwWindow::GetWidth() const { return m_Width; }

int GlfwWindow::GetHeight() const { return m_Height; }

// ---------------- 窗口操作 ----------------
void GlfwWindow::Move(int x, int y) {
    glfwSetWindowPos(m_Handle, x, y);
}

void GlfwWindow::Resize(int width, int height) {
    glfwSetWindowSize(m_Handle, width, height);
}

void GlfwWindow::Minimize() {
    glfwIconifyWindow(m_Handle);
}

void GlfwWindow::Maximize() {
    glfwMaximizeWindow(m_Handle);
}

void GlfwWindow::Restore() {
    glfwRestoreWindow(m_Handle);
}

void GlfwWindow::Close() {
    glfwSetWindowShouldClose(m_Handle, GLFW_TRUE);
}

void GlfwWindow::SetVSync(const bool enabled) {
    glfwSwapInterval(enabled ? 1 : 0);
}

bool GlfwWindow::IsVSync() const {
    return glfwGetWindowAttrib(m_Handle, GLFW_DOUBLEBUFFER);
}

float GlfwWindow::GetContentScale() const {
    float xscale, yscale;
    glfwGetWindowContentScale(m_Handle, &xscale, &yscale);
    return xscale;
}
// ---------------- 回调 ----------------
void GlfwWindow::SetKeyCallback(KeyCallback cb) {
    m_KeyCallback = cb;
}

void GlfwWindow::SetCloseCallback(CloseCallback cb) {
    m_CloseCallback = cb;
}

void GlfwWindow::SetEventCallback(const UIEventCallbackFn& cb) {
    m_EventCallback = cb;
}

GLFWwindow * GlfwWindow::GetHandle() const {
    return m_Handle;
}

// ---------------- 静态回调 ----------------
void GlfwWindow::FramebufferResizeCallback(GLFWwindow* window, int w, int h) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
    if (!self) return;

    self->m_Width = w;
    self->m_Height = h;

    if (self->m_EventCallback) {
        CTL::WindowResizeEvent e(static_cast<uint32_t>(w),
                           static_cast<uint32_t>(h));
        self->m_EventCallback(e);
    }
}

void GlfwWindow::KeyCallbackDispatch(GLFWwindow* window, int key, int, int action, int) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
    if (!self || !self->m_EventCallback) return;

    if (action == GLFW_PRESS) {
        KeyPressedEvent e(key);
        self->m_EventCallback(e);
    }
}

void GlfwWindow::CloseCallbackDispatch(GLFWwindow* window) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
    if (!self) return;

    if (self->m_CloseCallback) {
        self->m_CloseCallback();
    }

    if (self->m_EventCallback) {
        WindowCloseEvent e;
        self->m_EventCallback(e);
    }
}
