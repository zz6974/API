#ifndef CCWeb_Servlet_H
#define CCWeb_Servlet_H
#pragma once

#include <CCThreadPool.h>
#include "CCResponse.h"
#include "WebSocket/CCWebSocket.h"
#include "HttpSocketChannel.h"
#include "Socket/CCServerSocket.h"
#include "CCAuth.h"

// 定义 WebServlet 类格式的回调函数宏，用于简化成员函数回调的编写
#define CC_ServletFunClass(Fun) [this](auto && PH1, auto && PH2) { Fun(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); }
// 定义 WebServlet 普通函数格式的回调函数宏，用于简化普通函数回调的编写
#define CC_ServletFun(Fun) Fun
// 定义 WebServlet 类格式的回调函数宏，用于简化成员函数回调的编写
#define CC_ServletFunClass_CXX11(Fun) [this](CTL::Request& req, CTL::Response& resp) { Fun(req, resp); }

namespace CTL {
    struct StaticResource {
        // 资源类型
        String Type;
        // 资源内容
        CCVector<char> Content;
        // 资源内容GZip
        CCVector<char> ContentGZip;
        // 资源名称
        String Name;
        // 资源路径
        String Path;
    };
    struct Event_Client {
        SOCKET fd{};
        ByteArray Data{};
    };
    // WebServlet 类，用于处理网络请求和管理服务器状态
    class WebServlet{
    public:
        // 使用 std::function 定义请求处理函数类型
        using RequestFunc = std::function<void(Request& ,Response&)>;
    private:
        // 网络服务器对象
        // ServerSocket* server = nullptr;
        EPServer* server = nullptr;
        // 服务器运行标志
        bool ServerFlag = true;
        EventLoop event_loop_t{};
        // 线程池对象，用于处理并发请求
        ThreadPool M_S_POOl{};
        // 线程池大小配置
        size_t M_Pool_Size = 128,M_Pool_Max = 1024;
        // 线程池中线程的保活时间
        int M_Pool_KeepAliveTime = 1000;
        // 静态资源的 URL 映射
        CCList<CTL::String> StaticPath;
        // 静态资源内容的映射
        CTL::Map<CTL::String,StaticResource> StaticResources;
        // Web 信号处理函数
        RequestFunc WebSignalFun;
        // 请求处理函数映射
        CTL::Map<std::string,RequestFunc> RequestFun;
        // CORS 头对象指针，用于处理跨域请求
        CORS* CORS_Header = nullptr;
        // SSL 上下文对象指针，用于 HTTPS 连接
        CTX_SSL SSL_CTX;
        // WebSocket 对象，用于处理 WebSocket 连接
        WebSocketServer m_WebSocket;
        // WebSocket 连接标志
        bool WebSocketFlag = false;
        // 请求超时时间 单位/毫秒
        unsigned long TimingTime = 1000;
        // 设置请求头缓冲区大小
        unsigned int ReadHBuff = 4096;
        IPVX IPvx = IPV4;
        bool source_map_ = false;
        unsigned long CacheTime = 86400 * 7; // 浏览器缓存时间
        time_t CacheTime_t = {};
        bool GZipFlag = true;
        int FileSize = 30 * KB_Unit_t;
        uint32_t KeepTimeOut = 60;
        uint32_t KeepMax = 30;
        // 全局认证提供者
        std::shared_ptr<IAuthProvider> m_globalAuthProvider;
        size_t MaxDataSize = 64 * MB_Unit_t;
    public:
        // 构造函数
        WebServlet();
        // 析构函数
        ~WebServlet();
        // 初始化服务器
        bool Init(const InetAddress &address,int Port) const;
        // 初始化服务器
        bool Init(const String& address,int Port) const;
        // 设置 WebSocket 配置
        bool SetWebSocket(const CTL::WSBind& bind);
        // 初始化 SSL 配置
        bool InitSSL(const std::string& cert_file,const std::string& key_file,const std::string& pass) const;
        // 运行服务器
        int Running(bool Ws = false,bool InitStaticFlag = true);
        // 停止服务器
        void Stop();
        // 添加静态资源路径
        bool AddStaticPath(const CTL::String& Path);
        // 添加CCSDK_PAK资源路径
        bool SetResources(const CTL::String& Path);
        // 设置 WebServlet 请求处理函数
        void SetWebServlet(const CTL::String& UrlPath,const RequestFunc& RFunc);
        // 设置 WebServlet 普通请求处理函数
        void SetWebServletFun(RequestFunc RFunc);
        // 设置带认证保护的 WebServlet
        void SetWebServletWithAuth(const CTL::String& UrlPath,
                                   std::shared_ptr<IAuthProvider> authProvider,
                                   const RequestFunc& RFunc);
        // 设置全局认证提供者（应用于所有未显式设置认证的路由）
        void SetGlobalAuthProvider(std::shared_ptr<IAuthProvider> authProvider);
        // 设置 CORS 配置
        void SetWebCORS(const CORS& cors);
        // 获取 WebSocket 对象
        WebSocketServer* GetWebSocket();
        // 设置请求超时时间
        void SetTimeout(unsigned long MS);
        // 设置接收数据头缓冲区大小
        void SetReadHeaderBufferSize(unsigned int Size);
        // 设置线程池参数
        void SetThreadPool(size_t Size,size_t Max,int KeepAliveTime);
        // 设置IP类型 必须在Init之前设置
        void SetIPType(IPVX IPvx);
        // 初始化静态资源
        void InitStatic(int fileSize = 30 * KB_Unit_t);
        // 设置最大文件上传数量
        static void SetMaxConcurrentFileUploads(int Num);
        // 设置资源调试
        void SetSourceMap(bool flag);
        // 设置静态资源缓存时间
        void SetCacheTime(unsigned long CacheTime_t);
        // 静态资源GZip
        void SetGZipFlag(bool Flag);
        // 设置长连接信息
        void SetKeepAlice(uint32_t KeepTimeOut_t,uint32_t KeepMax_t);
        // 设置最大数据接收大小
        void SetMaxDataSize(size_t Size);
    private:
        // 打印消息
        static void MessagePrint(const CTL::String& str);
        // 检查客户端权限
        bool Permission(const HttpSocketChannel* client_channel) const;
        // 处理客户端请求
        void ProcessingClientRequests(const SOCKET& fd);
        // 处理数据类型
        int ProcessingDataType(HttpSocketChannel* client_channel,Request& request);
        // 处理请求
        void HandleRequest(HttpSocketChannel* client_channel,Request& request);
        // 处理路径
        CTL::String ProcessPath(const CTL::String& path) const;
        // 初始化目录静态资源
        void DirInitStatic(const CTL::String& Path);
        // 检查路径是否包含映射后缀
        static bool hasMapSuffix(const std::string& path);
        // 移除路径中的映射后缀
        static std::string removeMapSuffix(const std::string& path);
        // 获取 Application/form-data 类型的数据
        CTL::String GetApp_Form_Data(const HttpSocketChannel* client_channel,Request& HTTPRequests);
        // 获取 Multipart/form-data 类型的数据信息
        CCMap<CTL::String,CTL::String> GetMultipart_Form_DataInfo(const HttpSocketChannel* client_channel,CTL::String& RecvMessage);
        void ErrorHandling(HttpSocketChannel* client_channel,Request& request,int ErrorCode = 404) const;
        static String FormatRequest(const String& str);
        ByteArray GetRequestAll(const HttpSocketChannel* client_channel,Request& HTTPRequests) const;
    };
}

#endif
