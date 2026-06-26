#ifndef SRCCTL_GRAPHICS_CONTEXT_H
#define SRCCTL_GRAPHICS_CONTEXT_H

#include <glad/glad.h>
#include "glfw/glfw3.h"

namespace CTL {
    class GraphicsContext {
    public:
        explicit GraphicsContext(GLFWwindow* window);
        ~GraphicsContext();

        void SwapBuffers() const;
        static float GetTime() ;

    private:
        GLFWwindow* m_Window;
    };
}

#endif
