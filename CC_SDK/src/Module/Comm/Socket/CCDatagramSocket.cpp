#include "Socket/CCDatagramSocket.h"

CTL::DatagramPacket::DatagramPacket() {
    dataBuffer.resize(1440);
}

CTL::DatagramPacket::DatagramPacket(const size_t length) {
    dataBuffer.resize(length);
}

CTL::DatagramPacket::DatagramPacket(const char *data, const size_t length, const InetAddress &addr, const int portNum) {
    dataBuffer.resize(length);
    packetLength = length;
    address = addr;
    port = portNum;
    std::memcpy(dataBuffer.buffer(), data, length);
}

const char* CTL::DatagramPacket::getData() const {
    return dataBuffer.buffer();
}

long CTL::DatagramPacket::getLength() const {
    return packetLength;
}

void CTL::DatagramPacket::setLength(const size_t length) {
    packetLength = length;
}

size_t CTL::DatagramPacket::getCapacity() const {
    return dataBuffer.size();
}

CTL::InetAddress CTL::DatagramPacket::getAddress() const {
    return address;
}

void CTL::DatagramPacket::setAddress(const InetAddress &addr) {
    address = addr;
}

int CTL::DatagramPacket::getPort() const {
    return port;
}

void CTL::DatagramPacket::setPort(const int portNum) {
    port = portNum;
}

void CTL::DatagramPacket::setData(const char *data, const size_t length) {
    if (length > dataBuffer.size()) {
        dataBuffer.resize(length);
    }
    std::memcpy(dataBuffer.buffer(), data, length);
    packetLength = length;
}

void CTL::DatagramPacket::setData(const ByteArray &data) {
    if (data.size() > dataBuffer.size()) {
        dataBuffer.resize(data.size());
    }
    dataBuffer = data;
    packetLength = data.size();
}

void CTL::DatagramPacket::reset() {
    packetLength = 0;
    port = 0;
    address = InetAddress();
}

std::string CTL::DatagramPacket::getDataAsString() const {
    return dataBuffer.toString();
}

CTL::ByteArray CTL::DatagramPacket::getDataAsByteArray() const {
    return dataBuffer;
}

bool CTL::DatagramSocket::Init(const int Port, const InetAddress &address) {
    socket = new Socket();
    return bind(Port,address);
}

CTL::DatagramSocket::DatagramSocket() {
    socket = new Socket();
}

CTL::DatagramSocket::DatagramSocket(const int Port, const InetAddress &address) {
    Init(Port,address);
}

CTL::DatagramSocket::~DatagramSocket() {
    socket->Close();
    delete socket;
    socket = nullptr;
}

bool CTL::DatagramSocket::bind(const int Port, const InetAddress &address) {
    if (socket) {
        this->IP_x = address.getIPx();
        this->Port = Port;
        this->hostAddress = address.getHostAddress();
        socket->Init(IP_x, TORU::UDP, TYPE::DGRAM);
        this->isBound_t = socket->Bind(hostAddress.c_str(), Port);
        if (this->isBound_t) {
            this->isClosed_t = false;
        }
        return this->isBound_t;
    }
    return false;
}

bool CTL::DatagramSocket::send(const DatagramPacket &packet) const {
    const auto buffer = packet.getDataAsByteArray();
    const auto size = packet.getLength();
    const auto addr = packet.getAddress();
    const auto port = packet.getPort();
    return socket->UDPSendByte(buffer.buffer(),size,addr.getHostName().c_str(),port);
}

void CTL::DatagramSocket::receive(DatagramPacket &packet) const {
    if (socket) {
        const auto size = packet.getCapacity();
        const auto buffer = new char[size];
        CCHostInfo info;
        const int len = socket->UDPRecvData(buffer,size,&info);
        if (len > 0) {
            const InetAddress address(info.IPAddress);
            packet.setAddress(address);
            packet.setPort(info.Port);
            packet.setData(ByteArray(buffer, len));
            packet.setLength(len);
        }
        delete[] buffer;
    }
}

void CTL::DatagramSocket::close() {
    if (socket) {
        this->isClosed_t = true;
        this->isBound_t = false;
        socket->Close();
    }
}

bool CTL::DatagramSocket::isBound() const {
    return this->isBound_t;
}

bool CTL::DatagramSocket::isClosed() const {
    return this->isClosed_t;
}

int CTL::DatagramSocket::getLocalPort() const {
    return this->Port;
}

CTL::InetAddress CTL::DatagramSocket::getLocalAddress() const {
    InetAddress address(this->IP_x, this->hostAddress);
    return address;
}

bool CTL::DatagramSocket::available() const {
    if (socket) {
        return socket->isDataAvailable();
    }
    return false;
}

CTL::Socket * CTL::DatagramSocket::getSocket() const {
    return socket;
}

void CTL::DatagramSocket::broadcast(const DatagramPacket& packet) const{
    if (socket){
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(packet.getPort());
        serverAddr.sin_addr.s_addr = INADDR_BROADCAST;
        socket->SendData(packet.getData(),packet.getLength(),serverAddr);
    }
}

bool CTL::DatagramSocket::setSockOpt(const CCOpt opt) const {
    return socket->SetSockOpt(opt);
}

std::vector<int> CTL::DatagramSocket::assignPortNumber(const String& Host, int start,int end,int Number, bool isRandom){
    std::vector<int> availablePorts;
    if (Number <= 0) {
        return availablePorts;
    }
    int startPort = start;
    int maxPort = end;
    if (isRandom) {
        srand(static_cast<unsigned int>(time(nullptr)));
        startPort = 10000 + rand() % (maxPort - 10000 - Number);
        if (startPort % 2 != 0) {
            startPort++;
        }
    }
    else {
        if (startPort % 2 != 0) {
            startPort++;
        }
    }
    int currentPort = startPort;
    int foundCount = 0;
    while (currentPort < maxPort && foundCount < Number) {
        Socket testSocket = {};
        if (testSocket.Init(IPV4, TORU::UDP, TYPE::DGRAM)) {
            if (testSocket.Bind(Host.c_str(), currentPort)) {
                availablePorts.push_back(currentPort);
                foundCount++;
                currentPort++;
                currentPort++;
                testSocket.Close();
            }
            else {
                testSocket.Close();
                if (isRandom) {
                    currentPort = 10000 + rand() % (maxPort - 10000 - Number);
                    if (currentPort % 2 != 0) {
                        currentPort++;
                    }
                }
                else {
                    currentPort++;
                    if (currentPort % 2 != 0) {
                        currentPort++;
                    }
                }
                foundCount = 0;
                availablePorts.clear();
            }
        }
        else {
            currentPort++;
            if (currentPort % 2 != 0) {
                currentPort++;
            }
        }
    }
    return availablePorts;
}

