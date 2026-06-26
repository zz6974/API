#include "RTSP/CCRTPReceiver.h"

bool CTL::RTPReceiver::Init(uint16_t portBase, const std::string &ip) {
    jrtplib::RTPUDPv4TransmissionParams transParams;
    jrtplib::RTPSessionParams sessParams;

    // 设置时间戳单位（根据实际编码调整）
    sessParams.SetOwnTimestampUnit(1.0 / 90000.0); // H.264 常用 90kHz

    // 接受自己发送的包（可选）
    sessParams.SetAcceptOwnPackets(false);

    // 启用线程支持（自动轮询）
    sessParams.SetUsePollThread(true);

    // 设置本地端口
    transParams.SetPortbase(portBase);

    transParams.SetBindIP(inet_addr(ip.c_str()));

    // 创建会话
    int status = session.Create(sessParams, &transParams);
    if (status < 0) {
        std::cerr << "RTPSession Create Error: "
                << jrtplib::RTPGetErrorString(status) << std::endl;
        return false;
    }

    running = true;
    std::cout << "RTP Receiver started on port " << portBase << std::endl;
    return true;
}

bool CTL::RTPReceiver::AddDestination(const std::string &ip, uint16_t port) {
    jrtplib::RTPIPv4Address addr(inet_addr(ip.c_str()), port);
    int status = session.AddDestination(addr);
    if (status < 0) {
        std::cerr << "AddDestination Error: "
                << jrtplib::RTPGetErrorString(status) << std::endl;
        return false;
    }
    return true;
}

void CTL::RTPReceiver::ReceiveLoop() {
    while (running) {
        // 等待并接收数据
        session.BeginDataAccess();

        // 遍历所有有数据的源
        if (session.GotoFirstSourceWithData()) {
            do {
                jrtplib::RTPPacket *packet;

                // 获取当前源的所有数据包
                while ((packet = session.GetNextPacket()) != nullptr) {
                    ProcessRTPPacket(packet);

                    // 删除包（释放内存）
                    session.DeletePacket(packet);
                }
            } while (session.GotoNextSourceWithData());
        }

        session.EndDataAccess();

        // 短暂休眠，避免 CPU 占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }
}

void CTL::RTPReceiver::ProcessRTPPacket(const jrtplib::RTPPacket *packet) const {
    // 获取包头信息
    // uint32_t ssrc = packet->GetSSRC();
    // uint16_t seqNum = packet->GetSequenceNumber();
    // uint32_t timestamp = packet->GetTimestamp();
    // uint8_t payloadType = packet->GetPayloadType();
    // bool marker = packet->HasMarker();

    // 获取负载数据
    const uint8_t *payload = packet->GetPayloadData();
    size_t payloadLen = packet->GetPayloadLength();

    // std::cout << "Received RTP Packet:" << std::endl;
    // std::cout << "  SSRC: " << ssrc << std::endl;
    // std::cout << "  Seq: " << seqNum << std::endl;
    // std::cout << "  Timestamp: " << timestamp << std::endl;
    // std::cout << "  Payload Type: " << (int) payloadType << std::endl;
    // std::cout << "  Marker: " << marker << std::endl;
    // std::cout << "  Size: " << payloadLen << " bytes" << std::endl;

    if (recv_fun) {
        const CTL::ByteArray data(payload, payloadLen);
        recv_fun(data,packet);
    }

    // 在这里处理视频/音频数据
    // 例如：解码 H.264、播放音频等
}

void CTL::RTPReceiver::Stop() {
    running = false;
    session.BYEDestroy(jrtplib::RTPTime(1, 0), nullptr, 0);
}

jrtplib::RTPSession & CTL::RTPReceiver::GetSession() {
    return session;
}

void CTL::RTPReceiver::setRecvFun(const RTP_RecvFun &fun) {
    recv_fun = fun;
}
