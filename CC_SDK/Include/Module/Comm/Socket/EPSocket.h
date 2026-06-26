#ifndef SRCCTL_EP_SOCKET_H
#define SRCCTL_EP_SOCKET_H

#include "Socket/CCSocket.h"
#include "OpenSSL/CCSSL.h"
#include "CCEpoll.h"
#include "CCThreadPool.h"
#include "TL/Queue.h"
#include "TL/Map.h"


namespace CTL {
    using EPServer_Open = std::function<void(const SOCKET& fd)>;
    using EPServer_Message = std::function<void(const SOCKET& fd)>;
    using EPServer_Close = std::function<void(const SOCKET& fd)>;
    struct EP_Packet {
        CTL::ByteArray data;
        ByteHander status; // -1:no data -2:close
    };
    class EPClient {
        SOCKET fd;
        Socket socket;
        std::deque<CTL::ByteArray> send_queue;
        std::deque<CTL::EP_Packet> recv_queue;
        mutable std::mutex send_queue_mutex;
        mutable std::mutex recv_queue_mutex;
        mutable std::mutex epoll_mutex;
        SSL_Client *ssl_client_t = nullptr;
        CC_Epoll_t epfd;
        bool should_close_after_send = false;
        CCHostInfo client_host = {};
        int padding = 0;
    public:
    private:
        int send_t(const char *buf, int len);
        long read(char* buffer, long length) const;
    public:
        EPClient();
        ~EPClient();
        void setFd(SOCKET fd_t,CC_Epoll_t epfd_t = {});
        void setSSL(SSL_Client *ssl_client);
        SOCKET getFd() const;
        void close();
        EP_Packet recv(unsigned long length = 1024);
        bool is_recv_readable() const;
        bool flash();
        void write(const CTL::ByteArray& data);
        void modifyEpollEvent(uint32_t events) const;
        void flash_close() { should_close_after_send = true; }
        bool get_flash_close() const { return should_close_after_send; }
        void setClientHost(const CCHostInfo& info);
        int get_padding() const;
        CCHostInfo getClientHost() const;
        bool is_ssl() const;
        SSL* getSSL() const;
    };
    class EPServer {
        Socket socket = {};
        mutable std::mutex clients_mutex;
        std::map<SOCKET,EPClient*> clients;
        bool Flag = false;
        CTL::IPVX ipvx_t = IPV4;
        CC_Epoll_t epfd;
        EPServer_Open open_func;
        EPServer_Message message_func;
        EPServer_Close close_func;
        CTX_SSL SSL_CTX;
        CTL::ThreadPool thread_pool;
        int thread_num = -1;
        bool thread_flag = false;
    public:
    private:
        void putClient(EPClient* client);
        void remove(const EPClient* client);
        void handle_accept(SOCKET listen_fd);
        void create_client_connection(SOCKET fd, const CCHostInfo& client_info);
        int handle_client_event(const epoll_event& event);
        void handle_read_event(EPClient* client_t);
        void handle_write_event(EPClient* client_t);
        void remove_client(SOCKET fd, EPClient* client_t);
        void cleanup_all_clients();
        void epoll_running(SOCKET listen_fd);
    public:
        EPServer();
        ~EPServer();
        int listenInit(const std::string& ip,int port_t,CTL::IPVX ipvx = CTL::IPVX::IPV4);
        bool initSSL(const std::string& cert_file,const std::string& key_file,const std::string& pass);
        int running();
        void stop();
        bool is_client_connect(SOCKET fd);
        void setOpenFunc(const EPServer_Open &func);
        // void setMessageFunc(const EPServer_Message &func);
        void setCloseFunc(const EPServer_Close &func);
        EPClient* getClient(SOCKET fd);
        void setMultithreading(bool flag,int NumberOfThreads = -1);
        std::vector<SOCKET> getClientFds();
    };
}

#endif
