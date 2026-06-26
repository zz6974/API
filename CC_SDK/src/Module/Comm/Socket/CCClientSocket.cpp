#include "Socket/CCClientSocket.h"

#include "CCThread.h"

CTL::SocketInputStream::SocketInputStream(Socket *sock) {
    this->socket = sock;
}

int CTL::SocketInputStream::read(char *buffer, const size_t length) const {
    if (this->socket) {
        return this->socket->RecvData(buffer, length);
    }
    return -1;
}

int CTL::SocketInputStream::read() const {
    char buffer[1]{};
    const int ret = read(buffer, 1);
    if (ret > 0) {
        return buffer[0];
    }
    return -1;
}

int CTL::SocketInputStream::read(ByteArray &buffer, const size_t length) const {
    if (this->socket) {
        buffer.resize(length);
        const auto ret = this->socket->RecvData(buffer.buffer(), length);
        return ret > 0 ? ret : -1;
    }
    return -1;
}

bool CTL::SocketInputStream::available() const {
    if (this->socket) {
        return this->socket->isDataAvailable();
    }
    return false;
}

CTL::SocketOutputStream::SocketOutputStream(Socket *sock) {
    this->socket = sock;
}

int CTL::SocketOutputStream::write(const void *buffer, const size_t length) const {
    if (this->socket) {
        while (!CTL::Socket::IsSocketWritable(socket->Socketbit)) {
            Thread::Sleep(5);
        }
        int ret = 0;
        this->socket->SendByte(static_cast<const char *>(buffer),length,&ret);
        return ret;
    }
    return false;
}

bool CTL::SocketOutputStream::write(ByteArray &buffer) const {
    if (this->socket) {
        return this->socket->SendByte(buffer.buffer(),buffer.size());
    }
    return false;
}

bool CTL::SocketOutputStream::write(const String &str) const {
    if (this->socket) {
        return this->socket->SendByte(str.data(),str.size());
    }
    return false;
}

CTL::ClientSocket::ClientSocket() {
    socket = new Socket();
}

CTL::ClientSocket::~ClientSocket() {
    if (inputStream) {
        delete inputStream;
        inputStream = nullptr;
    }
    if (outputStream) {
        delete outputStream;
        inputStream = nullptr;
    }
    if (socket) {
        delete socket;
        socket = nullptr;
    }
    this->isConnect_t = false;
    this->isClosed_t = true;
}

CTL::ClientSocket::ClientSocket(const InetAddress &address, const int Port) {
    socket = new Socket();
    connect(address, Port);
}

CTL::ClientSocket::ClientSocket(Socket *sock) {
    this->socket = sock;
    this->inputStream = new SocketInputStream(this->socket);
    this->outputStream = new SocketOutputStream(this->socket);
    this->isConnect_t = true;
    this->isClosed_t = false;
    const auto addr = socket->GetClientHost();
    this->hostAddress = addr.IPAddress;
    this->Port = addr.Port;
}

bool CTL::ClientSocket::connect(const InetAddress &address, const int Port) {
    if (socket) {
        this->IP_x = address.getIPx();
        this->Port = Port;
        this->hostAddress = address.getHostAddress();
        socket->Init(IP_x, TORU::TCP, TYPE::STREAM);
        this->isConnect_t = socket->Connect(hostAddress.c_str(), Port);
        if (this->isConnect_t) {
            this->isClosed_t = false;
            this->inputStream = new SocketInputStream(this->socket);
            this->outputStream = new SocketOutputStream(this->socket);
        }
        return this->isConnect_t;
    }
    return false;
}

void CTL::ClientSocket::close() {
    if (socket) {
        this->isClosed_t = true;
        this->isConnect_t = false;
        socket->Close();
    }
}

bool CTL::ClientSocket::isConnect() const {
    return this->isConnect_t;
}

bool CTL::ClientSocket::isClosed() const {
    return this->isClosed_t;
}

CTL::SocketInputStream * CTL::ClientSocket::getInputStream() const {
      return this->inputStream;
}

CTL::SocketOutputStream * CTL::ClientSocket::getOutputStream() const {
    return this->outputStream;
}

CTL::Socket * CTL::ClientSocket::getSocket() const {
    return this->socket;
}

int CTL::ClientSocket::getPort() const {
    return this->Port;
}

CTL::InetAddress CTL::ClientSocket::getHostAddress() const {
    return InetAddress(hostAddress);
}
