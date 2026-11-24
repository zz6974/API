#include "WebRTC.h"
#include "../../../../Enviroment/libdatachannel/deps/plog/include/plog/Log.h"
#include "api/common/Logger.h"
#include "nlohmann/json.hpp"
#include <rtc/rtppacketizationconfig.hpp>

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
        const std::string json(buffer.begin() , buffer.end());
        try {
            const auto obj = nlohmann::json::parse(json);
            const std::string type = obj["type"];
            if (type == "offer" || type == "answer") {
                SetRemoteDescription(obj.value("sdp", ""));
            }else if (type == "candidate") {
                AddRemoteCandidate(obj.value("candidate", ""));
            }
        } catch (const std::exception &e) {
            LOG_ERROR("WebSocket message parse failed: {}", e.what());
        };
    };
    web_server.InitBind(bind);
    m_Running = f;
    return f;
}

void WebRTCServer::SetRemoteDescription(const std::string &sdp) {
    if (!m_pc) {
        LOG_ERROR("PeerConnection not initialized");
        return;
    }
    try {
        m_pc->setRemoteDescription(sdp);   //SDP
        m_track -> setDescription(sdp);
    } catch (const std::exception &e) {
        LOG_ERROR("setRemoteDescription failed: {}", e.what());
    }
}

void WebRTCServer::AddRemoteCandidate(const std::string &candidate) {
    if (!m_pc) {
        LOG_ERROR("PeerConnection not initialized");
        return;
    }
    try {
        m_pc->addRemoteCandidate(candidate);   //ICE
    } catch (const std::exception &e) {
        LOG_ERROR("addRemoteCandidate failed: {}", e.what());
    }
}

bool WebRTCServer::SendSignal(const std::string &json) {
    auto *client = web_server.getClient(m_ClientId);
    if (!client) {
        LOG_WARN("no client connected");
        return false;
    }
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
    if (!m_dc) return;
    m_dc->onOpen([this]() { HandleOpen(); });
    m_dc->onClosed([this]() { HandleClose(); });
    m_dc->onMessage([this](rtc::message_variant data) {
        if (std::holds_alternative<std::string>(data)) {
            HandleMessage(std::get<std::string>(data), false);
        } else {
            const auto &bin = std::get<rtc::binary>(data);
            HandleMessage(std::string(reinterpret_cast<const char *>(bin.data()), bin.size()), true);
        }
    });
}

void WebRTCServer::SetupPeerConnection() {
    rtc::Configuration config;
    config.iceServers.emplace_back("stun:stun.l.google.com:19302");
    m_pc = std::make_shared<rtc::PeerConnection>(config);

    // 导出 1:本地 SDP → JSON → WebSocket → 浏览器
    m_pc->onLocalDescription([this](const rtc::Description &description) {
        nlohmann::json msg;
        msg["type"] = description.type() == rtc::Description::Type::Offer ? "offer" : "answer";
        msg["sdp"] = std::string(description);
        m_Type = description.type();
        SendSignal(msg.dump());
    });

    // 导出 2:本地候选 → JSON → WebSocket → 浏览器
    m_pc->onLocalCandidate([this](const rtc::Candidate &candidate) {
        nlohmann::json msg;
        msg["type"] = "candidate";
        msg["candidate"] = std::string(candidate);
        msg["mid"] = candidate.mid();
        SendSignal(msg.dump());
    });

    m_dc = m_pc->createDataChannel("data");
    SetupDatachannel(); //注册
    //配置视频通道
    rtc::Description::Video video;
    //rtc::Description::Audio audio;    音频
    video.addH264Codec(96);
    video.addSSRC(1, "video-send");
    m_track = m_pc->addTrack(video);

    auto rtpConfig = std::make_shared<rtc::RtpPacketizationConfig>(
    1, "video-send", 96, rtc::H264RtpPacketizer::defaultClockRate);
    auto packetizer = std::make_shared<rtc::H264RtpPacketizer>(
        rtc::NalUnit::Separator::Length, rtpConfig);
    m_track->setMediaHandler(packetizer);

    m_pc->setLocalDescription(m_Type);
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
