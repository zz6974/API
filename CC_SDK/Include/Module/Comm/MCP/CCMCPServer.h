#ifndef SRCCTL_CC_MCP_SERVER_H
#define SRCCTL_CC_MCP_SERVER_H
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <memory>
#include <condition_variable>
#include <queue>

#include "CCJSONObject.h"
#include "CCWebServlet.h"

namespace CTL {

// ============================================================================
// MCP 协议数据结构
// ============================================================================

/// 工具输入参数的 JSON Schema 描述
struct MCPToolSchema {
    std::string type = "object";                    ///< 固定为 "object"
    JSON properties = JSON::object();               ///< 参数属性定义
    std::vector<std::string> required;              ///< 必填参数列表
};

/// 工具定义
struct MCPTool {
    std::string name;                               ///< 工具名称（唯一标识）
    std::string description;                        ///< 工具描述
    MCPToolSchema inputSchema;                      ///< 输入参数 Schema
};

/// 资源定义
struct MCPResource {
    std::string uri;                                ///< 资源 URI
    std::string name;                               ///< 资源名称
    std::string description;                        ///< 资源描述
    std::string mimeType;                           ///< MIME 类型
};

/// Prompt 参数定义
struct MCPPromptArgument {
    std::string name;                               ///< 参数名
    std::string description;                        ///< 参数描述
    bool required = false;                          ///< 是否必填
};

/// Prompt 定义
struct MCPPrompt {
    std::string name;                               ///< Prompt 名称
    std::string description;                        ///< Prompt 描述
    std::vector<MCPPromptArgument> arguments;       ///< 参数列表
};

// ============================================================================
// JSON-RPC 2.0 错误码
// ============================================================================
namespace MCPErrorCode {
    // JSON-RPC 2.0 标准错误码
    constexpr int ParseError     = -32700;
    constexpr int InvalidRequest = -32600;
    constexpr int MethodNotFound = -32601;
    constexpr int InvalidParams  = -32602;
    constexpr int InternalError  = -32603;

    // MCP 自定义错误码（-32000 ~ -32099）
    constexpr int ServerNotInitialized = -32002;
    constexpr int UnknownCapability    = -32003;
}

// ============================================================================
// MCPServer 类
// ============================================================================

/**
 * @brief MCP (Model Context Protocol) 服务器
 *
 * 实现 JSON-RPC 2.0 协议，支持：
 *   - 工具（Tools）：注册可调用的工具函数
 *   - 资源（Resources）：注册可读取的资源
 *   - 提示词（Prompts）：注册预定义的提示词模板
 *   - 传输层：stdio / HTTP+SSE 两种模式
 *
 * 使用示例：
 * @code
 *   CTL::MCPServer server;
 *   server.SetServerInfo("MyServer", "1.0.0");
 *
 *   // 注册工具
 *   server.RegisterTool("add", "加法运算",
 *       {{"a", "int"}, {"b", "int"}},
 *       [](const JSON& args) -> JSON {
 *           return {{"result", args["a"].get<int>() + args["b"].get<int>()}};
 *       });
 *
 *   // stdio 模式
 *   server.RunStdio();
 * @endcode
 */
class MCPServer {
public:
    // ========================================================================
    // 回调类型定义
    // ========================================================================

    /**
     * @brief 工具调用回调
     * @param arguments  客户端传入的参数（JSON 对象）
     * @return 工具执行结果（JSON 对象），包含 content 数组
     */
    using ToolHandler = std::function<JSON(const JSON& arguments)>;

    /**
     * @brief 资源读取回调
     * @param uri  资源 URI
     * @return pair<内容文本, MIME类型>
     */
    using ResourceHandler = std::function<std::pair<std::string, std::string>(const std::string& uri)>;

    /**
     * @brief Prompt 获取回调
     * @param arguments  客户端传入的参数（JSON 对象）
     * @return prompt 消息对象，包含 messages 数组
     */
    using PromptHandler = std::function<JSON(const JSON& arguments)>;

    /**
     * @brief 日志回调（用于将内部日志输出到 stderr / 文件）
     */
    using LogHandler = std::function<void(const std::string& level, const std::string& message)>;

    // ========================================================================
    // 构造 / 析构
    // ========================================================================
    MCPServer();
    ~MCPServer();

    // 禁止拷贝
    MCPServer(const MCPServer&) = delete;
    MCPServer& operator=(const MCPServer&) = delete;

    // ========================================================================
    // 服务器配置
    // ========================================================================

    /**
     * @brief 设置服务器信息（在 initialize 握手时返回给客户端）
     * @param name     服务器名称
     * @param version  服务器版本
     */
    void SetServerInfo(const std::string& name, const std::string& version);

    /**
     * @brief 设置 MCP 协议版本（默认 "2024-11-05"）
     */
    void SetProtocolVersion(const std::string& version);

    /**
     * @brief 设置日志回调
     */
    void SetLogHandler(LogHandler handler);

    // ========================================================================
    // 工具管理
    // ========================================================================

    /**
     * @brief 注册一个工具（简化版）
     * @param name          工具名称
     * @param description   工具描述
     * @param properties    参数属性（JSON 对象，如 {{"a", {{"type","number"}}}, {"b", {{"type","number"}}}} )
     * @param required      必填参数名列表
     * @param handler       工具调用回调
     */
    void RegisterTool(const std::string& name,
                      const std::string& description,
                      const JSON& properties,
                      const std::vector<std::string>& required,
                      ToolHandler handler);

    /**
     * @brief 注册一个无参数工具
     */
    void RegisterTool(const std::string& name,
                      const std::string& description,
                      ToolHandler handler);

    // ========================================================================
    // 资源管理
    // ========================================================================

    /**
     * @brief 注册一个资源
     * @param uri          资源 URI（如 "file:///data/sample.txt"）
     * @param name         资源名称
     * @param description  资源描述
     * @param mimeType     MIME 类型
     * @param handler      资源读取回调
     */
    void RegisterResource(const std::string& uri,
                          const std::string& name,
                          const std::string& description,
                          const std::string& mimeType,
                          ResourceHandler handler);

    // ========================================================================
    // Prompt 管理
    // ========================================================================

    /**
     * @brief 注册一个 Prompt 模板
     * @param name          Prompt 名称
     * @param description   Prompt 描述
     * @param arguments     Prompt 参数定义
     * @param handler       Prompt 获取回调
     */
    void RegisterPrompt(const std::string& name,
                        const std::string& description,
                        const std::vector<MCPPromptArgument>& arguments,
                        PromptHandler handler);

    // ========================================================================
    // 传输层
    // ========================================================================

    /**
     * @brief 以 stdio 模式运行（阻塞当前线程）
     *
     * 从 stdin 读取 JSON-RPC 请求，向 stdout 写入响应。
     * 适用于 MCP 客户端通过子进程方式启动服务器。
     */
    void RunStdio();

    /**
     * @brief 以 HTTP+SSE 模式运行（阻塞当前线程）
     *
     * 启动内置 HTTP 服务器，支持：
     *   - GET  /sse        SSE 事件流（服务器 → 客户端）
     *   - POST /message    客户端发送 JSON-RPC 请求
     *
     * @param address  监听地址
     * @param port     监听端口
     */
    int RunHTTP(const std::string& address, int port);

    /**
     * @brief 停止服务器
     */
    void Stop();

    // ========================================================================
    // 低级 API：处理单条 JSON-RPC 消息（用于集成到已有 HTTP 服务器）
    // ========================================================================

    /**
     * @brief 处理一条 JSON-RPC 消息，返回响应字符串
     * @param rawMessage  原始 JSON-RPC 消息（字符串）
     * @return 响应字符串（如果是通知则返回空字符串）
     */
    std::string ProcessMessage(const std::string& rawMessage);

    /**
     * @brief 获取 SSE 端点的事件流数据（用于集成）
     * @return SSE 格式的 endpoint 事件数据
     */
    std::string GetSSEEndpoint() const;

private:
    // ========================================================================
    // 内部类型
    // ========================================================================
    struct RegisteredTool {
        MCPTool def;
        ToolHandler handler;
    };

    struct RegisteredResource {
        MCPResource def;
        ResourceHandler handler;
    };

    struct RegisteredPrompt {
        MCPPrompt def;
        PromptHandler handler;
    };

    // ========================================================================
    // JSON-RPC 核心
    // ========================================================================
    std::string HandleMessage(const std::string& rawMessage);
    JSON BuildResponse(const JSON& id, const JSON& result);
    JSON BuildError(const JSON& id, int code, const std::string& message, const JSON& data = nullptr);
    std::string SerializeResponse(const JSON& response);

    // ========================================================================
    // 方法分发
    // ========================================================================
    JSON DispatchMethod(const std::string& method, const JSON& params);

    // ---- 生命周期 ----
    JSON HandleInitialize(const JSON& params);
    JSON HandlePing();

    // ---- 工具 ----
    JSON HandleToolsList();
    JSON HandleToolsCall(const JSON& params);

    // ---- 资源 ----
    JSON HandleResourcesList();
    JSON HandleResourcesRead(const JSON& params);

    // ---- Prompt ----
    JSON HandlePromptsList();
    JSON HandlePromptsGet(const JSON& params);

    // ========================================================================
    // 辅助函数
    // ========================================================================

    /// 构造 JSON-RPC 错误（用于内部统一处理）
    JSON MakeInvalidParamsError(const JSON& id, const std::string& detail = "");

    /// 日志输出
    void Log(const std::string& level, const std::string& message);

    // ========================================================================
    // 成员变量
    // ========================================================================
    std::string m_serverName    = "CC-SDK-MCP-Server";
    std::string m_serverVersion = "1.0.0";
    std::string m_protocolVersion = "2026-06-07";

    std::atomic<bool> m_initialized{false};
    std::string m_clientName;
    std::string m_clientVersion;

    // 注册项
    std::vector<RegisteredTool>     m_tools;
    std::vector<RegisteredResource> m_resources;
    std::vector<RegisteredPrompt>   m_prompts;

    // 线程安全
    mutable std::mutex m_mutex;

    // 运行状态
    std::atomic<bool> m_running{false};

    // HTTP 服务器（仅 HTTP 模式使用）
    WebServlet* m_httpServer = nullptr;
    int m_httpPort = 0;
    std::string m_messageEndpoint;

    // SSE 流式传输
    mutable std::mutex m_sseMutex;
    std::condition_variable m_sseCV;
    std::queue<std::string> m_sseOutbox;  // 待推送的 SSE 消息

    // 日志
    LogHandler m_logHandler = nullptr;
};

}

#endif
