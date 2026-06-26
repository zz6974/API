#ifndef SERVER_SOCKET_H
#define SERVER_SOCKET_H

#include "CCSocket.h"
#include "CCClientSocket.h"

namespace CTL {
    class ServerSocket {
        Socket* socket = nullptr;
        IPVX IP_x = IPVX::IPV4;
        std::string hostAddress;
        bool isClosed_t = true, isBound_t = false,isListen_t = false;
        int Port = 0;
    public:
    private:
        bool Init(int Port,const InetAddress& address);
    public:
        ServerSocket();
        ~ServerSocket();
        explicit ServerSocket(int Port, const InetAddress &address = InetAddress::getAnyAddress());
        bool bind(int Port,const InetAddress& address = InetAddress::getAnyAddress());
        bool listen(int backlog = 50);
        void close();
        bool isBound() const;
        bool isClosed() const;
        Socket* getSocket() const;
        ClientSocket* accept() const;
    };
}


#endif
