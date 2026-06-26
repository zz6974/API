#include "MCP/CCMCPServer.h"

#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

// ============================================================================
// 构造 / 析构
// ============================================================================

CTL::MCPServer::MCPServer() = default;

CTL::MCPServer::~MCPServer() {
    Stop();
}

// ============================================================================
// 服务器配置
// ============================================================================

void CTL::MCPServer::SetServerInfo(const std::string& name, const std::string& version) {
    m_serverName    = name;
    m_serverVersion = version;
}

void CTL::MCPServer::SetProtocolVersion(const std::string& version) {
    m_protocolVersion = version;
}

void CTL::MCPServer::SetLogHandler(LogHandler handler) {
    m_logHandler = std::move(handler);
}

// ============================================================================
// 工具管理
// ============================================================================

void CTL::MCPServer::RegisterTool(const std::string& name,
                                  const std::string& description,
                                  const JSON& properties,
                                  const std::vector<std::string>& required,
                                  ToolHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);

    MCPTool toolDef;
    toolDef.name        = name;
    toolDef.description = description;
    toolDef.inputSchema.type       = "object";
    toolDef.inputSchema.properties = properties;
    toolDef.inputSchema.required   = required;

    m_tools.push_back({std::move(toolDef), std::move(handler)});
}

void CTL::MCPServer::RegisterTool(const std::string& name,
                                  const std::string& description,
                                  ToolHandler handler) {
    RegisterTool(name, description, JSON::object(), {}, std::move(handler));
}

// ============================================================================
// 资源管理
// ============================================================================

void CTL::MCPServer::RegisterResource(const std::string& uri,
                                      const std::string& name,
                                      const std::string& description,
                                      const std::string& mimeType,
                                      ResourceHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);

    MCPResource resDef;
    resDef.uri         = uri;
    resDef.name        = name;
    resDef.description = description;
    resDef.mimeType    = mimeType;

    m_resources.push_back({std::move(resDef), std::move(handler)});
}

// ============================================================================
// Prompt 管理
// ============================================================================

void CTL::MCPServer::RegisterPrompt(const std::string& name,
                                    const std::string& description,
                                    const std::vector<MCPPromptArgument>& arguments,
                                    PromptHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);

    MCPPrompt promptDef;
    promptDef.name        = name;
    promptDef.description = description;
    promptDef.arguments   = arguments;

    m_prompts.push_back({std::move(promptDef), std::move(handler)});
}

// ============================================================================
// 传输层：stdio
// ============================================================================

void CTL::MCPServer::RunStdio() {
    m_running = true;
    Log("info", "MCP Server starting in stdio mode...");

    // 逐行读取 stdin，每行为一个完整的 JSON-RPC 消息
    std::string line;
    while (m_running && std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }

        // 去除行尾的 \r（Windows 兼容）
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::string response = ProcessMessage(line);

        // 通知（无 id）不产生响应
        if (!response.empty()) {
            std::cout << response << std::endl;
            std::cout.flush();
        }
    }

    Log("info", "MCP Server stdio loop ended.");
    m_running = false;
}

// ============================================================================
// 传输层：HTTP + SSE
// ============================================================================

int CTL::MCPServer::RunHTTP(const std::string& address,const int port) {
    m_running = true;
    m_httpPort = port;

    Log("info", "MCP Server starting in HTTP+SSE mode on " + address + ":" + std::to_string(port));

    auto* servlet = new WebServlet();

    // CORS 配置
    CORS cors;
    cors.SetHeaders(CC_CORS_Header);
    cors.SetMethods(CC_CORS_Method);
    servlet->SetWebCORS(cors);
    if (!servlet->Init(address, port)) {
        return -2;
    }

    // ====================================================================
    // GET /sse  —— SSE 事件流（长连接，不关闭）
    // ====================================================================
    servlet->SetWebServlet("/sse", [this](Request& req, Response& resp) {
        // 设置 SSE 响应头
        resp.SetHeader("Content-Type", "text/event-stream");
        resp.SetHeader("Cache-Control", "no-cache");
        resp.SetHeader("Connection", "keep-alive");  // 覆盖框架的默认 close
        resp.SetHeader("X-Accel-Buffering", "no");
        resp.SetHeader("Transfer-Encoding", "identity");

        // 发送 endpoint 事件（WriteStatus 不会强制 Connection: close）
        std::string endpointUri = "http://127.0.0.1:" + std::to_string(m_httpPort) + "/message";
        std::string sseEndpoint = "event: endpoint\ndata: " + endpointUri + "\n\n";
        resp.WriteStatus(sseEndpoint, 200);

        Log("info", "SSE client connected, endpoint = " + endpointUri);

        // SSE 事件循环：阻塞当前线程，保持连接打开
        try {
            while (m_running) {
                std::string message;
                {
                    std::unique_lock<std::mutex> lock(m_sseMutex);
                    m_sseCV.wait_for(lock, std::chrono::seconds(30), [this] {
                        return !m_sseOutbox.empty() || !m_running;
                    });

                    if (!m_running) break;

                    if (!m_sseOutbox.empty()) {
                        message = m_sseOutbox.front();
                        m_sseOutbox.pop();
                    } else {
                        // 心跳：SSE 注释行，保持连接
                        message = ": heartbeat\n\n";
                    }
                }

                if (!message.empty()) {
                   bool F = resp.Flash(message.c_str(), message.size());
                }
            }
        }
        catch (const std::exception& e) {
            Log("info", std::string("SSE send error (client disconnected): ") + e.what());
        }
        catch (...) {
            Log("info", "SSE send error (client disconnected)");
        }

        Log("info", "SSE client disconnected");
    });

    // ====================================================================
    // POST /message  —— JSON-RPC 请求（双通道：HTTP 直接返回 + SSE 推送）
    // ====================================================================
    servlet->SetWebServlet("/message", [this](Request& req, Response& resp) {
        const auto &body_s = req.GetBody();
        std::string body = body_s.toString();

        // 回退：如果 Body 为空，尝试从 RawData 中提取
        if (body.empty()) {
            const std::string& raw = req.RequestData.RawData;
            size_t pos = raw.find("\r\n\r\n");
            if (pos != std::string::npos) {
                body = raw.substr(pos + 4);
            } else {
                pos = raw.find("\n\n");
                if (pos != std::string::npos) {
                    body = raw.substr(pos + 2);
                }
            }
        }

        // 去除 body 首尾空白
        while (!body.empty() && (body.front() == ' ' || body.front() == '\r' || body.front() == '\n'))
            body.erase(0, 1);
        while (!body.empty() && (body.back() == ' ' || body.back() == '\r' || body.back() == '\n'))
            body.pop_back();

        if (!body.empty()) {
            std::string jsonResponse = ProcessMessage(body);
            if (!jsonResponse.empty()) {
                // 通道1：推送到 SSE 队列（兼容传统 SSE 传输模式）
                std::string sseEvent = "event: message\ndata: ";
                for (char c : jsonResponse) {
                    if (c == '\n') sseEvent += "\ndata: ";
                    else sseEvent += c;
                }
                sseEvent += "\n\n";
                {
                    std::lock_guard<std::mutex> lock(m_sseMutex);
                    m_sseOutbox.push(sseEvent);
                }
                m_sseCV.notify_one();

                // 通道2：直接在 HTTP 响应中返回（兼容 streamable HTTP 模式）
                resp.SetHeader("Content-Type", "application/json");
                resp.Write(jsonResponse, 200);
                return;
            }
        }

        // 通知（无响应体）返回 202
        resp.SetHeader("Content-Type", "application/json");
        resp.Write("{}", 202);
    });

    // 启动
    m_httpServer = servlet;
    return servlet->Running(false, false);
}

// ============================================================================
// 停止
// ============================================================================

void CTL::MCPServer::Stop() {
    m_running = false;

    // 唤醒阻塞在 SSE 事件循环中的线程
    m_sseCV.notify_all();

    if (m_httpServer) {
        m_httpServer->Stop();
        delete m_httpServer;
        m_httpServer = nullptr;
    }

    Log("info", "MCP Server stopped.");
}

// ============================================================================
// 低级 API：处理单条 JSON-RPC 消息
// ============================================================================

std::string CTL::MCPServer::ProcessMessage(const std::string& rawMessage) {
    return HandleMessage(rawMessage);
}

std::string CTL::MCPServer::GetSSEEndpoint() const {
    return "http://127.0.0.1:" + std::to_string(m_httpPort) + "/message";
}

// ============================================================================
// JSON-RPC 核心消息处理
// ============================================================================

std::string CTL::MCPServer::HandleMessage(const std::string& rawMessage) {
    JSON request;

    // 1. 解析 JSON
    try {
        request = JSON::parse(rawMessage);
    }
    catch (const JSON::parse_error& e) {
        Log("error", std::string("JSON parse error: ") + e.what());
        JSON errResp = BuildError(nullptr, MCPErrorCode::ParseError, "Parse error: " + std::string(e.what()));
        return SerializeResponse(errResp);
    }

    // 2. 验证 JSON-RPC 基本字段
    if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0") {
        JSON errResp = BuildError(nullptr, MCPErrorCode::InvalidRequest, "Invalid Request: jsonrpc must be \"2.0\"");
        return SerializeResponse(errResp);
    }

    if (!request.contains("method") || !request["method"].is_string()) {
        JSON errResp = BuildError(nullptr, MCPErrorCode::InvalidRequest, "Invalid Request: missing method");
        return SerializeResponse(errResp);
    }

    std::string method = request["method"].get<std::string>();
    JSON params = request.value("params", JSON::object());
    bool isNotification = !request.contains("id");
    JSON id = request.value("id", JSON(nullptr));

    // 3. 检查初始化状态（initialize 方法例外）
    if (!m_initialized && method != "initialize") {
        JSON errResp = BuildError(id, MCPErrorCode::ServerNotInitialized,
                                  "Server not initialized. Send 'initialize' first.");
        return SerializeResponse(errResp);
    }

    // 4. 分发方法
    JSON result;
    try {
        result = DispatchMethod(method, params);
    }
    catch (const std::exception& e) {
        Log("error", std::string("Method dispatch error: ") + e.what());
        JSON errResp = BuildError(id, MCPErrorCode::InternalError, std::string("Internal error: ") + e.what());
        return SerializeResponse(errResp);
    }

    // 5. 通知不返回响应
    if (isNotification) {
        return "";
    }

    // 6. 检查 result 是否是错误响应（包含 "code" 和 "message" 字段）
    if (result.is_object() && result.contains("code") && result.contains("message") &&
        result["code"].is_number_integer() && result["code"].get<int>() != 0) {
        JSON errResp = BuildError(id, result["code"].get<int>(),
                                  result["message"].get<std::string>(),
                                  result.value("data", JSON(nullptr)));
        return SerializeResponse(errResp);
    }

    // 7. 构建成功响应
    return SerializeResponse(BuildResponse(id, result));
}

// ============================================================================
// JSON-RPC 响应构建
// ============================================================================

JSON CTL::MCPServer::BuildResponse(const JSON& id, const JSON& result) {
    JSON response;
    response["jsonrpc"] = "2.0";
    response["id"]      = id;
    response["result"]  = result;
    return response;
}

JSON CTL::MCPServer::BuildError(const JSON& id, int code, const std::string& message, const JSON& data) {
    JSON response;
    response["jsonrpc"] = "2.0";
    response["id"]      = id.is_null() ? JSON(nullptr) : id;

    JSON errorObj;
    errorObj["code"]    = code;
    errorObj["message"] = message;
    if (!data.is_null()) {
        errorObj["data"] = data;
    }
    response["error"] = errorObj;

    return response;
}

std::string CTL::MCPServer::SerializeResponse(const JSON& response) {
    return response.dump();
}

// ============================================================================
// 方法分发
// ============================================================================

JSON CTL::MCPServer::DispatchMethod(const std::string& method, const JSON& params) {
    if (method == "initialize")           return HandleInitialize(params);
    if (method == "ping")                 return HandlePing();
    if (method == "tools/list")           return HandleToolsList();
    if (method == "tools/call")           return HandleToolsCall(params);
    if (method == "resources/list")       return HandleResourcesList();
    if (method == "resources/read")       return HandleResourcesRead(params);
    if (method == "prompts/list")         return HandlePromptsList();
    if (method == "prompts/get")          return HandlePromptsGet(params);
    if (method == "notifications/initialized") {
        // 客户端通知服务器初始化完成——无需处理
        return JSON::object();
    }

    // 未知方法
    JSON error;
    error["code"]    = MCPErrorCode::MethodNotFound;
    error["message"] = "Method not found: " + method;
    return error;
}

// ============================================================================
// === 生命周期方法 ===
// ============================================================================

JSON CTL::MCPServer::HandleInitialize(const JSON& params) {
    if (m_initialized) {
        JSON error;
        error["code"]    = MCPErrorCode::InvalidRequest;
        error["message"] = "Server is already initialized";
        return error;
    }

    // 记录客户端信息
    m_clientName    = params.value("clientInfo", JSON::object()).value("name", "");
    m_clientVersion = params.value("clientInfo", JSON::object()).value("version", "");
    std::string clientProtocol = params.value("protocolVersion", "");

    Log("info", "Initialize from client: " + m_clientName + " v" + m_clientVersion +
                " (protocol: " + clientProtocol + ")");

    m_initialized = true;

    // 构建服务器能力
    JSON capabilities;
    capabilities["tools"]     = JSON::object();     // 支持工具
    capabilities["resources"] = JSON::object();     // 支持资源
    capabilities["prompts"]   = JSON::object();     // 支持提示词

    // 如果注册了工具，添加 listChanged 能力
    if (!m_tools.empty()) {
        capabilities["tools"]["listChanged"] = false;
    }
    if (!m_resources.empty()) {
        capabilities["resources"]["listChanged"] = false;
        capabilities["resources"]["subscribe"]   = false;
    }
    if (!m_prompts.empty()) {
        capabilities["prompts"]["listChanged"] = false;
    }

    JSON result;
    result["protocolVersion"] = m_protocolVersion;
    result["capabilities"]    = capabilities;

    JSON serverInfo;
    serverInfo["name"]    = m_serverName;
    serverInfo["version"] = m_serverVersion;
    result["serverInfo"]  = serverInfo;

    result["instructions"] = "CC-SDK MCP Server is ready. Use tools/list, resources/list, prompts/list to explore.";

    return result;
}

JSON CTL::MCPServer::HandlePing() {
    return JSON::object(); // 返回空对象表示 pong
}

// ============================================================================
// === 工具方法 ===
// ============================================================================

JSON CTL::MCPServer::HandleToolsList() {
    std::lock_guard<std::mutex> lock(m_mutex);

    JSON toolsArray = JSON::array();
    for (const auto& t : m_tools) {
        JSON tool;
        tool["name"]        = t.def.name;
        tool["description"] = t.def.description;

        JSON inputSchema;
        inputSchema["type"]       = t.def.inputSchema.type;
        inputSchema["properties"] = t.def.inputSchema.properties;
        if (!t.def.inputSchema.required.empty()) {
            inputSchema["required"] = t.def.inputSchema.required;
        }
        tool["inputSchema"] = inputSchema;

        toolsArray.push_back(tool);
    }

    JSON result;
    result["tools"] = toolsArray;
    return result;
}

JSON CTL::MCPServer::HandleToolsCall(const JSON& params) {
    std::string toolName = params.value("name", "");
    if (toolName.empty()) {
        return MakeInvalidParamsError(JSON(nullptr), "Missing 'name' parameter");
    }

    JSON arguments = params.value("arguments", JSON::object());

    std::lock_guard<std::mutex> lock(m_mutex);

    // 查找工具
    for (const auto& t : m_tools) {
        if (t.def.name == toolName) {
            try {
                JSON toolResult = t.handler(arguments);

                // 构建标准 MCP 工具响应
                JSON result;
                JSON contentArray = JSON::array();

                // 如果 handler 返回了 content 数组，直接使用
                if (toolResult.contains("content") && toolResult["content"].is_array()) {
                    result["content"] = toolResult["content"];
                }
                // 如果返回的是字符串，包装为 text content
                else if (toolResult.is_string()) {
                    JSON textContent;
                    textContent["type"] = "text";
                    textContent["text"] = toolResult.get<std::string>();
                    contentArray.push_back(textContent);
                    result["content"] = contentArray;
                }
                // 否则将整个结果转为 JSON 文本
                else {
                    JSON textContent;
                    textContent["type"] = "text";
                    textContent["text"] = toolResult.dump(2);
                    contentArray.push_back(textContent);
                    result["content"] = contentArray;
                }

                // 传递 isError
                if (toolResult.contains("isError")) {
                    result["isError"] = toolResult["isError"];
                }

                return result;
            }
            catch (const std::exception& e) {
                JSON errorContent;
                errorContent["type"] = "text";
                errorContent["text"] = std::string("Tool execution error: ") + e.what();

                JSON result;
                result["content"] = JSON::array({errorContent});
                result["isError"] = true;
                return result;
            }
        }
    }

    return MakeInvalidParamsError(JSON(nullptr), "Unknown tool: " + toolName);
}

// ============================================================================
// === 资源方法 ===
// ============================================================================

JSON CTL::MCPServer::HandleResourcesList() {
    std::lock_guard<std::mutex> lock(m_mutex);

    JSON resourcesArray = JSON::array();
    for (const auto& r : m_resources) {
        JSON resource;
        resource["uri"]         = r.def.uri;
        resource["name"]        = r.def.name;
        resource["description"] = r.def.description;
        resource["mimeType"]    = r.def.mimeType;
        resourcesArray.push_back(resource);
    }

    JSON result;
    result["resources"] = resourcesArray;
    return result;
}

JSON CTL::MCPServer::HandleResourcesRead(const JSON& params) {
    std::string uri = params.value("uri", "");
    if (uri.empty()) {
        return MakeInvalidParamsError(JSON(nullptr), "Missing 'uri' parameter");
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& r : m_resources) {
        if (r.def.uri == uri) {
            try {
                auto resPair = r.handler(uri);
                const auto& content = resPair.first;
                const auto& mimeType = resPair.second;

                JSON result;

                // 构建 contents 数组
                JSON textResource;
                textResource["uri"]      = uri;
                textResource["mimeType"] = mimeType;
                textResource["text"]     = content;

                result["contents"] = JSON::array({textResource});
                return result;
            }
            catch (const std::exception& e) {
                JSON error;
                error["code"]    = MCPErrorCode::InternalError;
                error["message"] = std::string("Resource read error: ") + e.what();
                return error;
            }
        }
    }

    return MakeInvalidParamsError(JSON(nullptr), "Unknown resource: " + uri);
}

// ============================================================================
// === Prompt 方法 ===
// ============================================================================

JSON CTL::MCPServer::HandlePromptsList() {
    std::lock_guard<std::mutex> lock(m_mutex);

    JSON promptsArray = JSON::array();
    for (const auto& p : m_prompts) {
        JSON prompt;
        prompt["name"]        = p.def.name;
        prompt["description"] = p.def.description;

        // 构建 arguments 数组
        JSON argsArray = JSON::array();
        for (const auto& arg : p.def.arguments) {
            JSON argObj;
            argObj["name"]        = arg.name;
            argObj["description"] = arg.description;
            argObj["required"]    = arg.required;
            argsArray.push_back(argObj);
        }
        prompt["arguments"] = argsArray;

        promptsArray.push_back(prompt);
    }

    JSON result;
    result["prompts"] = promptsArray;
    return result;
}

JSON CTL::MCPServer::HandlePromptsGet(const JSON& params) {
    std::string promptName = params.value("name", "");
    if (promptName.empty()) {
        return MakeInvalidParamsError(JSON(nullptr), "Missing 'name' parameter");
    }

    JSON arguments = params.value("arguments", JSON::object());

    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& p : m_prompts) {
        if (p.def.name == promptName) {
            try {
                JSON promptResult = p.handler(arguments);

                // 如果 handler 返回的已经包含 messages，直接使用
                if (promptResult.contains("messages")) {
                    return promptResult;
                }

                // 否则包装
                JSON result;
                result["description"] = p.def.description;
                result["messages"]    = promptResult;
                return result;
            }
            catch (const std::exception& e) {
                JSON error;
                error["code"]    = MCPErrorCode::InternalError;
                error["message"] = std::string("Prompt error: ") + e.what();
                return error;
            }
        }
    }

    return MakeInvalidParamsError(JSON(nullptr), "Unknown prompt: " + promptName);
}

// ============================================================================
// 辅助函数
// ============================================================================

JSON CTL::MCPServer::MakeInvalidParamsError(const JSON& id, const std::string& detail) {
    JSON error;
    error["code"]    = MCPErrorCode::InvalidParams;
    error["message"] = "Invalid params";
    if (!detail.empty()) {
        error["data"] = detail;
    }
    return error;
}

void CTL::MCPServer::Log(const std::string& level, const std::string& message) {
    if (m_logHandler) {
        m_logHandler(level, message);
    }
}
