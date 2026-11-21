#ifndef API_WEBRTC_H
#define API_WEBRTC_H

#include "rtc/rtc.hpp"
#include "mutex"
#include <functional>
#include <memory>
#include "CCWebSocket.h"
#ifdef _WIN32
#include <windows.h>
#endif

/**
 * @brief WebRTC DataChannel 会话封装(事件模型对齐 WebSocket:OnBind 注册回调)
 *
 * 与 WebSocket 的本质差异:建连前必须通过 OnLocalDescription /
 * OnLocalCandidate 回调把信令(SDP、候选)导出给对端,并由外部
 * SetRemoteDescription / AddRemoteCandidate 注入对端信令。
 */
class RTCBind {
    using OnOpenFunc     = std::function<void()>;
    using OnCloseFunc    = std::function<void()>;
    using OnMessageFunc  = std::function<void(const std::string&, bool isBinary)>;
    using OnStateFunc    = std::function<void(int state)>;
    using OnSignalFunc   = std::function<void(const std::string&)>;  // SDP / 候选出口
    using OnErrorFunc    = std::function<void(const std::string&)>;

    OnOpenFunc m_onOpen;
    OnCloseFunc m_onClose;
    OnMessageFunc m_onMessage;
    OnStateFunc m_onState;
    OnSignalFunc m_onLocalDescription;
    OnSignalFunc m_onLocalCandidate;
    OnErrorFunc m_onError;
};
class WebRTCServer {
public:

    WebRTCServer(bool Running = false) : m_Running(Running) {}
    void Run();
    void Stop();

    bool InitWebSocket(std::string &ip , int port);
    void SetRemoteDescription(const std::string& sdp);
    void AddRemoteCandidate(const std::string& candidate);

    void OnBind(const RTCBind& bind);

    bool SendSignal(const std::string& json);
    bool SendText(const std::string& text);
    bool SendBinary(const void* data, size_t size);

private:
    void SetupDatachannel();
    void SetupPeerConnection();
    std::shared_ptr<rtc::PeerConnection> m_pc;
    std::shared_ptr<rtc::DataChannel> m_dc;
    std::atomic<bool> m_Running;
    CTL::WebSocketServer web_server;
    CTL::WSBind m_bind;
    std::thread m_thread;
    SOCKET m_ClientId;

    void HandleOpen();
    void HandleClose();
    void HandleMessage(const std::string& message, bool isBinary);
    void HandleError(const std::string& error);

};

class WebRTCClient {

};

#endif //API_WEBRTC_H
