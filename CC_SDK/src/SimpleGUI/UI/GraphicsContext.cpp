#include "UI/GraphicsContext.h"
#include <stdexcept>

using namespace CTL;

GraphicsContext::GraphicsContext(GLFWwindow* window)
    : m_Window(window) {

    glfwMakeContextCurrent(m_Window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        throw std::runtime_error("Failed to load OpenGL");

    glViewport(0, 0, 1280, 720);
}

GraphicsContext::~GraphicsContext() {
    glfwMakeContextCurrent(nullptr);
}

void GraphicsContext::SwapBuffers() const {
    glfwSwapBuffers(m_Window);
}

float GraphicsContext::GetTime() {
    return static_cast<float>(glfwGetTime());
}
