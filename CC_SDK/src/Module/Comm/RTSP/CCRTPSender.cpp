#include "RTSP/CCRTPSender.h"
#include <iostream>

CTL::RTPSender::~RTPSender() {
    Stop();
}

bool CTL::RTPSender::Init(const std::string &ip, uint16_t rtp_port, uint16_t rtcp_port,
    uint16_t Base_port,const std::string& local_ip)
{
    if (initialized) {
        Stop();
    }

    jrtplib::RTPSessionParams sessParams;
    sessParams.SetUsePollThread(true);
    sessParams.SetMaximumPacketSize(65535);
    sessParams.SetOwnTimestampUnit(1.0 / 1);
    sessParams.SetAcceptOwnPackets(false);

    jrtplib::RTPUDPv4TransmissionParams transParams;

    if (Base_port > 0) {
        transParams.SetPortbase(Base_port);
    } else {
        transParams.SetPortbase(0);
    }

    transParams.SetRTPSendBuffer(65536);
    transParams.SetRTCPSendBuffer(65536);
    transParams.SetBindIP(inet_addr(local_ip.c_str()));

    int status = session.Create(sessParams, &transParams);
    if (status < 0) {
        std::cerr << "RTPSender Create failed: " << status << std::endl;
        return false;
    }

    jrtplib::RTPIPv4Address rtpAddr(inet_addr(ip.c_str()), rtp_port);
    status = session.AddDestination(rtpAddr);
    if (status < 0) {
        std::cerr << "RTPSender AddDestination RTP failed: " << status << std::endl;
        session.Destroy();
        return false;
    }

    destIP = ip;
    destRTPPort = rtp_port;
    destRTCPPort = rtcp_port;
    initialized = true;
    
    std::cout << "RTPSender initialized: dest=" << ip << ":" << rtp_port
              << ", base_port=" << Base_port << std::endl;

    return true;
}

bool CTL::RTPSender::SendPacket(const CTL::ByteArray &data, uint8_t payloadType,
    uint32_t timestamp, bool marker)
{
    if (!initialized) {
        std::cerr << "RTPSender not initialized" << std::endl;
        return false;
    }

    if (data.size() == 0) {
        return false;
    }

    // session.SetDefaultPayloadType(payloadType);
    // session.SetDefaultMark(marker);

    int status = session.SendPacket(data.buffer(), data.size(),
                                  payloadType, marker, timestamp);

    if (status < 0) {
        std::cerr << "RTPSender SendPacket failed: " << status
                  << ", size=" << data.size() << std::endl;
        return false;
    }

    jrtplib::RTPTime::Wait(jrtplib::RTPTime(0, 100));

    return true;
}

void CTL::RTPSender::Stop() {
    if (initialized) {
        session.BYEDestroy(jrtplib::RTPTime(1, 0), nullptr, 0);
        session.Destroy();
        initialized = false;
    }
}

