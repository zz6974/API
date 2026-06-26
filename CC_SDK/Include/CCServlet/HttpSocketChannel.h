#ifndef CC_SOCKET_CHANNEL_H
#define CC_SOCKET_CHANNEL_H

#include "Socket/EPSocket.h"
#include "Socket/CCClientSocket.h"
#include "CCLogger.h"
#include "EventLoop.h"

#define SocketClientNULL "Error_10030"
#define CC_R_BUFFER_SIZE 1
#define CC_S_BUFFER_SIZE 1024
#define CC_R_INFO_SIZE 256
#define File_Buffer_Max 4096

namespace CTL {
    class HttpSocketChannel {
        // SSL_Client* ssl_client_t = nullptr;
        EPClient* client_t = nullptr;
        unsigned long TimingTime = 1000;
        Logger_Level level = Debug;
        EventLoop* m_eventLoop = nullptr;
        static CCMutex _mutex_;
        bool SSL_Free = true;
        bool isTimed = false;
        bool send_status = false, send_start = false;
        using ReadCallBack = std::function<void()>;
        CCMutex _mutex_read;
        ReadCallBack read_cb;
        size_t start_time = 0;
        bool is_release = true;
    public:
        String URL = "";
        bool keepAliceOut = false;
        int KeepAliceCount = 0;
        uint32_t KeepAliceTime = 0;
    private:
        void MessagePrint(const CTL::String& str,Logger_Level level = Debug) const;
    public:
        HttpSocketChannel(EPClient* client);
        ~HttpSocketChannel();
        [[nodiscard]] EPClient* getClient() const;
        [[nodiscard]] String readHeader();
        int receive(char* buffer,size_t size) const;
        void setTimeOut(int ms);
        void setReadFun(const ReadCallBack& cb);
        int getTimeout() const;
        void release();
        void setStartTime(size_t time);
    };
}

#endif
