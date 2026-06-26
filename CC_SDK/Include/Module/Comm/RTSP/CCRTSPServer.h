#ifndef SRCCTL_CC_RTSP_SERVER_H
#define SRCCTL_CC_RTSP_SERVER_H

#include <atomic>
#include <memory>
#include "CCRTP.h"
#include "EventLoop.h"
#include "CCThreadPool.h"
#include "CCRTSPDataChannel.h"
#include "CCSDP.h"
#include "CCRTCP.h"
#include "rtpsessionparams.h"
#include "CCRTPReceiver.h"
#include "CCRTPSender.h"


//OPTIONS,DESCRIBE,SETUP,TEARDOWN,PLAY,PAUSE,ANNOUNCE,RECORD
#define RTSP_REQ_OPTIONS "OPTIONS"
#define RTSP_REQ_DESCRIBE "DESCRIBE"
#define RTSP_REQ_SETUP "SETUP"
#define RTSP_REQ_TEARDOWN "TEARDOWN"
#define RTSP_REQ_PLAY "PLAY"
#define RTSP_REQ_PAUSE "PAUSE"
#define RTSP_REQ_ANNOUNCE "ANNOUNCE"
#define RTSP_REQ_RECORD "RECORD"
#define RTSP_REQ_SET_PARAMETER "SET_PARAMETER"
#define RTSP_REQ_SET_GET_PARAMETER "GET_PARAMETER"
#define RTSP_REQ_SET_GET_REDIRECT "REDIRECT"

namespace CTL{
    struct RTSP_MEDIA {
        CTL::ByteArray buffer;
        const CJ_RTPPacket *packet = nullptr;
        CTL::String url;
        uint8_t payloadType = 0;
        uint32_t timestamp = 0;
        bool marker = false;
    };
    using RTSP_REQ_FUN = std::function<void(RTSP_Request&,RTSP_Response*)>;
    using MediaStreamCallback = std::function<void(const CTL::RTSP_MEDIA&,int streamID)>;

    // 拉流数据源回调：用户实现此接口来提供媒体数据（适用于无推流端、仅拉流的场景）
    // 返回 false 表示数据源已结束
    using PullDataSource = std::function<bool(const String& path, RTSP_MEDIA& outMedia)>;

    // 拉流 SDP 回调：为纯拉流路径提供 SDP 媒体描述（m= 行及 a= 属性）
    // 返回空字符串表示无 SDP 信息
    using PullSDPCallback = std::function<String(const String& path)>;

    class RTSP_Stream {
        RTPReceiver receiver;
        RTPSender sender;
        bool isReceiverMode = true; // true:接收模式(客户端推流) false:发送模式(服务器推流)
        CTL::Queue<RTSP_MEDIA> RTP_Queue;
    public:
        ~RTSP_Stream();
        bool Init(int rtp_port,int rtcp_port,const std::string &ip);
        bool InitSender(const std::string &dest_ip, int dest_rtp_port,
            int dest_rtcp_port,uint16_t Base_port,const std::string& local_ip);
        int streamID = 0;
        int StreamType = 0; // 0:推流 1:拉流
        CTL::String SDP; // SDP
        void setFun(const RTP_RecvFun& fun);
        bool SendRTPPacket(const CTL::ByteArray& data, uint8_t payloadType,
                  uint32_t timestamp, bool marker);
        bool IsReceiverMode() const { return isReceiverMode; }
        void addData(CTL::RTSP_MEDIA data);
        CTL::AutoDestruct<CTL::RTSP_MEDIA> getData();
        size_t dataSize();
    };
    class RTSPClient {
        CTL::Map<int,RTSP_Stream*> Streams;
    public:
        ~RTSPClient();
        int StreamType = 0; // 0:推流 1:拉流
        String SessionId;
        String Path = "";
        String IP;
        int Port = 0;
        void addStream(RTSP_Stream* stream);
        RTSP_Stream* getStream(int id);
        int getStreamLength();
        // 清理所有流
        void clearStreams();
    };
    class RTSPServer{
        ServerSocket* server = nullptr;
        std::atomic<bool> ServerFlag{true};
        EventLoop event_loop_t{};
        ThreadPool M_S_POOl{};
        size_t M_Pool_Size = 128,M_Pool_Max = 1024;
        int M_Pool_KeepAliveTime = 1000;
        IPVX IPvx = IPV4;
        String ServerIP;
        int startPort = 32000,endPort = 33000;

        // ── 线程安全 ──
        mutable CCMutex clientMutex_;       // 保护 Clients、PushStreamingSession、PullStreamingSession
        mutable CCMutex pullQueueMutex_;    // 保护 pullDataQueue_
        static CCMutex port_mutex;

        Map<String,String> PushStreamingSession;          // path -> pushSessionId
        Map<String,Map<String,String>> PullStreamingSession; // path -> {pullSessionId -> pullSessionId}
        Map<String,RTSPClient*> Clients;                   // sessionId -> RTSPClient*

        RTSP_REQ_FUN REQ_CallBack = nullptr;
        int PreCachedNumber = 5;
        MediaStreamCallback MediaStreamCallback_t;

        // ── 拉流数据注入 ──
        // 每个 path 对应一个数据队列，外部通过 pushMediaData() 注入数据
        // 使用 shared_ptr 因为 Queue 包含 mutex，不可拷贝
        Map<String, std::shared_ptr<CTL::Queue<RTSP_MEDIA>>> pullDataQueue_;
        // 每个 path 对应一个分发线程停止标志
        Map<String, std::shared_ptr<std::atomic<bool>>> pullThreadFlags_;
        // 拉流数据源回调（可选，用于无推流端的纯拉流场景）
        PullDataSource pullDataSource_;
        // 拉流 SDP 回调（可选，为纯拉流路径提供 SDP 媒体描述）
        PullSDPCallback pullSDPCallback_;

        // ── 配置项 ──
        int pullDistributeIntervalMs_ = 3;   // 拉流分发轮询间隔(ms)
        int pullDataQueueMaxSize_ = 512;      // 拉流数据队列最大长度

    public:

    private:
        void HandleNewConnection();
        void ProcessingClientRequests(ClientSocket* client);
        static void getClientPort(int* rtp, int* rtcp, const String& transport);
        static void removeRTSPSession(const String& sessionId);
        static String generateSessionId();
        void ReadData(const CTL::ByteArray& data,const CJ_RTPPacket* packet,RTSPClient* client,int streamID) const;
        void Push(const String &sessionId,int streamID);

        // ── 拉流分发 ──
        // 从 pullDataQueue_[path] 取数据发送给所有拉流客户端
        void PullDistribute(const String& path, std::shared_ptr<std::atomic<bool>> runningFlag);
        // 启动拉流分发线程
        void startPullDistribution(const String& path);
        // 停止拉流分发线程
        void stopPullDistribution(const String& path);

        // ── 线程安全的客户端操作 ──
        RTSPClient* getClient(const String& sessionId);
        void removeClient(const String& sessionId);
        void addClient(const String& sessionId, RTSPClient* client);

    public:
        RTSPServer();
        ~RTSPServer();
        bool Init(const InetAddress& address,int Port);
        void SetCallBack(const RTSP_REQ_FUN& fun);
        int Running();
        void Stop();
        void setMediaStreamCallback(const MediaStreamCallback& fun);

        // ── 拉流数据注入 API ──
        // 方式1: 推送原始数据到指定路径的所有拉流客户端
        // path: 流路径（如 "/streamAudio"），与 ANNOUNCE/DESCRIBE 中的路径一致
        // data: 媒体数据（不含 RTP 头，由内部 RTPSender 封装）
        bool pushMediaData(const String& path, const CTL::ByteArray& data,
                          uint8_t payloadType, uint32_t timestamp, bool marker);
        // 方式2: 推送完整的 RTSP_MEDIA 结构
        bool pushMediaData(const String& path, const RTSP_MEDIA& media);

        // 方式3: 设置拉流数据源回调（适用于从文件/设备持续读取数据的场景）
        // 当有拉流客户端 PLAY 且无推流端时，服务器通过此回调获取数据
        void setPullDataSource(const PullDataSource& source);

        // 方式4: 设置拉流 SDP 回调（为纯拉流路径提供 codec/媒体描述信息）
        void setPullSDPCallback(const PullSDPCallback& cb) { pullSDPCallback_ = cb; }

        // ── 配置 API ──
        void setPortRange(int start, int end) { startPort = start; endPort = end; }
        void setPullDistributeInterval(int ms) { pullDistributeIntervalMs_ = ms; }
        void setPullDataQueueMaxSize(int size) { pullDataQueueMaxSize_ = size; }
        void setThreadPoolConfig(size_t minSize, size_t maxSize, int keepAliveMs) {
            M_Pool_Size = minSize; M_Pool_Max = maxSize; M_Pool_KeepAliveTime = keepAliveMs;
        }
    };
}

#endif
