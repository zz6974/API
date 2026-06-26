#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#include "SimpleGUI/Components/Renderer.h"
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <vector>

using namespace CTL;

// =============================================
//  工具函数：UTF-8 解码与码点收集
// =============================================
static uint32_t DecodeUtf8(const char*& it) {
    unsigned char c = *it;
    if (c < 0x80) { ++it; return c; }
    else if (c < 0xC0) { ++it; return 0xFFFD; }
    else if (c < 0xE0) {
        uint32_t cp = (c & 0x1F) << 6;
        c = *(++it); if ((c & 0xC0) != 0x80) return 0xFFFD; cp |= (c & 0x3F);
        ++it; return cp;
    } else if (c < 0xF0) {
        uint32_t cp = (c & 0x0F) << 12;
        c = *(++it); if ((c & 0xC0) != 0x80) return 0xFFFD; cp |= (c & 0x3F) << 6;
        c = *(++it); if ((c & 0xC0) != 0x80) return 0xFFFD; cp |= (c & 0x3F);
        ++it; return cp;
    } else {
        uint32_t cp = (c & 0x07) << 18;
        c = *(++it); if ((c & 0xC0) != 0x80) return 0xFFFD; cp |= (c & 0x3F) << 12;
        c = *(++it); if ((c & 0xC0) != 0x80) return 0xFFFD; cp |= (c & 0x3F) << 6;
        c = *(++it); if ((c & 0xC0) != 0x80) return 0xFFFD; cp |= (c & 0x3F);
        ++it; return cp;
    }
}

static std::vector<uint32_t> CollectUniqueCodepoints(const std::string& utf8str) {
    std::unordered_set<uint32_t> set;
    const char* p = utf8str.c_str();
    while (*p) {
        uint32_t cp = DecodeUtf8(p);
        if (cp != 0xFFFD) set.insert(cp);
    }
    return {set.begin(), set.end()};
}

// =============================================
//  着色器源码
// =============================================
static const char* vertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
uniform vec2 uScreenSize;

void main() {
    float xpos = (aPos.x / uScreenSize.x) * 2.0 - 1.0;
    float ypos = 1.0 - (aPos.y / uScreenSize.y) * 2.0;
    gl_Position = vec4(xpos, ypos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

static const char* fragmentShaderSrc = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTextTexture;
uniform vec3 uTextColor;
uniform float uScale;           // 当前目标字号 / 基准字号
uniform float uSmoothing;       // 边缘平滑度（通常 0.25/基准字号 较合适）

void main() {
    float distance = texture(uTextTexture, TexCoord).r;
    // 平滑区间映射到屏幕像素
    float smoothing = uSmoothing / uScale;
    float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
    FragColor = vec4(uTextColor, alpha);
}
)";

// =============================================
//  构造函数
// =============================================
TextRenderer::TextRenderer(const char* fontPath, float baseFontSize,
                                 const std::string& supportedChars)
    : baseFontSize(baseFontSize)
{
    // 1. 读取字体文件
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "无法打开字体文件: " << fontPath << std::endl;
        return;
    }
    size_t fileSize = file.tellg();
    file.seekg(0);
    std::vector<unsigned char> ttfBuffer(fileSize);
    file.read(reinterpret_cast<char*>(ttfBuffer.data()), fileSize);

    // 2. 确定要打包的码点
    std::vector<uint32_t> codepoints = CollectUniqueCodepoints(supportedChars);
    // 至少包含 ASCII 32-126
    for (uint32_t c = 32; c < 127; ++c) {
        if (std::find(codepoints.begin(), codepoints.end(), c) == codepoints.end())
            codepoints.push_back(c);
    }

    // 3. 构建 SDF 图集
    BuildSdfAtlas(codepoints, ttfBuffer.data(), (int)fileSize);

    // 4. 创建 OpenGL 资源
    SetupBuffersAndShader();
    ready = true;
}

// =============================================
//  核心：生成 SDF 图集
// =============================================
void TextRenderer::BuildSdfAtlas(const std::vector<uint32_t>& codepoints,
                                    unsigned char* ttfBuffer, int ttfSize)
{
    // 初始化字体信息
    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, ttfBuffer, 0)) {
        std::cerr << "字体初始化失败" << std::endl;
        return;
    }

    float scale = stbtt_ScaleForPixelHeight(&font, baseFontSize);
    const int padding = 4;                 // 距离场边缘扩展
    const int onEdgeValue = 128;           // 0.5 映射为 128
    const float pixelDistScale = 1.0f;     // 距离场精度

    // 临时存储每个字符的 SDF 位图及其度量信息
    struct TempGlyph {
        int w, h;
        std::vector<unsigned char> bitmap;
        int xoff, yoff;
        float advance;
    };
    std::vector<TempGlyph> tempGlyphs(codepoints.size());

    // 为每个码点生成 SDF
    for (size_t i = 0; i < codepoints.size(); ++i) {
        uint32_t cp = codepoints[i];
        int glyphIndex = stbtt_FindGlyphIndex(&font, cp);
        if (glyphIndex == 0) {
            // 字体中无此字符，用占位符（空格大小）
            tempGlyphs[i].w = 0; tempGlyphs[i].h = 0;
            tempGlyphs[i].advance = 0.0f;
            continue;
        }

        int w, h, xoff, yoff;
        unsigned char* sdf = stbtt_GetGlyphSDF(&font, scale, glyphIndex,
                                               padding, onEdgeValue,
                                               pixelDistScale, &w, &h,
                                               &xoff, &yoff);
        if (sdf) {
            tempGlyphs[i].w = w;
            tempGlyphs[i].h = h;
            tempGlyphs[i].bitmap.assign(sdf, sdf + w * h);
            tempGlyphs[i].xoff = xoff;
            tempGlyphs[i].yoff = yoff;

            // 获取字形的水平步进
            int advance, lsb;
            stbtt_GetGlyphHMetrics(&font, glyphIndex, &advance, &lsb);
            tempGlyphs[i].advance = advance * scale;

            stbtt_FreeSDF(sdf, nullptr);
        } else {
            // 生成失败（极少发生）
            tempGlyphs[i].w = 0; tempGlyphs[i].h = 0;
            tempGlyphs[i].advance = 0.0f;
        }
    }

    // 准备矩形打包
    // stbtt_pack_context pc;
    std::vector<unsigned char> atlasPixels(atlasWidth * atlasHeight, 0);
    std::vector<stbrp_node> nodes(atlasWidth);   // 打包需要的临时节点
    stbrp_context rpCtx;

    stbrp_init_target(&rpCtx, atlasWidth, atlasHeight, nodes.data(), (int)nodes.size());

    // 填充 rects 数组（保持原有逻辑）
    std::vector<stbrp_rect> rects(codepoints.size());
    for (size_t i = 0; i < codepoints.size(); ++i) {
        rects[i].id = (int)i;
        rects[i].w = tempGlyphs[i].w;
        rects[i].h = tempGlyphs[i].h;
        // 即使尺寸为 0，也允许打包（会被忽略）
    }

    // 执行打包
    stbrp_pack_rects(&rpCtx, rects.data(), (int)rects.size());

    // 将 SDF 位图拷贝到图集中，并建立映射
    for (size_t i = 0; i < codepoints.size(); ++i) {
        if (rects[i].was_packed && tempGlyphs[i].w > 0 && tempGlyphs[i].h > 0) {
            const TempGlyph& g = tempGlyphs[i];
            int rx = rects[i].x, ry = rects[i].y;
            for (int row = 0; row < g.h; ++row) {
                memcpy(&atlasPixels[(ry + row) * atlasWidth + rx],
                       &g.bitmap[row * g.w], g.w);
            }

            CharInfo info = {};
            // stbtt_packedchar info;
            info.x0 = (unsigned short)rx;
            info.y0 = (unsigned short)ry;
            info.x1 = (unsigned short)(rx + g.w);
            info.y1 = (unsigned short)(ry + g.h);
            info.xoff = (float)g.xoff;
            info.yoff = (float)g.yoff;
            info.xadvance = g.advance;
            // info_t.x0 = info.x0;
            // info_t.y0 = info.y0;
            // info_t.x1 = info.x1;
            // info_t.y1 = info.y1;
            // info_t.xoff = info.xoff;
            // info_t.yoff = info.yoff;
            // // info_t.yoff2 = info.yoff2;
            // // info_t.xoff2 = info.xoff2;
            // info_t.xadvance = info.xadvance;
            charMap[codepoints[i]] = info;
        }
    }

    // stbtt_PackEnd(&pc);

    // 上传图集到 OpenGL
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight,
                 0, GL_RED, GL_UNSIGNED_BYTE, atlasPixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// =============================================
//  渲染文本
// =============================================
void TextRenderer::RenderText(const std::string& text, float x, float y,
                                 float targetFontSize, float r, float g, float b)
{
    if (!ready || charMap.empty()) return;

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float screenW = (float)viewport[2];
    float screenH = (float)viewport[3];

    float scale = targetFontSize / baseFontSize;
    float smoothing = 0.25f / baseFontSize;  // 经验值，可调节

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(shaderProgram);

    glUniform2f(glGetUniformLocation(shaderProgram, "uScreenSize"), screenW, screenH);
    glUniform3f(glGetUniformLocation(shaderProgram, "uTextColor"), r, g, b);
    glUniform1f(glGetUniformLocation(shaderProgram, "uScale"), scale);
    glUniform1f(glGetUniformLocation(shaderProgram, "uSmoothing"), smoothing);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(shaderProgram, "uTextTexture"), 0);

    glBindVertexArray(VAO);

    // 生成顶点数据
    std::vector<float> vertices;
    vertices.reserve(text.size() * 6 * 4);

    float penX = x;
    const char* p = text.c_str();
    while (*p) {
        uint32_t cp = DecodeUtf8(p);
        auto it = charMap.find(cp);
        if (it == charMap.end()) continue;

        const CharInfo& pc = it->second;
        // 所有位置和偏移都需要乘以 scale
        float xpos = penX + pc.xoff * scale;
        float ypos = y   + pc.yoff * scale;
        float w = (pc.x1 - pc.x0) * scale;
        float h = (pc.y1 - pc.y0) * scale;

        float tx0 = (float)pc.x0 / atlasWidth;
        float ty0 = (float)pc.y0 / atlasHeight;
        float tx1 = (float)pc.x1 / atlasWidth;
        float ty1 = (float)pc.y1 / atlasHeight;

        float quad[6][4] = {
            { xpos,     ypos + h,   tx0, ty1 },
            { xpos + w, ypos + h,   tx1, ty1 },
            { xpos,     ypos,       tx0, ty0 },
            { xpos + w, ypos + h,   tx1, ty1 },
            { xpos + w, ypos,       tx1, ty0 },
            { xpos,     ypos,       tx0, ty0 }
        };

        for (int i = 0; i < 6; ++i)
            vertices.insert(vertices.end(), quad[i], quad[i] + 4);

        penX += pc.xadvance * scale;
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertices.size() / 4));

    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_BLEND);
}

// =============================================
//  获取文本宽度
// =============================================
float TextRenderer::GetTextWidth(const std::string& text, float targetFontSize) {
    float width = 0.0f;
    float scale = targetFontSize / baseFontSize;
    const char* p = text.c_str();
    while (*p) {
        uint32_t cp = DecodeUtf8(p);
        auto it = charMap.find(cp);
        if (it != charMap.end())
            width += it->second.xadvance * scale;
    }
    return width;
}

// =============================================
//  着色器与缓冲区初始化
// =============================================
void TextRenderer::SetupBuffersAndShader() {
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(s, 512, nullptr, log);
            std::cerr << "Shader compile error: " << log << std::endl;
        }
        return s;
    };

    GLuint vs = compile(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

// =============================================
//  析构函数
// =============================================
TextRenderer::~TextRenderer() {
    if (textureID) glDeleteTextures(1, &textureID);
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}
