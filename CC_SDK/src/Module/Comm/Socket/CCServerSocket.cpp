#include "Socket/CCServerSocket.h"

bool CTL::ServerSocket::Init(const int Port, const InetAddress &address) {
    socket = new Socket();
    return bind(Port,address);
}

CTL::ServerSocket::ServerSocket() {
    socket = new Socket();
}

CTL::ServerSocket::~ServerSocket() {
    delete socket;
}

CTL::ServerSocket::ServerSocket(const int Port, const InetAddress &address) {
    Init(Port,address);
}

bool CTL::ServerSocket::bind(const int Port, const InetAddress &address) {
    if (socket) {
        this->IP_x = address.getIPx();
        this->Port = Port;
        this->hostAddress = address.getHostAddress();
        socket->Init(IP_x, TORU::TCP, TYPE::STREAM);
        this->isBound_t = socket->Bind(hostAddress.c_str(), Port);
        if (this->isBound_t) {
            this->isClosed_t = false;
        }
        return this->isBound_t;
    }
    return false;
}

bool CTL::ServerSocket::listen(const int backlog){
    if (socket) {
        this->isListen_t = socket->Listen(backlog);
        if (this->isListen_t) {
            this->isClosed_t = false;
            this->isListen_t = true;
        }
        return this->isListen_t;
    }
    return false;
}

void CTL::ServerSocket::close() {
    if (socket) {
        this->isClosed_t = true;
        this->isBound_t = false;
        socket->Close();
    }
}

bool CTL::ServerSocket::isBound() const {
    return this->isBound_t;
}

bool CTL::ServerSocket::isClosed() const {
    return this->isClosed_t;
}

CTL::Socket * CTL::ServerSocket::getSocket() const {
    return this->socket;
}

CTL::ClientSocket * CTL::ServerSocket::accept() const {
    if (!this->isListen_t) {
        throw std::runtime_error("ServerSocket::accept() error: ServerSocket is not listening.");
    }
    const auto clientSocket = new Socket(socket->Accept());
    return new ClientSocket(clientSocket);
}
