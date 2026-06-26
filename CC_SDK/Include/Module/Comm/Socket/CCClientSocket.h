#ifndef CLIENT_SOCKET_H
#define CLIENT_SOCKET_H

#include "CCSocket.h"
#include "CCByteArray.h"

namespace CTL {
    class SocketInputStream {
        Socket* socket = nullptr;
    public:
        explicit SocketInputStream(Socket* sock);
        int read(char* buffer, size_t length) const;
        int read() const;
        int read(ByteArray& buffer, size_t length) const;
        bool available() const;
    };
    class SocketOutputStream {
        Socket* socket = nullptr;
    public:
        explicit SocketOutputStream(Socket* sock);
        int write(const void* buffer, size_t length) const;
        bool write(ByteArray& buffer) const;
        bool write(const String& str) const;
    };
    class ClientSocket {
        Socket* socket = nullptr;
        SocketInputStream* inputStream = nullptr;
        SocketOutputStream* outputStream = nullptr;
        IPVX IP_x = IPVX::IPV4;
        std::string hostAddress;
        bool isClosed_t = true, isConnect_t = false;
        int Port = 0;
    public:
    private:
    public:
        ClientSocket();
        ~ClientSocket();
        ClientSocket(const InetAddress& address,int Port);
        explicit ClientSocket(Socket* sock);
        bool connect(const InetAddress& address,int Port);
        void close();
        bool isConnect() const;
        bool isClosed() const;
        SocketInputStream* getInputStream() const;
        SocketOutputStream* getOutputStream() const;
        Socket* getSocket() const;
        int getPort() const;
        InetAddress getHostAddress() const;
    };
}

#endif
