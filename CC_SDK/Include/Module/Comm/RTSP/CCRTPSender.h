#ifndef SRCCTL_CC_RTP_SENDER_H
#define SRCCTL_CC_RTP_SENDER_H

#include "rtpsession.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtppacket.h"
#include "CCByteArray.h"
#include <string>

namespace CTL {
    class RTPSender {
        jrtplib::RTPSession session;
        bool initialized;
        std::string destIP;
        uint16_t destRTPPort;
        uint16_t destRTCPPort;
    public:
        RTPSender() : initialized(false), destRTPPort(0), destRTCPPort(0) {}
        ~RTPSender();
        bool Init(const std::string& ip, uint16_t rtp_port, uint16_t rtcp_port,
            uint16_t Base_port,const std::string& local_ip);
        bool SendPacket(const CTL::ByteArray& data, uint8_t payloadType,uint32_t timestamp, bool marker);
        void Stop();
    };
}

#endif
