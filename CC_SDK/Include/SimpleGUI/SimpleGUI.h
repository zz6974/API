#ifndef SRCCTL_SIMPLEGUI_H
#define SRCCTL_SIMPLEGUI_H

#include "UI/UIWindow.h"
#include "UI/WindowEvent.h"
#include "Components/Renderer.h"

namespace CTL {
    struct AppConfig {
        String title = "OpenGL Window";
        int width = 1280;
        int height = 720;
    };
    class SimpleGUI {
    public:
        static int RunApp(const AppConfig& config,const std::function<void()>& func);
    };
}

#endif
