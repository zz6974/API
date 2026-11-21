#include "WebRTC.h"

#include "../../../../Enviroment/libdatachannel/deps/plog/include/plog/Log.h"
#include "api/common/Logger.h"

void WebRTCServer::Run() {
    if (!m_Running) {return;}
    m_thread = std::thread([this]() {
        web_server.Running();});
}

void WebRTCServer::Stop() {
    m_Running = false;
    m_thread.join();
}

bool WebRTCServer::InitWebSocket(std::string &ip, int port) {
    bool f = web_server.Init(ip , port);
    if (!f) {
        LOG_ERROR("Failed to init WebSocket server");
    }
    CTL::WSBind bind;
    bind.URL = "/webrtc";
    bind.OnOpenFun = [this](CTL::WebClient& client) {
        m_ClientId = client.GetID();
        LOG_INFO("WebSocket opened");
    };
    bind.OnCloseFun = [this](CTL::WebClient& client) {
        m_ClientId = -1;
        LOG_INFO("WebSocket closed");
    };
    bind.OnErrorFun = [this](CTL::WebClient& client) { LOG_INFO("WebSocket error"); };
    bind.OnMessageFun = [this](CTL::WebClient& client) {
        auto buffer = client.GetBuffer();
        LOG_INFO("WebSocket message");
    };
    web_server.InitBind(bind);
    m_Running = f;
    return f;
}

void WebRTCServer::SetRemoteDescription(const std::string &sdp) {
}

void WebRTCServer::AddRemoteCandidate(const std::string &candidate) {
}

void WebRTCServer::OnBind(const RTCBind &bind) {

}
bool WebRTCServer::SendSignal(const std::string &json) {
    // ① 按句柄找到浏览器连接
    auto *client = web_server.getClient(m_ClientId);
    if (!client) {
        LOG_WARN("no client connected");
        return false;
    }
    // ② WebClient 继承 WebSocketInfo,SendText 走 EPClient 发送队列,线程安全
    return client->SendText(json.c_str());
}

bool WebRTCServer::SendText(const std::string &text) {
    if (!m_dc || !m_dc->isOpen()) {
        LOG_WARN("DataChannel not open");
        return false;
    }
    try {
        m_dc->send(text);
        return true;
    } catch (const std::exception &e) {
        LOG_ERROR("SendText failed: {}", e.what());
        return false;
    }
}

bool WebRTCServer::SendBinary(const void *data, size_t size) {
    if (!m_dc || !m_dc->isOpen()) {
        LOG_WARN("DataChannel not open");
        return false;
    }
    if (!data || size == 0) {
        return false;
    }
    try {
        // void* 收窄为 std::byte*,构造 rtc::binary 后发送
        const auto *bytes = static_cast<const std::byte *>(data);
        m_dc->send(rtc::binary(bytes, bytes + size));
        return true;
    } catch (const std::exception &e) {
        LOG_ERROR("SendBinary failed: {}", e.what());
        return false;
    }
}

void WebRTCServer::SetupDatachannel() {
}

void WebRTCServer::SetupPeerConnection() {
}

void WebRTCServer::HandleOpen() {
    LOG_INFO("WebRTC DataChannel opened");
}

void WebRTCServer::HandleClose() {
    LOG_INFO("WebRTC DataChannel closed");
}

void WebRTCServer::HandleMessage(const std::string &message, bool isBinary) {

}

void WebRTCServer::HandleError(const std::string &error) {
}
