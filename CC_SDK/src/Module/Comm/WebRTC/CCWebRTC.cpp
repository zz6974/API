#include "CCWebRTC.h"
#include "CCSystem.h"
#include <iostream>

void CTL::WebRTC::Init() {
    rtc::InitLogger(rtc::LogLevel::Warning,CTL::WebRTC::LogPrint);
    rtc::Preload();
}

void CTL::WebRTC::Release() {
    rtc::Cleanup();
}

CTL::WebRTC::WebRTC() = default;

CTL::WebRTC::~WebRTC() {
    Close();
}

bool CTL::WebRTC::Initialize() {
    try {
        if (!m_peerConnection) {
            SetupPeerConnection();
        }
        return m_peerConnection != nullptr;
    }
    catch (const std::exception& e) {
        if (m_onErrorCallback) {
            m_onErrorCallback(e.what());
        }
        return false;
    }
    catch (...) {
        if (m_onErrorCallback) {
            m_onErrorCallback("Unknown error during initialization");
        }
        return false;
    }
}

void CTL::WebRTC::SetupPeerConnection() {
    rtc::Configuration config;
    // 添加多个STUN服务器以提高连接成功率
    config.iceServers.emplace_back("stun:stun.l.google.com:19302");
    config.iceServers.emplace_back("stun:stun1.l.google.com:19302");
    config.iceServers.emplace_back("stun:stun.stunprotocol.org:3478");

    // 禁用UDP多路复用以减少复杂性
    config.enableIceUdpMux = false;

    // 设置ICE传输策略
    config.iceTransportPolicy = rtc::TransportPolicy::All;

    m_peerConnection = std::make_shared<rtc::PeerConnection>(config);
    m_peerConnection->onIceStateChange([this](rtc::PeerConnection::IceState iceState){
         if(m_onErrorCallback){
            switch(iceState) {
                case rtc::PeerConnection::IceState::Failed:
                    m_onErrorCallback("ICE gathering failed");
                    break;
                case rtc::PeerConnection::IceState::Disconnected:
                    m_onErrorCallback("ICE connection disconnected");
                    break;
                default:
                    break;
            }
        }
    });
    // 设置连接状态变化回调
    m_peerConnection->onStateChange([this](rtc::PeerConnection::State state) {
        switch (state) {
            case rtc::PeerConnection::State::Connected:
                if (m_onConnectionCallback) {
                    m_onConnectionCallback();
                }
                break;
            case rtc::PeerConnection::State::Failed:
                if (m_onErrorCallback) {
                    m_onErrorCallback("PeerConnection failed");
                }
                break;
            case rtc::PeerConnection::State::Closed:
                // 连接关闭时的处理
                if (m_onErrorCallback) {
                    m_onErrorCallback("PeerConnection closed");
                }
                break;
            default:
                break;
        }
    });

    // 设置数据通道回调
    m_peerConnection->onDataChannel([this](const std::shared_ptr<rtc::DataChannel> &dc) {
        // CTL::System::Println("Remote DataChannel created with label: '{}'", dc->label().c_str());
        m_dataChannel = dc;

        // 添加数据通道状态回调
        m_dataChannel->onOpen([this]() {
            try {
                if (this->m_onDataChannelOpen) {
                    this->m_onDataChannelOpen();
                }
            }
            catch (std::exception& exception) {
                const CTL::String error = CC::format("DataChannel onOpen Error: %s",exception.what());
                CC::Println(error,RED);
            }
        });

        m_dataChannel->onClosed([this]() {
            try {
                if (this->m_onDataChannelClose) {
                    this->m_onDataChannelClose();
                }
            }
            catch (std::exception& exception) {
                const CTL::String error = CC::format("DataChannel onClosed Error: %s",exception.what());
                CC::Println(error,RED);
            }
        });

        m_dataChannel->onMessage([this](const rtc::message_variant &data) {
            SetupDataChannelMessage(data);
        });
    });

    // 设置ICE候选回调
    m_peerConnection->onLocalCandidate([this](const rtc::Candidate& candidate) {
        if (m_onIceCandidateCallback) {
            ICECandidate iceCandidate;
            iceCandidate.candidate = candidate.candidate();
            iceCandidate.sdpMid = candidate.mid();
            iceCandidate.sdpMLineIndex = 0;
            m_onIceCandidateCallback(iceCandidate);
            // CTL::System::Println("ICE: {}",candidate.candidate().c_str());
        }
    });
    m_peerConnection->onLocalDescription([this](const rtc::Description& description) {
        if (m_onLocalDescriptionCallback) {
            const auto str = description.generateSdp();
            m_onLocalDescriptionCallback(str);
        }
    });
}

void CTL::WebRTC::SetupDataChannelMessage(const rtc::message_variant &data) const {
    if (m_onMessageCallback) {
        RTCData data_f{};
        if (std::holds_alternative<std::string>(data)) {
            data_f.message = std::get<std::string>(data);
            data_f.type = Text;
        }
        else {
            auto buffer= std::get<rtc::binary>(data);
            const auto str = reinterpret_cast<char *>(buffer.data());
            data_f.byteArray.assign(str, static_cast<ByteHander>(buffer.size()));
            data_f.type = Binary;
        }
        m_onMessageCallback(data_f);
    }
}

std::string CTL::WebRTC::CreateOffer() {
    if (!m_peerConnection) {
        if (m_onErrorCallback) {
            m_onErrorCallback("PeerConnection not initialized");
        }
        return "";
    }

    try {
        // 创建数据通道
        m_dataChannel = m_peerConnection->createDataChannel("test");
        m_dataChannel->onMessage([this](const rtc::message_variant &data) {
            SetupDataChannelMessage(data);
        });

        m_peerConnection->setLocalDescription(rtc::Description::Type::Offer);

        // 创建offer
        auto offerOpt = m_peerConnection->localDescription();
        if (!offerOpt) {
            if (m_onErrorCallback) {
                m_onErrorCallback("Failed to create offer: local description is not available");
            }
            return "";
        }
        rtc::Description offer = *offerOpt;
        return std::string(offer);
    }
    catch (const std::exception& e) {
        if (m_onErrorCallback) {
            m_onErrorCallback(e.what());
        }
        return "";
    }
}

bool CTL::WebRTC::SetRemoteDescription(const std::string& sdp, const std::string& type) const {
    if (!m_peerConnection) {
        if (m_onErrorCallback) {
            m_onErrorCallback("PeerConnection not initialized");
        }
        return false;
    }

    try {
        rtc::Description desc(sdp, type);
        m_peerConnection->setRemoteDescription(desc);
        return true;
    } catch (const std::exception& e) {
        if (m_onErrorCallback) {
            m_onErrorCallback(e.what());
        }
        return false;
    }
}

bool CTL::WebRTC::SetLocalDescription(const std::string& sdp, const std::string& type) const {
    if (!m_peerConnection) {
        if (m_onErrorCallback) {
            m_onErrorCallback("PeerConnection not initialized");
        }
        return false;
    }

    try {
        const rtc::Description desc(sdp, type);
        m_peerConnection->setLocalDescription(desc.type());
        return true;
    } catch (const std::exception& e) {
        if (m_onErrorCallback) {
            m_onErrorCallback(e.what());
        }
        return false;
    }
}

std::string CTL::WebRTC::CreateAnswer() const {
    if (!m_peerConnection) {
        if (m_onErrorCallback) {
            m_onErrorCallback("PeerConnection not initialized");
        }
        return "";
    }

    try {
        if (const auto answer = m_peerConnection->localDescription()) {
            return answer->generateSdp();
        }
        return "";
    }
    catch (const std::exception& e) {
        if (m_onErrorCallback) {
            m_onErrorCallback(e.what());
        }
        return "";
    }
}

bool CTL::WebRTC::AddIceCandidate(const std::string& candidate, const std::string& mid) const {
    if (!m_peerConnection) {
        if (m_onErrorCallback) {
            m_onErrorCallback("PeerConnection not initialized");
        }
        return false;
    }

    try {
        rtc::Candidate iceCandidate(candidate, mid);
        m_peerConnection->addRemoteCandidate(iceCandidate);
        return true;
    }
    catch (const std::exception& e) {
        if (m_onErrorCallback) {
            CTL::String str = CTL::String::format("addRemoteCandidate: {}",e.what());
            m_onErrorCallback(str);
        }
        return false;
    }
}

bool CTL::WebRTC::CreateDataChannel(const std::string& label) {
    if (!m_peerConnection) {
        if (m_onErrorCallback) {
            m_onErrorCallback("PeerConnection not initialized");
        }
        return false;
    }

    try {
        m_dataChannel = m_peerConnection->createDataChannel(label);
        m_dataChannel->onMessage([this](const rtc::message_variant &data) {
            SetupDataChannelMessage(data);
        });
        return true;
    }
    catch (const std::exception& e) {
        if (m_onErrorCallback) {
            m_onErrorCallback(e.what());
        }
        return false;
    }
}

bool CTL::WebRTC::IsDataChannelOpen() const {
    return m_dataChannel && m_dataChannel->isOpen();
}

bool CTL::WebRTC::IsConnected() const {
    return m_peerConnection &&
                   m_peerConnection->state() == rtc::PeerConnection::State::Connected;
}

bool CTL::WebRTC::IsClosed() const {
    return !m_peerConnection ||
       m_peerConnection->state() == rtc::PeerConnection::State::Closed ||
       m_peerConnection->state() == rtc::PeerConnection::State::Failed;
}

bool CTL::WebRTC::SendText(const std::string& message) const {
    if (!IsDataChannelOpen()) {
        return false;
    }
    if (!m_dataChannel) {
        if (m_onErrorCallback) {
            m_onErrorCallback("DataChannel not available");
        }
        return false;
    }

    try {
        m_dataChannel->send(message);
        return true;
    }
    catch (const std::exception& e) {
        if (m_onErrorCallback) {
            m_onErrorCallback(e.what());
        }
        return false;
    }
}

bool CTL::WebRTC::SendBinary(void *data, const int size) const {
    if (!IsDataChannelOpen()) {
        return false;
    }
    if (!m_dataChannel) {
        if (m_onErrorCallback) {
            m_onErrorCallback("DataChannel not available");
        }
        return false;
    }

    try {
        // 将void*转换为std::byte*
        auto byteData = static_cast<std::byte*>(data);
        // 创建rtc::binary对象
        rtc::binary buffer(byteData, byteData + size);
        // 发送二进制数据
        m_dataChannel->send(buffer);
        return true;
    }
    catch (const std::exception& e) {
        if (m_onErrorCallback) {
            m_onErrorCallback(e.what());
        }
        return false;
    }
}

bool CTL::WebRTC::SendByteArray(const ByteArray &data) const {
    return SendBinary(const_cast<char *>(data.buffer()), data.size());
}

void CTL::WebRTC::Close() {
    try {
        // 先关闭数据通道
        if (m_dataChannel.get()) {
            // 检查数据通道是否已经关闭
            if (m_dataChannel->isOpen()) {
                m_dataChannel->close();
            }
            m_dataChannel.reset();
        }

        // 再关闭PeerConnection
        if (m_peerConnection.get()) {
            // 检查连接状态
            auto state = m_peerConnection->state();
            if (state != rtc::PeerConnection::State::Closed &&
                state != rtc::PeerConnection::State::Failed) {
                m_peerConnection->close();
                }
            m_peerConnection.reset();
        }
    }
    catch (const std::exception& e) {
        // 记录错误但不抛出，因为这是清理操作
        std::cerr << "Error during WebRTC close: " << e.what() << std::endl;
    }
}

void CTL::WebRTC::SetOnMessageCallback(const OnMessageCallback& callback) {
    m_onMessageCallback = callback;
}

void CTL::WebRTC::SetOnDataChannelOpen(const OnDataChannelOpen& callback) {
    m_onDataChannelOpen = callback;
}

void CTL::WebRTC::SetOnDataChannelClose(const OnDataChannelClose& callback) {
    m_onDataChannelClose = callback;
}

void CTL::WebRTC::SetOnConnectionCallback(const OnConnectionCallback& callback) {
    m_onConnectionCallback = callback;
}

void CTL::WebRTC::SetOnErrorCallback(const OnErrorCallback& callback) {
    m_onErrorCallback = callback;
}

void CTL::WebRTC::SetOnIceCandidateCallback(const OnIceCandidateCallback& callback) {
    m_onIceCandidateCallback = callback;
}

void CTL::WebRTC::SetOnLocalDescriptionCallback(const LocalDescription& callback) {
    m_onLocalDescriptionCallback = callback;
}

void CTL::WebRTC::LogPrint(rtc::LogLevel level, std::string message){
    CTL::String str;
    if(level == rtc::LogLevel::None){
        str = "None";
    }
    else if(level == rtc::LogLevel::Fatal){
        str = "Fatal";
    }
    else if(level == rtc::LogLevel::Error){
        str = "Error";
    }
    else if(level == rtc::LogLevel::Warning){
        str = "Warning";
    }
    else if(level == rtc::LogLevel::Info){
         str = "Info";
    }
    else if(level == rtc::LogLevel::Debug){
        str = "Debug";
    }
    else if(level == rtc::LogLevel::Verbose){
        str = "Verbose";
    }
    CTL::System::Println("{} -> {}",str.c_str(),message.c_str());
}
