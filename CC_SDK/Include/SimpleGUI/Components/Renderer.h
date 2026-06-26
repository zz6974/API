#pragma once
#ifndef SRCCTL_RENDERER_H
#define SRCCTL_RENDERER_H


#include <string>
#include <unordered_map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#endif



namespace CTL {

    // 前向声明stb库中的结构体
    // struct stbtt_packedchar;
    struct CharInfo {
        unsigned short x0,y0,x1,y1; // coordinates of bbox in bitmap
        float xoff,yoff,xadvance;
        float xoff2,yoff2;
    };

    class TextRenderer {
    public:
        // fontPath: ttf 文件路径
        // baseFontSize: 生成 SDF 纹理时使用的基准像素大小（如 64）
        // supportedChars: 需要支持的字符（UTF-8 编码），若为空则默认包含 ASCII 32-126
        TextRenderer(const char* fontPath, float baseFontSize,
                        const std::string& supportedChars = "");
        ~TextRenderer();

        // 渲染文本
        // targetFontSize: 希望在屏幕上显示的字体像素大小
        void RenderText(const std::string& text, float x, float y,
                        float targetFontSize, float r, float g, float b);

        // 计算文本在指定字号下的像素宽度
        float GetTextWidth(const std::string& text, float targetFontSize);

    private:
        void SetupBuffersAndShader();
        void BuildSdfAtlas(const std::vector<uint32_t>& codepoints,
                           unsigned char* ttfBuffer, int ttfSize);

        GLuint textureID = 0;
        GLuint VAO = 0, VBO = 0;
        GLuint shaderProgram = 0;

        // 从码点映射到字符在图集中的数据
        std::unordered_map<uint32_t, CharInfo> charMap;

        int atlasWidth = 1024;      // 图集尺寸（可按需增大）
        int atlasHeight = 1024;
        float baseFontSize;         // 基准像素高度
        bool ready = false;         // 是否初始化成功
    };

}

#endif
