#include "RTSP/CCRTSPServer.h"
#include <random>
#include <algorithm>
#include <mutex>
#include "CCFileOutStream.h"

CCMutex CTL::RTSPServer::port_mutex;

// ============================================================================
// RTSP_Stream 实现
// ============================================================================

CTL::RTSP_Stream::~RTSP_Stream() {
    receiver.Stop();
    sender.Stop();
}

bool CTL::RTSP_Stream::Init(int rtp_port, int rtcp_port,const std::string &ip) {
    if (!receiver.Init(rtp_port,ip)) {
        return false;
    }
    if (!receiver.AddDestination(ip, rtcp_port)) {
        return false;
    }
    CTL::Thread::addTask([this]() {
        receiver.ReceiveLoop();
        CTL::System::Println("receiver ReceiveLoop Close");
    });
    return true;
}

bool CTL::RTSP_Stream::InitSender(const std::string &dest_ip, int dest_rtp_port,
    int dest_rtcp_port,uint16_t Base_port,const std::string& local_ip) {
    isReceiverMode = false;
    if (!sender.Init(dest_ip, dest_rtp_port, dest_rtcp_port,Base_port,local_ip)) {
        return false;
    }
    return true;
}

void CTL::RTSP_Stream::setFun(const RTP_RecvFun &fun) {
    receiver.setRecvFun(fun);
}

bool CTL::RTSP_Stream::SendRTPPacket(const CTL::ByteArray &data, uint8_t payloadType, uint32_t timestamp, bool marker) {
    if (isReceiverMode) {
        return false;
    }
    return sender.SendPacket(data, payloadType, timestamp, marker);
}

void CTL::RTSP_Stream::addData(RTSP_MEDIA data) {
    RTP_Queue.Add(data);
}

CTL::AutoDestruct<CTL::RTSP_MEDIA> CTL::RTSP_Stream::getData() {
    return RTP_Queue.Poll();
}

size_t CTL::RTSP_Stream::dataSize() {
    return RTP_Queue.Size();
}

// ============================================================================
// RTSPClient 实现
// ============================================================================

CTL::RTSPClient::~RTSPClient() {
    clearStreams();
}

void CTL::RTSPClient::clearStreams() {
    for (const auto &item: Streams) {
        delete item.second;
    }
    Streams.clear();
}

void CTL::RTSPClient::addStream(RTSP_Stream *stream) {
    if (stream) {
        stream->streamID = Streams.size();
        Streams.put(stream->streamID,stream);
    }
}

CTL::RTSP_Stream * CTL::RTSPClient::getStream(const int id) {
    if (!Streams.IsContains(id)) {
        return nullptr;
    }
    const auto item = Streams.get(id);
    if (item && *item) {
        return *item;
    }
    return nullptr;
}

int CTL::RTSPClient::getStreamLength() {
    return Streams.size();
}

// ============================================================================
// RTSPServer: 线程安全客户端操作
// ============================================================================

CTL::RTSPClient* CTL::RTSPServer::getClient(const String& sessionId) {
    std::lock_guard<CCMutex> lock(clientMutex_);
    const auto it = Clients.get(sessionId);
    if (it && *it) {
        return *it;
    }
    return nullptr;
}

void CTL::RTSPServer::addClient(const String& sessionId, RTSPClient* client) {
    std::lock_guard<CCMutex> lock(clientMutex_);
    Clients.put(sessionId, client);
}

void CTL::RTSPServer::removeClient(const String& sessionId) {
    std::lock_guard<CCMutex> lock(clientMutex_);
    Clients.remove(sessionId);
}

// ============================================================================
// RTSPServer: 拉流数据注入
// ============================================================================

bool CTL::RTSPServer::pushMediaData(const String& path, const CTL::ByteArray& data,
                                     uint8_t payloadType, uint32_t timestamp, bool marker) {
    RTSP_MEDIA media;
    media.buffer = data;
    media.payloadType = payloadType;
    media.timestamp = timestamp;
    media.marker = marker;
    media.url = path;
    return pushMediaData(path, media);
}

bool CTL::RTSPServer::pushMediaData(const String& path, const RTSP_MEDIA& media) {
    if (!ServerFlag.load(std::memory_order_acquire)) {
        return false;
    }
    {
        std::lock_guard<CCMutex> lock(pullQueueMutex_);
        if (!pullDataQueue_.IsContains(path)) {
            pullDataQueue_.put(path, std::make_shared<CTL::Queue<RTSP_MEDIA>>());
        }
        const auto qPtr = pullDataQueue_.get(path);
        if (qPtr && *qPtr) {
            auto& queue = *(*qPtr);
            if (queue.Size() >= pullDataQueueMaxSize_) {
                queue.Poll();
            }
            queue.Add(media);
            return true;
        }
    }
    return false;
}

void CTL::RTSPServer::setPullDataSource(const PullDataSource& source) {
    std::lock_guard<CCMutex> lock(clientMutex_);
    pullDataSource_ = source;
}

// ============================================================================
// RTSPServer: 拉流分发
// ============================================================================

void CTL::RTSPServer::PullDistribute(const String& path, std::shared_ptr<std::atomic<bool>> runningFlag) {
    RTSPDataChannel::MessagePrint("PullDistribute started for path: {}", path.c_str());

    while (runningFlag && runningFlag->load(std::memory_order_acquire)) {
        // 1) 收集当前 path 下的所有拉流客户端
        std::vector<String> pullSessionIds;
        std::vector<RTSPClient*> pullClients;
        {
            std::lock_guard<CCMutex> lock(clientMutex_);
            const auto pullMapPtr = PullStreamingSession.get(path);
            if (pullMapPtr) {
                const auto ids = pullMapPtr->values();
                for (const auto& sid : ids) {
                    const auto cl = Clients.get(sid);
                    if (cl && *cl) {
                        pullSessionIds.push_back(sid);
                        pullClients.push_back(*cl);
                    }
                }
            }
        }

        if (pullClients.empty()) {
            // 没有拉流客户端，检查是否还有推流端在运行
            bool hasPushSource = false;
            {
                std::lock_guard<CCMutex> lock(clientMutex_);
                const auto pushIdPtr = PushStreamingSession.get(path);
                hasPushSource = (pushIdPtr && !pushIdPtr->empty());
            }
            if (!hasPushSource && !pullDataSource_) {
                // 既无推流端也无数据源回调，停止分发
                RTSPDataChannel::MessagePrint("PullDistribute stopped (no clients, no source): {}", path.c_str());
                break;
            }
            CTL::Thread::SleepMS(pullDistributeIntervalMs_);
            continue;
        }

        // 2) 尝试获取数据
        RTSP_MEDIA media;
        bool gotData = false;

        // 2a) 先从注入队列获取
        {
            std::lock_guard<CCMutex> lock(pullQueueMutex_);
            const auto qPtr = pullDataQueue_.get(path);
            if (qPtr && *qPtr) {
                const auto item = (*qPtr)->Poll();
                if (item) {
                    media = *item;
                    gotData = true;
                }
            }
        }

        // 2b) 如果没有注入数据，尝试从数据源回调获取
        if (!gotData && pullDataSource_) {
            PullDataSource sourceCopy;
            {
                std::lock_guard<CCMutex> lock(clientMutex_);
                sourceCopy = pullDataSource_;
            }
            if (sourceCopy) {
                try {
                    gotData = sourceCopy(path, media);
                } catch (...) {
                    gotData = false;
                }
            }
        }

        // 2c) 如果仍然没有数据，尝试从推流客户端获取
        if (!gotData) {
            std::lock_guard<CCMutex> lock(clientMutex_);
            const auto pushIdPtr = PushStreamingSession.get(path);
            if (pushIdPtr && !pushIdPtr->empty()) {
                const auto pushClientPtr = Clients.get(*pushIdPtr);
                if (pushClientPtr && *pushClientPtr) {
                    const auto pc = *pushClientPtr;
                    for (int i = 0; i < pc->getStreamLength(); ++i) {
                        const auto stream = pc->getStream(i);
                        if (stream) {
                            const auto item = stream->getData();
                            if (item) {
                                media = *item;
                                gotData = true;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // 3) 将数据发送给所有拉流客户端
        if (gotData) {
            for (size_t idx = 0; idx < pullClients.size(); ++idx) {
                const auto pullClient = pullClients[idx];
                if (!pullClient) continue;
                for (int i = 0; i < pullClient->getStreamLength(); ++i) {
                    const auto pullStream = pullClient->getStream(i);
                    if (pullStream && !pullStream->IsReceiverMode()) {
                        pullStream->SendRTPPacket(media.buffer, media.payloadType,
                                                  media.timestamp, media.marker);
                    }
                }
            }
        }

        CTL::Thread::SleepMS(pullDistributeIntervalMs_);
    }

    RTSPDataChannel::MessagePrint("PullDistribute stopped for path: {}", path.c_str());
}

void CTL::RTSPServer::startPullDistribution(const String& path) {
    // 检查是否已经启动
    {
        std::lock_guard<CCMutex> lock(clientMutex_);
        if (pullThreadFlags_.IsContains(path)) {
            return; // 已存在
        }
    }

    auto flag = std::make_shared<std::atomic<bool>>(true);
    {
        std::lock_guard<CCMutex> lock(clientMutex_);
        pullThreadFlags_.put(path, flag);
    }

    // 在线程池中启动分发任务
    M_S_POOl.AddTask([this, path, flag]() {
        PullDistribute(path, flag);
    });
}

void CTL::RTSPServer::stopPullDistribution(const String& path) {
    std::shared_ptr<std::atomic<bool>> flag;
    {
        std::lock_guard<CCMutex> lock(clientMutex_);
        const auto f = pullThreadFlags_.get(path);
        if (f && *f) {
            flag = *f;
        }
        pullThreadFlags_.remove(path);
    }
    if (flag) {
        flag->store(false, std::memory_order_release);
    }
    // 清理对应的数据队列
    {
        std::lock_guard<CCMutex> lock(pullQueueMutex_);
        pullDataQueue_.remove(path);
    }
}

// ============================================================================
// RTSPServer: 连接处理
// ============================================================================

void CTL::RTSPServer::HandleNewConnection(){
    if (!ServerFlag.load(std::memory_order_acquire)) {
        return;
    }
    const auto client = server->accept();
    const auto socket = client->getSocket();
    if (!socket) {
        return;
    }
    if (socket->Socketbit == SockError) {
        RTSPDataChannel::MessagePrint("CTL::RTSPServer::HandleNewConnection: " + CTL::String(strerror(errno)));
        return;
    }
    M_S_POOl.AddTask(&RTSPServer::ProcessingClientRequests,this,client);
}

void CTL::RTSPServer::ProcessingClientRequests(ClientSocket* client){
    const auto socket = client->getSocket();
    if (!socket) {
        return;
    }
    const auto Channel = new RTSPDataChannel(client);
    AutoDestruct<RTSPDataChannel> client_t(Channel);

    const String clientIP = client->getHostAddress().getHostAddress();
    const int clientPort = client->getPort();

    const auto Client = new RTSPClient;
    Client->SessionId = generateSessionId();
    Client->IP = clientIP;
    Client->Port = clientPort;
    addClient(Client->SessionId, Client);

    while (ServerFlag.load(std::memory_order_acquire)) {
        try {
            RTSP_Response response_t;
            response_t.setSocket(Channel);
            const String data = Channel->readHeader();
            if (data.empty() || data == "close") {
                // readHeader 超时 — 推流/拉流模式保持连接（RTP数据在UDP通道上传输）
                if (Client->StreamType == 0 || Client->StreamType == 1) {
                    continue;
                }
                break;
            }
            auto RTSP_Message = RTSPDataChannel::ParseRTSP(data);

            // 安全获取 CSeq
            String c_seq;
            auto cseqIt = RTSP_Message.headers.find("cseq");
            if (cseqIt != RTSP_Message.headers.end()) {
                c_seq = cseqIt->second;
            }

            // ================================================================
            // OPTIONS
            // ================================================================
            if (RTSP_Message.method == RTSP_REQ_OPTIONS) {
                response_t.reasonPhrase = "OK";
                response_t.statusCode = 200;
                response_t.setHeader("CSeq", c_seq);
                response_t.setHeader("Public","OPTIONS,DESCRIBE,SETUP,TEARDOWN,PLAY,PAUSE,ANNOUNCE,RECORD");
                try {
                    if (REQ_CallBack) {
                        REQ_CallBack(RTSP_Message,&response_t);
                    }
                }
                catch (CCException& e) {
                    RTSPDataChannel::MessagePrint(String::format("CTL::RTSPServer::OPTIONS Error: {}", e.what()));
                }
                if (response_t.auto_send) {
                    Channel->write(response_t.getBuild().c_str(), response_t.getBuild().length());
                }
            }
            // ================================================================
            // DESCRIBE
            // ================================================================
            else if (RTSP_Message.method == RTSP_REQ_DESCRIBE) {
                SDPBuilder sdp_builder;
                sdp_builder.setSessionName(String::format("RTSP_Stream_{}",Client->SessionId.c_str()));
                sdp_builder.setConnectionInfo("IN", "IP4", ServerIP.c_str());

                // 查找此 path 对应的推流端
                String pushSid;
                {
                    std::lock_guard<CCMutex> lock(clientMutex_);
                    const auto psPtr = PushStreamingSession.get(RTSP_Message.uri_path);
                    if (psPtr && !psPtr->empty()) {
                        pushSid = *psPtr;
                    }
                }

                RTSPClient* streamClient = nullptr;
                if (!pushSid.empty()) {
                    streamClient = getClient(pushSid);
                }

                if (streamClient) {
                    const auto streamLength = streamClient->getStreamLength();
                    for (int i = 0; i < streamLength; i++) {
                        const auto stream = streamClient->getStream(i);
                        if (stream) {
                            sdp_builder.addMedia(stream->SDP);
                            const auto stream_pull = new RTSP_Stream;
                            stream_pull->StreamType = 1; // 拉流模式
                            Client->addStream(stream_pull);
                        }
                    }
                    Client->StreamType = 1;
                    Client->Path = RTSP_Message.uri_path;
                }
                else {
                    // 无推流端 — 检查是否有数据源或已注册路径
                    bool hasSource = false;
                    {
                        std::lock_guard<CCMutex> lock(clientMutex_);
                        hasSource = (pullDataSource_ != nullptr) ||
                                    PullStreamingSession.IsContains(RTSP_Message.uri_path);
                    }
                    if (hasSource) {
                        // 允许纯拉流无推流场景：从 SDP 回调获取媒体描述
                        sdp_builder.setSessionName(String::format("RTSP_Stream_{}",Client->SessionId.c_str()));

                        String sdpMedia;
                        {
                            std::lock_guard<CCMutex> lock(clientMutex_);
                            if (pullSDPCallback_) {
                                sdpMedia = pullSDPCallback_(RTSP_Message.uri_path);
                            }
                        }

                        if (!sdpMedia.empty()) {
                            sdp_builder.addMedia(sdpMedia);
                        }

                        const auto stream_pull = new RTSP_Stream;
                        stream_pull->StreamType = 1;
                        stream_pull->SDP = sdpMedia;
                        Client->addStream(stream_pull);
                        Client->StreamType = 1;
                        Client->Path = RTSP_Message.uri_path;
                    }
                    else {
                        response_t.reasonPhrase = "Not Found";
                        response_t.statusCode = 404;
                        response_t.setHeader("CSeq", c_seq);
                        try {
                            if (REQ_CallBack) REQ_CallBack(RTSP_Message,&response_t);
                        } catch (CCException& e) {
                            RTSPDataChannel::MessagePrint(String::format("DESCRIBE Error: {}", e.what()));
                        }
                        if (response_t.auto_send) {
                            Channel->write(response_t.getBuild().c_str(), response_t.getBuild().length());
                        }
                        // 404 后继续循环，让客户端有机会重试
                        continue;
                    }
                }

                const auto sdp_content = sdp_builder.build();
                response_t.reasonPhrase = "OK";
                response_t.statusCode = 200;
                response_t.setHeader("CSeq", c_seq);
                response_t.setHeader("Content-Type","application/sdp");
                response_t.setHeader("Content-Length",CC::to_String(static_cast<long>(sdp_content.length())));
                String response = response_t.getBuild();
                response.append(sdp_content);
                try {
                    if (REQ_CallBack) REQ_CallBack(RTSP_Message,&response_t);
                } catch (CCException& e) {
                    RTSPDataChannel::MessagePrint(String::format("DESCRIBE Error: {}", e.what()));
                }
                if (response_t.auto_send) {
                    Channel->write(response.c_str(), response.length());
                }
            }
            // ================================================================
            // SETUP
            // ================================================================
            else if (RTSP_Message.method == RTSP_REQ_SETUP) {
                response_t.reasonPhrase = "OK";
                response_t.statusCode = 200;
                int c_rtp_port = 0, c_rtcp_port = 0;
                const String transport = RTSP_Message.headers.count("transport") ?
                    RTSP_Message.headers.at("transport") : "RTP/UDP";
                getClientPort(&c_rtp_port, &c_rtcp_port, transport);

                // 判断推流/拉流模式
                bool isPushMode = false;
                {
                    const String mode = RTSP_Message.headers.count("mode") ?
                        RTSP_Message.headers.at("mode") : "";
                    if (mode == "record") {
                        isPushMode = true;
                    }
                }

                CCVector<int> PortList;
                String server_port;

                if (isPushMode) {
                    // ============ 推流模式 (客户端 → 服务器) ============
                    Client->StreamType = 0;
                    Client->Path = RTSP_Message.base_path;
                    PortList = DatagramSocket::assignPortNumber(ServerIP, startPort, endPort, 2, true);
                    Thread::SleepMS(200);

                    const auto stream = Client->getStream(Client->getStreamLength() - 1);
                    if (stream) {
                        stream->Init(PortList[0], c_rtcp_port, ServerIP);

                        // 注册推流会话
                        {
                            std::lock_guard<CCMutex> lock(clientMutex_);
                            PushStreamingSession.put(Client->Path, Client->SessionId);
                            if (!PullStreamingSession.IsContains(Client->Path)) {
                                PullStreamingSession.put(Client->Path, {});
                            }
                        }

                        // ★ 修复: 值捕获替代 [&] 引用捕获
                        const String sessionId = Client->SessionId;
                        const int sid = stream->streamID;
                        CTL::Thread::addTask([this, sessionId, sid]() {
                            Push(sessionId, sid);
                        });

                        // ★ 修复: 值捕获
                        RTSPClient* clientPtr = Client;
                        const int streamID = stream->streamID;
                        stream->setFun([this, clientPtr, streamID](const CTL::ByteArray& buff, const CJ_RTPPacket* packet) {
                            this->ReadData(buff, packet, clientPtr, streamID);
                        });
                    }
                    server_port = String::format("{}-{}", PortList[0], PortList[1]);
                }
                else {
                    // ============ 拉流模式 (服务器 → 客户端) ============
                    Client->StreamType = 1;
                    Client->Path = RTSP_Message.base_path;
                    PortList = DatagramSocket::assignPortNumber(ServerIP, startPort, endPort, 2, true);
                    Thread::SleepMS(200);

                    // ★ 修复: 遍历所有流而非硬编码 getStream(0)
                    bool senderInited = false;
                    for (int i = 0; i < Client->getStreamLength(); ++i) {
                        const auto pullStream = Client->getStream(i);
                        if (pullStream && pullStream->StreamType == 1) {
                            if (pullStream->InitSender(Client->IP, c_rtp_port, c_rtcp_port, PortList[0], ServerIP)) {
                                senderInited = true;
                            }
                        }
                    }

                    if (senderInited) {
                        server_port = String::format("{}-{}", PortList[0], PortList[1]);
                        RTSPDataChannel::MessagePrint("SETUP pull stream: client={}, path={}",
                            Client->SessionId.c_str(), RTSP_Message.base_path.c_str());
                    }
                    else {
                        response_t.reasonPhrase = "Internal Server Error";
                        response_t.statusCode = 500;
                    }
                }

                response_t.setHeader("CSeq", c_seq);
                if (!server_port.empty()) {
                    response_t.setHeader("Transport", transport + ";server_port=" + server_port);
                }
                response_t.setHeader("Session", Client->SessionId + ";timeout=60");
                try {
                    if (REQ_CallBack) REQ_CallBack(RTSP_Message, &response_t);
                } catch (CCException& e) {
                    RTSPDataChannel::MessagePrint(String::format("SETUP Error: {}", e.what()));
                }
                if (response_t.auto_send) {
                    Channel->write(response_t.getBuild().c_str(), response_t.getBuild().length());
                }
            }
            // ================================================================
            // PLAY
            // ================================================================
            else if (RTSP_Message.method == RTSP_REQ_PLAY) {
                const String path = RTSP_Message.uri_path;
                bool pathExists = false;
                {
                    std::lock_guard<CCMutex> lock(clientMutex_);
                    pathExists = PullStreamingSession.IsContains(path);
                }

                if (!pathExists) {
                    // 路径未注册 — 可能是纯拉流场景，允许动态创建
                    {
                        std::lock_guard<CCMutex> lock(clientMutex_);
                        if (!PullStreamingSession.IsContains(path)) {
                            PullStreamingSession.put(path, {});
                        }
                    }
                }

                // 将当前客户端注册到拉流列表
                {
                    std::lock_guard<CCMutex> lock(clientMutex_);
                    const auto pullMapPtr = PullStreamingSession.get(path);
                    if (pullMapPtr) {
                        pullMapPtr->put(Client->SessionId, Client->SessionId);
                    }
                }

                // ★ 启动拉流分发线程（幂等）
                startPullDistribution(path);

                response_t.reasonPhrase = "OK";
                response_t.statusCode = 200;
                response_t.setHeader("CSeq", c_seq);
                response_t.setHeader("Range", "npt=0-");
                response_t.setHeader("Session", Client->SessionId + ";timeout=60");
                try {
                    if (REQ_CallBack) REQ_CallBack(RTSP_Message, &response_t);
                } catch (CCException& e) {
                    RTSPDataChannel::MessagePrint(String::format("PLAY Error: {}", e.what()));
                }
                if (response_t.auto_send) {
                    Channel->write(response_t.getBuild().c_str(), response_t.getBuild().length());
                }
            }
            // ================================================================
            // PAUSE
            // ================================================================
            else if (RTSP_Message.method == RTSP_REQ_PAUSE) {
                response_t.reasonPhrase = "OK";
                response_t.statusCode = 200;
                response_t.setHeader("CSeq", c_seq);
                try {
                    if (REQ_CallBack) REQ_CallBack(RTSP_Message, &response_t);
                } catch (CCException& e) {
                    RTSPDataChannel::MessagePrint(String::format("PAUSE Error: {}", e.what()));
                }
                if (response_t.auto_send) {
                    Channel->write(response_t.getBuild().c_str(), response_t.getBuild().length());
                }
            }
            // ================================================================
            // TEARDOWN
            // ================================================================
            else if (RTSP_Message.method == RTSP_REQ_TEARDOWN) {
                response_t.reasonPhrase = "OK";
                response_t.statusCode = 200;
                response_t.setHeader("CSeq", c_seq);
                try {
                    if (REQ_CallBack) REQ_CallBack(RTSP_Message, &response_t);
                } catch (CCException& e) {
                    RTSPDataChannel::MessagePrint(String::format("TEARDOWN Error: {}", e.what()));
                }
                if (response_t.auto_send) {
                    Channel->write(response_t.getBuild().c_str(), response_t.getBuild().length());
                }
                break; // 结束当前连接的处理循环
            }
            // ================================================================
            // ANNOUNCE
            // ================================================================
            else if (RTSP_Message.method == RTSP_REQ_ANNOUNCE) {
                uint64_t len = 0;
                auto clIt = RTSP_Message.headers.find("content-length");
                if (clIt != RTSP_Message.headers.end()) {
                    len = clIt->second.to_int();
                }
                if (len <= 0) {
                    clIt = RTSP_Message.headers.find("Content-Length");
                    if (clIt != RTSP_Message.headers.end()) {
                        len = clIt->second.to_int();
                    }
                }

                Client->StreamType = 0;
                Client->Path = RTSP_Message.uri_path;

                // 线程安全注册
                {
                    std::lock_guard<CCMutex> lock(clientMutex_);
                    PushStreamingSession.put(Client->Path, Client->SessionId);
                    if (!PullStreamingSession.IsContains(Client->Path)) {
                        PullStreamingSession.put(Client->Path, {});
                    }
                }

                if (len > 0) {
                    RTSP_Message.body = Channel->readData(len);
                    RTSP_Message.sdp_content = RTSP_Message.body;
                }

                response_t.reasonPhrase = "OK";
                response_t.statusCode = 200;
                response_t.setHeader("CSeq", c_seq);
                const auto stream = new RTSP_Stream;
                stream->SDP = RTSP_Message.sdp_content;
                Client->addStream(stream);
                try {
                    if (REQ_CallBack) REQ_CallBack(RTSP_Message, &response_t);
                } catch (CCException& e) {
                    RTSPDataChannel::MessagePrint(String::format("ANNOUNCE Error: {}", e.what()));
                }
                if (response_t.auto_send) {
                    Channel->write(response_t.getBuild().c_str(), response_t.getBuild().length());
                }
            }
            // ================================================================
            // RECORD
            // ================================================================
            else if (RTSP_Message.method == RTSP_REQ_RECORD) {
                response_t.reasonPhrase = "OK";
                response_t.statusCode = 200;
                response_t.setHeader("CSeq", c_seq);
                if (RTSP_Message.headers.count("range") || RTSP_Message.headers.count("Range")) {
                    String range_value = RTSP_Message.headers.count("range") ?
                        RTSP_Message.headers.at("range") : RTSP_Message.headers.at("Range");
                    response_t.setHeader("Range", range_value);
                }
                try {
                    if (REQ_CallBack) REQ_CallBack(RTSP_Message, &response_t);
                } catch (CCException& e) {
                    RTSPDataChannel::MessagePrint(String::format("RECORD Error: {}", e.what()));
                }
                if (response_t.auto_send) {
                    Channel->write(response_t.getBuild().c_str(), response_t.getBuild().length());
                }
            }
            // ================================================================
            // 未知方法 / 回调处理
            // ================================================================
            else {
                response_t.setHeader("CSeq", c_seq);
                try {
                    if (REQ_CallBack) REQ_CallBack(RTSP_Message, &response_t);
                } catch (CCException& e) {
                    RTSPDataChannel::MessagePrint(String::format("OTHER Error: {}", e.what()));
                }
                if (response_t.auto_send) {
                    Channel->write(response_t.getBuild().c_str(), response_t.getBuild().length());
                }
            }
        }
        catch (const std::out_of_range&) {
            // 缺少必需的 header（如 CSeq），跳过
            RTSPDataChannel::MessagePrint("CTL::RTSPServer::ProcessingClientRequests missing required header");
        }
        catch (CCException& e) {
            RTSPDataChannel::MessagePrint(String::format("CTL::RTSPServer::ProcessingClientRequests Error: {}", e.what()));
        }
    }

    // ── 连接断开，清理资源 ──
    const String clientPath = Client->Path;
    const String clientSid = Client->SessionId;

    if (Client->StreamType == 0) {
        // 推流端断开
        {
            std::lock_guard<CCMutex> lock(clientMutex_);
            PushStreamingSession.remove(clientPath);
            PullStreamingSession.remove(clientPath);
        }
        stopPullDistribution(clientPath);
    }
    else {
        // 拉流端断开
        {
            std::lock_guard<CCMutex> lock(clientMutex_);
            const auto pullMapPtr = PullStreamingSession.get(clientPath);
            if (pullMapPtr) {
                pullMapPtr->remove(clientSid);
            }
        }
    }

    removeClient(clientSid);
    delete Client;
}

void CTL::RTSPServer::getClientPort(int* rtp, int* rtcp, const String& transport){
    if (!rtp || !rtcp) {
        return;
    }
    size_t client_port_pos = transport.find("client_port=");
    if (client_port_pos != String::npos) {
        client_port_pos += 12; // 跳过 "client_port="
        size_t dash_pos = transport.find('-', client_port_pos);
        if (dash_pos != String::npos) {
            String rtp_port_str = transport.substr(client_port_pos, dash_pos - client_port_pos);
            String rtcp_port_str = transport.substr(dash_pos + 1);
            try {
                *rtp = std::stoi(rtp_port_str);
                *rtcp = std::stoi(rtcp_port_str);
            }
            catch (...) {
                *rtp = 0;
                *rtcp = 0;
            }
        }
    }
}

void CTL::RTSPServer::removeRTSPSession(const String& sessionId){

}

CTL::String CTL::RTSPServer::generateSessionId(){
    // 使用随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << std::hex;

    // 生成一个UUID风格的session ID (32位十六进制字符)
    for (int i = 0; i < 16; ++i) {
        ss << std::setw(2) << std::setfill('0') << dis(gen);
    }

    return ss.str();
}

void CTL::RTSPServer::ReadData(const CTL::ByteArray &data, const CJ_RTPPacket *packet,
    RTSPClient* client,int streamID) const
{
    // uint32_t ssrc = packet->GetSSRC();
    // uint16_t seqNum = packet->GetSequenceNumber();
    const uint32_t timestamp = packet->GetTimestamp();
    const uint8_t payloadType = packet->GetPayloadType();
    const bool marker = packet->HasMarker();
    // size_t payloadLen = packet->GetPayloadLength();
    //
    // std::cout << "Received RTP Packet:" << std::endl;
    // std::cout << "  SSRC: " << ssrc << std::endl;
    // std::cout << "  Seq: " << seqNum << std::endl;
    // std::cout << "  Timestamp: " << timestamp << std::endl;
    // std::cout << "  Payload Type: " << (int) payloadType << std::endl;
    // std::cout << "  Marker: " << marker << std::endl;
    // std::cout << "  Size: " << payloadLen << " bytes" << std::endl;
    // std::cout << "  streamID: " << streamID << std::endl;

    // CTL::FileOutStream::Append("./File/Out.mp3", data.toCharVector());
    if (client) {
        const auto stream = client->getStream(streamID);
        if (stream) {
            RTSP_MEDIA media;
            media.buffer = data;
            media.packet = packet;
            media.url = client->Path;
            media.payloadType = payloadType;
            media.timestamp = timestamp;
            media.marker = marker;
            if (MediaStreamCallback_t) {
                try {
                    MediaStreamCallback_t(media,streamID);
                }
                catch (CCException& e) {
                    CTL::System::Println("CTL::RTSPServer::ReadData MediaStreamCallback Error: {}",e.what());
                }
            }
            stream->addData(media);
            // CTL::System::Println("stream length: {}",stream->dataSize());
        }
    }
}

void CTL::RTSPServer::Push(const String &sessionId,const int streamID) {
    while (ServerFlag.load(std::memory_order_acquire)) {
        const auto client = getClient(sessionId);
        if (!client) {
            break;
        }
        const auto stream = client->getStream(streamID);
        if (!stream) {
            CTL::Thread::SleepMS(3);
            continue;
        }
        const auto media = stream->getData();
        if (!media) {
            CTL::Thread::SleepMS(3);
            continue;
        }

        // 获取拉流客户端列表
        std::vector<String> pullIds;
        {
            std::lock_guard<CCMutex> lock(clientMutex_);
            const auto pullMapPtr = PullStreamingSession.get(client->Path);
            if (pullMapPtr) {
                pullIds = pullMapPtr->values();
            }
        }

        for (const auto& item : pullIds) {
            const auto pullClient = getClient(item);
            if (pullClient) {
                const auto pullStream = pullClient->getStream(streamID);
                if (pullStream && !pullStream->IsReceiverMode()) {
                    pullStream->SendRTPPacket(media->buffer, media->payloadType,
                                              media->timestamp, media->marker);
                }
            }
        }
        CTL::Thread::SleepMS(3);
    }
}

CTL::RTSPServer::RTSPServer()= default;

CTL::RTSPServer::~RTSPServer(){
    Stop();
    // 等待后台线程结束
    Thread::SleepMS(100);
}

bool CTL::RTSPServer::Init(const InetAddress& address, const int Port){
    const InetAddress addr(this->IPvx,address.getHostAddress());
    server = new ServerSocket(Port, addr);
    ServerIP = address.getHostAddress();
    return server->isBound();
}

void CTL::RTSPServer::SetCallBack(const RTSP_REQ_FUN& fun){
    REQ_CallBack = fun;
}

int CTL::RTSPServer::Running(){
    if (!server) {
        return -1;
    }
    const auto socket = server->getSocket();
    if (!socket) {
        return -2;
    }
    ServerFlag.store(true, std::memory_order_release);
    const bool F = server->listen();
    if (!F) {
        RTSPDataChannel::MessagePrint("CTL::RTSPServer::Running Listen Failed");
        return -1;
    }
    M_S_POOl.InitStart(M_Pool_Size, M_Pool_Max, M_Pool_KeepAliveTime);
    event_loop_t.registerEvent(socket->Socketbit,[this](uint32_t event) {
        HandleNewConnection();
    });
    event_loop_t.run();
    M_S_POOl.Stop();
    return 0;
}

void CTL::RTSPServer::Stop() {
    ServerFlag.store(false, std::memory_order_release);
    event_loop_t.stop();

    // 停止所有拉流分发线程
    std::vector<String> paths;
    {
        std::lock_guard<CCMutex> lock(clientMutex_);
        const auto keys = pullThreadFlags_.keys();
        paths.assign(keys.begin(), keys.end());
    }
    for (const auto& path : paths) {
        stopPullDistribution(path);
    }
}

void CTL::RTSPServer::setMediaStreamCallback(const MediaStreamCallback &fun) {
    MediaStreamCallback_t = fun;
}
