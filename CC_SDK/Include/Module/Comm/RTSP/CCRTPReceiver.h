#ifndef SRCCTL_RTP_RECEIVER_H
#define SRCCTL_RTP_RECEIVER_H

#include "rtpsession.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "rtppacket.h"
#include "rtcpcompoundpacketbuilder.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <functional>
#include "CCByteArray.h"

typedef jrtplib::RTPPacket CJ_RTPPacket;

namespace CTL {
    using RTP_RecvFun = std::function<void(const CTL::ByteArray& data,const CJ_RTPPacket* packet)>;
    class RTPReceiver {
        jrtplib::RTPSession session;
        bool running;
        RTP_RecvFun recv_fun = nullptr;
    public:
        RTPReceiver() : running(false) {
        }

        // 初始化 RTP 会话
        bool Init(uint16_t portBase,const std::string &ip);

        // 添加目标地址（用于发送 RTCP RR）
        bool AddDestination(const std::string &ip, uint16_t port);

        // 接收 RTP 数据包（阻塞模式）
        void ReceiveLoop();

        // 处理单个 RTP 包
        void ProcessRTPPacket(const jrtplib::RTPPacket *packet) const;

        // 停止接收
        void Stop();

        // 获取会话对象（用于高级操作）
        jrtplib::RTPSession &GetSession();

        void setRecvFun(const RTP_RecvFun& fun);
    };
}

// 使用示例
// int main() {
// #ifdef RTP_SOCKETTYPE_WINSOCK
//     WSADATA dat;
//     WSAStartup(MAKEWORD(2, 2), &dat);
// #endif
//
//     CCRTPReceiver receiver;
//
//     // 1. 初始化（监听端口 5004）
//     if (!receiver.Init(5004)) {
//         return -1;
//     }
//
//     // 2. 添加目标（用于发送 RTCP）
//     receiver.AddDestination("192.168.1.100", 5005);
//
//     // 3. 启动接收线程
//     std::thread receiveThread([&receiver]() {
//         receiver.ReceiveLoop();
//     });
//
//     // 4. 主线程可以做其他事情
//     std::cout << "Press Enter to stop..." << std::endl;
//     std::cin.get();
//
//     // 5. 停止
//     receiver.Stop();
//     receiveThread.join();
//
// #ifdef RTP_SOCKETTYPE_WINSOCK
//     WSACleanup();
// #endif
//
//     return 0;
// }


#endif
