#ifndef CC_WEBRTC_H
#define CC_WEBRTC_H

#include <memory>
#include <string>
#include <functional>
#include "CCByteArray.h"
#include <rtc/rtc.hpp>
#include "CCJSONObject.h"

namespace CTL {
    enum DataType {
        Text = 0,
        Binary
    };
    struct RTCData {
        std::string message;
        ByteArray byteArray;
        DataType type;
    };
    struct ICECandidate {
        JSON_TYPE_INTRUSIVE(ICECandidate, candidate, sdpMid, sdpMLineIndex);
        std::string candidate;
        std::string sdpMid;
        int sdpMLineIndex;
    };
    class WebRTC {
    public:
        // 回调函数类型定义
        using OnMessageCallback = std::function<void(const RTCData&)>;
        using OnConnectionCallback = std::function<void()>;
        using OnDataChannelOpen = std::function<void()>;
        using OnDataChannelClose = std::function<void()>;
        using OnErrorCallback = std::function<void(const std::string&)>;
        using LocalDescription = std::function<void(const std::string&)>;
        using OnIceCandidateCallback = std::function<void(ICECandidate&)>;
        static void Init();
        static void Release();
        WebRTC();
        ~WebRTC();

        // 初始化PeerConnection
        bool Initialize();

        // 创建Offer
        std::string CreateOffer();

        // 设置远程描述
        bool SetRemoteDescription(const std::string& sdp, const std::string& type) const;

        // 设置本地描述
        bool SetLocalDescription(const std::string& sdp, const std::string& type) const;

        // 创建Answer
        std::string CreateAnswer() const;

        // 添加ICE候选
        bool AddIceCandidate(const std::string& candidate, const std::string& mid) const;

        // 创建数据通道
        bool CreateDataChannel(const std::string& label);

        // 数据通道是否打开
        bool IsDataChannelOpen() const;

        // 连接状态
        bool IsConnected() const;

        // 检查是否已关闭
        bool IsClosed() const;

        // 发送消息
        bool SendText(const std::string& message) const;
        bool SendBinary(void *data, int size) const;
        bool SendByteArray(const ByteArray& data) const;

        void Close();

        // 设置回调函数
        void SetOnMessageCallback(const OnMessageCallback& callback);
        void SetOnDataChannelOpen(const OnDataChannelOpen& callback);
        void SetOnDataChannelClose(const OnDataChannelClose& callback);
        void SetOnConnectionCallback(const OnConnectionCallback& callback);
        void SetOnErrorCallback(const OnErrorCallback& callback);
        void SetOnIceCandidateCallback(const OnIceCandidateCallback& callback);
        void SetOnLocalDescriptionCallback(const LocalDescription& callback);

    private:
        std::shared_ptr<rtc::PeerConnection> m_peerConnection;
        std::shared_ptr<rtc::DataChannel> m_dataChannel;

        OnMessageCallback m_onMessageCallback;
        OnDataChannelOpen m_onDataChannelOpen;
        OnDataChannelClose m_onDataChannelClose;
        OnConnectionCallback m_onConnectionCallback;
        OnErrorCallback m_onErrorCallback;
        OnIceCandidateCallback m_onIceCandidateCallback;
        LocalDescription m_onLocalDescriptionCallback;

        void SetupPeerConnection();
        void SetupDataChannelMessage(const rtc::message_variant &data) const;
        static void LogPrint(rtc::LogLevel level, std::string message);
    };
}

#endif
