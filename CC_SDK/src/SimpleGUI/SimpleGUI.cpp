#include "SimpleGUI/SimpleGUI.h"
#include "CCSystem.h"

int CTL::SimpleGUI::RunApp(const AppConfig &config,const std::function<void()> &func) {
    int i = 0;
    CTL::GlfwWindow window(config.width, config.height, config.title);
    window.SetVSync(true);

    window.SetEventCallback([](CTL::Event& e) {
        if (e.GetType() == CTL::EventType::WindowResize) {
            auto& re = static_cast<CTL::WindowResizeEvent&>(e);
            glViewport(0, 0, re.Width, re.Height);
        }
    });

    TextRenderer textRenderer("./File/simfang.ttf",100,"你好世界Hello World！");
    while (!window.ShouldClose()) {
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        textRenderer.RenderText("你好", 10, 50, 13, 0.0f, 1.0f, 1.0f);

        if (func) {
            try {
                func();
            }
            catch (std::exception& e) {
                System::Println("GlfwWindow::RunApp func error: {}",e.what());
            }
        }

        window.SwapBuffers();
        window.PollEvents();
    }
    return i;
}
