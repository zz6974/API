#ifndef CC_WEBSOCKET_H
#define CC_WEBSOCKET_H

#include "Socket/EPSocket.h"
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <CCThreadPool.h>
#include "EventLoop.h"
#include "CC.h"
#include <ws_packet.h>
#include <string>
#include <random>
#include "CCTimer.h"
#include "TL/Map.h"
#include "TL/Queue.h"


// WebSocket 类格式的回调函数宏定义
#define CC_WSFunClass(Fun) [this](auto && PH1) { Fun(std::forward<decltype(PH1)>(PH1)); }
// WebSocket 普通函数格式的回调函数宏定义
#define CC_WSFun(Fun) Fun
// WebSocket 类格式的回调函数宏定义
#define CC_WSFunClass_CXX11(Fun) [this](CTL::WebSocketInfo& info) { Fun(info); }


namespace CTL{
    /**
     * WebSocket 连接信息类
     * 用于存储 WebSocket 连接的相关信息和数据
     */
    class WebSocketInfo {
        WebSocketPacket m_packet;    // WebSocket 数据包
        CTL::Socket m_sock;          // Socket 连接
        ByteArray m_buffer;          // 数据缓冲区
        bool Con_Flag = false;       // 连接标志
        SOCKET ID = -1;                // 客户端 ID
        int m_count = 0;
        String URL_Path = "/";
        bool m_status = false;
        mutable std::mutex m_mutex_id;
    public:
        virtual ~WebSocketInfo() = default;

        /**
         * 获取 WebSocket 数据包
         * @return WebSocket 数据包引用
         */
        WebSocketPacket& GetPacket();

        /**
         * 获取 Socket
         * @return Socket 引用
         */
        CTL::Socket& GetSock();

        /**
         * 获取 WebSocket 数据包（常量版本）
         * @return 常量 WebSocket 数据包引用
         */
        [[nodiscard]] const WebSocketPacket& GetPacket() const;

        /**
         * 设置 WebSocket 数据包
         * @param packet 要设置的 WebSocket 数据包
         */
        void SetPacket(const WebSocketPacket& packet);

        /**
         * 设置 Socket
         * @param socket 要设置的 Socket
         */
        void SetSock(const CTL::Socket& socket);

        /**
         * 设置缓冲区
         * @param buffer 要设置的数据缓冲区
         */
        void SetBuffer(ByteBuffer& buffer);

        /**
         * 获取缓冲区
         * @return 数据缓冲区引用
         */
        ByteArray GetBuffer() const;

        /**
         * 获取连接标志
         * @return 连接标志
         */
        [[nodiscard]] bool GetConnectFlag() const;

        /**
         * 设置连接标志
         * @param F 连接标志值
         */
        void SetConnectFlag(bool F);

        /**
         * 设置客户端 ID
         * @param ID 客户端 ID 值
         */
        void SetID(SOCKET ID);

        SSL* ssl = nullptr;  // SSL 对象指针
        void* UserParam = nullptr; // 用户自定义参数指针

        /**
         * 获取客户端 ID
         * @return 客户端 ID
         */
        [[nodiscard]] SOCKET GetID() const;

        /**
         * 向目标端发送数据
         * @param data 要发送的数据
         * @param size 数据大小
         * @return 发送是否成功
         */
        virtual bool Write(const char* data, size_t size);

        /**
         * 向目标端发送文本消息
         * @param text 要发送的文本消息
         * @return 发送是否成功
         */
        virtual bool SendText(const CTL::String& text);

        /**
         * 向目标端发送二进制数据
         * @param binaryData 要发送的二进制数据
         * @return 发送是否成功
         */
        virtual bool SendBinary(const ByteArray& binaryData);

        /**
         * 向目标端发送Ping二进制数据
         * @param binaryData 要发送的二进制数据
         * @return 发送是否成功
         */
        virtual bool SendPing(const ByteArray& binaryData);

        /**
         * 向目标端发送Pong二进制数据
         * @param binaryData 要发送的二进制数据
         * @return 发送是否成功
         */
        virtual bool SendPong(const ByteArray& binaryData);

        /**
         * 写入 SSL 响应数据
         * @param ssl SSL 对象指针
         * @param buffer 要发送的数据
         * @param size 数据大小
         * @return 发送结果标志
         */
        static int WriteSSLResponse(SSL* ssl, const char* buffer, size_t size);

        /**
         * 关闭与客户端或服务端的连接
         * @param statusCode 关闭状态码
         * @param reason 关闭原因
         */
        void CloseConnection(uint16_t statusCode = 1000, const CTL::String& reason = "");

        /**
         * 更新连接状态Ping心跳内收到一个Ping消息就更新连接状态
         */
        void StatusUpdate();

        /**
         * 重置连接状态Ping心跳内收到一个Ping消息就更新连接状态
         */
        void StateReset();

        /**
         *
         * @return 获取Ping心跳个数
         */
        int GetStatus() const;

        /**
         * 设置客户端路由
         * @param path 设置路由
         */
        void SetURLPath(const String& path);

        /**
         * 获取客户端路由
         * @return 返回路由字符串
         */
        String GetURLPath();

        /**
         * 获取是否正在发送数据
         * @return 返回结果
         */
        bool GetSendStatus() const;
    };
    /**
     * WebSocket 服务器类
     * 用于创建和管理 WebSocket 服务器，处理客户端连接和消息
     */
    using RequestFunc = std::function<void(WebSocketInfo&)>;
    class WebSocketBind {
    public:
        WebSocketBind() = default;
        CTL::String URL = "";
        RequestFunc OnOpenFun;
        RequestFunc OnCloseFun;
        RequestFunc OnMessageFun;
        RequestFunc OnPingFun;
        RequestFunc OnPongFun;
        RequestFunc OnErrorFun;
    };
    class WebSocket {
        mutable std::mutex m_mutex_t;
    public:
        /**
         * 初始化 WebSocket 服务器
         * @param ip 服务器 IP 地址
         * @param port 服务器端口号
         * @param ChokeUp 是否启用流量控制
         * @return 初始化是否成功
         */
        bool Init(const CTL::String& ip,int port,bool ChokeUp = true);

        /**
         * 复制初始化 WebSocket 服务器
         * @param bind 绑定对象
         * @return 初始化是否成功
         */
        bool Init(const WebSocketBind& bind);

        /**
         * 设置 SSL
         * @param ssl_ctx_t  SSL_CTX 对象指针
         */
        void SetSSL(SSL_CTX* ssl_ctx_t);

        /**
         * 运行 WebSocket 服务器
         * @return 运行状态码
         */
        int Running();

        /**
         * 设置事件处理函数
         * @param URL 请求路径
         * @param OnOpenFun 客户端连接成功回调函数
         * @param OnCloseFun 服务端关闭连接回调函数
         * @param OnMessageFun 客户端接收消息回调函数
         * @param OnErrorFun 连接错误回调函数
         * @param OnPingFun 客户端接收到 Ping 消息回调函数
         * @param OnPongFun 客户端接收到 Pong 消息回调函数
         */
        void OnBind(const CTL::String& URL,const RequestFunc& OnOpenFun = nullptr,const RequestFunc& OnCloseFun = nullptr,
            const RequestFunc& OnMessageFun = nullptr,const RequestFunc& OnErrorFun = nullptr,
            const RequestFunc& OnPingFun = nullptr,const RequestFunc& OnPongFun = nullptr);

        /**
         * 处理客户端请求
         * @param client 客户端 Socket
         * @param ev 事件信息
         * @param Data 请求数据
         * @param ssl SSL 对象指针
         */
        void ProcessingClientRequests(CTL::Socket& client,const CTL::String& Data,SSL* ssl = nullptr);

        /**
         * 获取所有客户端ID列表
         * @return 包含所有客户端ID的列表
         */
        CCVector<int> GetClientListAll();

        /**
         * 根据 ID 获取客户端信息
         * @param ID 客户端 ID
         * @return 对应的 WebSocketInfo 对象
         */
        WebSocketInfo* GetClient(int ID);

        /**
         * 初始化 SSL
         * @return 初始化是否成功
         */
        bool InitSSL();

        /**
         * 加载 SSL 证书
         * @param CertPath 证书文件路径
         * @return 加载是否成功
         */
        bool LoadSSLCertificate(const CTL::String& CertPath) const;

        /**
         * 加载 SSL 私钥
         * @param KeyPath 私钥文件路径
         * @param Passwd 私钥密码
         * @return 加载是否成功
         */
        bool LoadSSLKey(const CTL::String &KeyPath, const CTL::String &Passwd) const;

        /**
         * 加载 SSL 客户端验证证书
         * @param CA_Path 客户端验证证书文件路径
         * @return 加载是否成功
         */
        bool LoadSSLVerify(const CTL::String& CA_Path) const;

        /**
         * 停止 WebSocket 服务器或客户端
         * */
         void Stop();

        /**
         * 生成 sec-websocket-key
         * @return 生成的 sec-websocket-key
         */
        static std::string generate_sec_websocket_key();

        /**
         *
         * @param str 需要去除空格的字符串
         * @return 去除空格后的字符串
         */
        static std::string& trim(std::string& str);

        /**
         * 读取缓冲区数据
         * @param ssl SSL 对象指针
         * @param client 客户端 Socket
         * @param buf 缓冲区指针
         * @param size 缓冲区大小
         * @return 读取的数据大小
         */
        static ByteHander ReadBuffer(SSL* ssl,CTL::Socket& client,char* buf,ByteHander size);

        /**
         * 设置Ping心跳间隔
         * @param Seconds 设置秒
         */
        void SetTimingTime(const unsigned long  Seconds) {
            m_timer_count =  Seconds;
        }

        /**
         *
         * @return Ping心跳间隔
         */
        unsigned long GetTimingTime() const {
            return m_timer_count;
        }

        /**
         * 设置Ping心跳开关
         * @param PingUp Ping心跳开关
         */
        void SettingPing(const bool PingUp) {
            Tick_m =  PingUp;
        }

        /**
         * 初始化WebSocket服务
         */
        void InitService();
    protected:
        CTL::Socket m_sock;                             // 服务器 Socket
        EventLoop event_loop_t;                                      // EventLoop 对象
        bool ServerFlag = true,Client = false,ClientOK = false,Tick_m = true;  // 标志位
        ThreadPool M_S_POOl{};                          // 线程池
        size_t M_Pool_Size = 128,M_Pool_Max = 1024;     // 线程池参数
        int M_Pool_KeepAliveTime = 1000;                // 线程池保活时间
        CCMap<CTL::String,RequestFunc> URLOpenFun;      // URL 打开回调函数映射
        CCMap<CTL::String,RequestFunc> URLCloseFun;     // URL 关闭回调函数映射
        CCMap<CTL::String,RequestFunc> URLMessageFun;   // URL 消息回调函数映射
        CCMap<CTL::String,RequestFunc> URL_ERROR_Fun;   // URL 错误回调函数映射
        CCMap<CTL::String,RequestFunc> URL_Ping_Fun;    // URL Ping回调函数映射
        CCMap<CTL::String,RequestFunc> URL_Pong_Fun;    // URL Pong回调函数映射
        CCMap<CTL::String,bool> URLS;                   // URL 标志位映射
        Map<int,WebSocketInfo*> Clients;               // 客户端信息映射
        // CCMutex m_mutex;                          // 共享互斥锁
        SSL_CTX* ssl_ctx = nullptr;                     // SSL 上下文
        Timer m_timer;                                  // 定时器
        unsigned long m_timer_count = 120;
    private:
        // 接受客户端连接
        void AcceptingClient();

        /**
         * 获取客户端请求
         * @param client 客户端 Socket
         * @param ssl SSL 对象指针
         * @return 请求数据
         */
        CTL::String GetRequest(CTL::Socket& client,SSL* ssl = nullptr);

        /**
         * 打印消息
         * @param str 要打印的消息
         */
        void MessagePrint(const CTL::String& str);

        /**
         * 分配客户端 ID
         * @return 分配的客户端 ID
         */
        int AllocationID();

        /**
         * 添加客户端
         * @param ws 要添加的 WebSocketInfo 对象
         */
        void AddClient(WebSocketInfo* ws);

        /**
         * 删除客户端
         * @param ws 要删除的 WebSocketInfo 对象
         */
        void DeleteClient(const int ID);

        /**
         * 创建新的 SSL 对象
         * @param client 客户端 Socket
         * @return 新的 SSL 对象指针
         */
        SSL* NewSSL(const CTL::Socket& client) const;

        /**
         * 定时器回调函数
         */
        void Tick();
    };
    class WebSocketClient {
        // 请求处理函数类型
        using RequestFunc = std::function<void(WebSocketInfo&)>;
        WebSocketInfo LocalClientInfo;// 客户端连接信息
        /**
         * WebSocket 客户端信息结构体
         * 用于存储连接到 WebSocket 服务器的客户端信息
         */
        struct WebSocketClient_Info {
            CTL::String IP;          // 客户端 IP 地址
            int Prot = 0;            // 客户端端口号
            CTL::String Path;        // 请求路径
            CTL::String Protocol;    // 使用的协议
        };
        ByteBuffer ClientBuffer;                   // 客户端缓冲区
        RequestFunc OnErrorFun,OnOpenFun,OnCloseFun,OnMessageFun,OnPingFun,OnPongFun; // 回调函数
        String ClientKey;
        bool ServerFlag = true,Client = false,ClientOK = false;  // 标志位
        Socket m_sock;
        SSL_CTX* ssl_ctx = nullptr;                // SSL 上下文
    public:
        /**
         * 连接 WebSocket 服务器
         * @param ip 服务器 IP 地址
         * @param port 服务器端口号
         * @param URL 请求路径
         * @return 连接是否成功
         */
        bool Connect(const CTL::String& ip,int port,const String& URL);

        /**
         * 连接 WebSocket 服务器
         * @param WsURL 完整的 WebSocket URL
         * @return 连接是否成功
         */
        bool Connect(const CTL::String& WsURL);

        /**
         * 设置客户端事件处理函数
         * @param OnOpenFun 客户端连接成功回调函数
         * @param OnCloseFun 服务端关闭连接回调函数
         * @param OnMessageFun 客户端接收消息回调函数
         * @param OnErrorFun 连接错误回调函数
         * @param OnPingFun 客户端接收到 Ping 消息回调函数
         * @param OnPongFun 客户端接收到 Pong 消息回调函数
         */
        void OnBind(const RequestFunc& OnOpenFun = nullptr,const RequestFunc& OnCloseFun = nullptr,
        const RequestFunc& OnMessageFun = nullptr,const RequestFunc& OnErrorFun = nullptr,
        const RequestFunc& OnPingFun = nullptr,const RequestFunc& OnPongFun = nullptr);

        /**
         * 运行 WebSocket 客户端 阻塞运行
         */
        int Running();

        /**
         * 停止 WebSocket 客户端
         */
        void Stop();

        /**
         * @param data 要发送的数据
         * @return 发送是否成功
         */
        bool SendBinary(const ByteArray& data);

        /**
         * @param text 要发送的文本数据
         * @return 发送是否成功
         */
        bool SendText(const String& text);

        /**
         * 初始化 SSL
         * @return 初始化是否成功
         */
        bool InitSSL();

        /**
         * 加载 SSL 证书
         * @param CertPath 证书文件路径
         * @return 加载是否成功
         */
        bool LoadSSLCertificate(const CTL::String& CertPath) const;

        /**
         * 加载 SSL 私钥
         * @param KeyPath 私钥文件路径
         * @param Passwd 私钥密码
         * @return 加载是否成功
         */
        bool LoadSSLKey(const CTL::String &KeyPath, const CTL::String &Passwd) const;

        /**
         * 加载 SSL 客户端验证证书
         * @param CA_Path 客户端验证证书文件路径
         * @return 加载是否成功
         */
        bool LoadSSLVerify(const CTL::String& CA_Path) const;
    private:
        /**
         * 解析 WebSocket 链接
         * @param url 完整的 WebSocket URL
         * @return 解析后的 WebSocketClient 结构体
         */
        static WebSocketClient_Info ParsingLink(const std::string& url);

        /**
         * 发送握手请求
         * @param ip_ 服务器 IP 地址
         * @param port_ 服务器端口号
         * @param Path 请求路径
         * @param ssh 未知参数
         * @return 发送是否成功
         */
        [[nodiscard]] bool Send_Handshake(const String &ip_, int port_,const String& Path, const String &ssh) const;

        /**
         * 接收握手响应
         * @return 接收是否成功
         */
        bool Recv_Handshake();
    };
    class WebClient : public WebSocketInfo {
        void write(const CTL::ByteArray& data) const;
        EPClient* client = nullptr;
    public:
        SOCKET id = -1;
        void set_client(EPClient* cl);
        void close();
        bool Write(const char *data, size_t size) override;
        // bool SendText(const CTL::String &text) override;
        // bool SendBinary(const ByteArray &binaryData) override;
        // bool SendPing(const ByteArray &binaryData) override;
        // bool SendPong(const ByteArray &binaryData) override;
    };
    using RequestFunc2 = std::function<void(WebClient&)>;
    class WSBind {
    public:
        WSBind() = default;
        CTL::String URL = "";
        RequestFunc2 OnOpenFun;
        RequestFunc2 OnCloseFun;
        RequestFunc2 OnMessageFun;
        RequestFunc2 OnPingFun;
        RequestFunc2 OnPongFun;
        RequestFunc2 OnErrorFun;
    };
    class WebSocketServer {
    private:
        bool Init_Bind = false;
        mutable std::mutex m_mutex_t;
        bool Flag = false;
        bool Tick_m = true;
        EPServer* m_server = nullptr;
        Timer m_timer;
        CCMap<CTL::String,RequestFunc2> URLOpenFun;      // URL 打开回调函数映射
        CCMap<CTL::String,RequestFunc2> URLCloseFun;     // URL 关闭回调函数映射
        CCMap<CTL::String,RequestFunc2> URLMessageFun;   // URL 消息回调函数映射
        CCMap<CTL::String,RequestFunc2> URL_ERROR_Fun;   // URL 错误回调函数映射
        CCMap<CTL::String,RequestFunc2> URL_Ping_Fun;    // URL Ping回调函数映射
        CCMap<CTL::String,RequestFunc2> URL_Pong_Fun;    // URL Pong回调函数映射
        CCMap<CTL::String,bool> URLS;                   // URL 标志位映射
        CCMap<SOCKET,WebClient*> Clients;
    public:
        WebSocketServer();
        ~WebSocketServer();
        void setEPSocket(EPServer* server_t);
        bool Init(const CTL::String& ip,int port);
        bool InitBind(const WSBind& bind);
        bool InitSSL(const std::string& cert_file,const std::string& key_file,const std::string& pass) const;
        void HandleClient(const SOCKET& fd, const CTL::String& request);
        void InitTick(unsigned long tick_time = 60);
        int Running();
        void Stop();
        WebClient* getClient(const SOCKET& id);
        std::vector<SOCKET> getClientFds() const;
    private:
        void put(WebClient* client);
        void remove(WebClient* client);
        void OnBind(const CTL::String& URL,const RequestFunc2& OnOpenFun = nullptr,const RequestFunc2& OnCloseFun = nullptr,
            const RequestFunc2& OnMessageFun = nullptr,const RequestFunc2& OnErrorFun = nullptr,
            const RequestFunc2& OnPingFun = nullptr,const RequestFunc2& OnPongFun = nullptr);
        CTL::String getRequest(const SOCKET& id) const;
        void Tick();
    };
}

#endif
