#include "CCNetwork.h"
using namespace CTL;

CTL::String Packets::to_String() const {
    return String(data.begin(), data.end());
}

CTL::String Packets::to_String() {
    return String(data.begin(), data.end());
}

Network::Network(const Socket& sock) {
    this->Socketbit = sock.Socketbit;
    client = sock.client;
    server = sock.server;
}

Network::Network(const Network &sock) {
    this->Socketbit = sock.Socketbit;
    client = sock.client;
    server = sock.server;
}

Network::Network(Socket &sock) {
    this->Socketbit = sock.Socketbit;
    client = sock.client;
    server = sock.server;
}

Network::Network(Network &sock) {
    this->Socketbit = sock.Socketbit;
    client = sock.client;
    server = sock.server;
}

Network::Network(int Port, TORU toru, IPVX ip) {
    TorU_ = toru;
    if(toru == TORU::TCP) {
        Init(ip, toru,STREAM);
        Bind(Any,Port);
        Listen(Port);
    }
    else if(toru == TORU::UDP) {
        Init(ip, toru, DGRAM);
        Bind(Any,Port);
    }
}

Network::Network(const TORU toru, IPVX ip) {
    Initialize(toru, ip);
}

void Network::Initialize(TORU toru, IPVX ip) {
    TorU_ = toru;
    if(toru == TORU::TCP) {
        Init(ip, toru,STREAM);
    }
    else if(toru == TORU::UDP) {
        Init(ip, toru, DGRAM);
    }
}

bool Network::Bind(const CCAddress address, unsigned short Port) {
    bool F = false;
    if(address == Any) {
        if (this->IPVx == IPV4) {
            F = Bind("0.0.0.0",Port);
        }
        else if (this->IPVx == IPV6) {
            F = Bind("::",Port);
        }
    }
    else if(address == Local) {
        if (this->IPVx == IPV4) {
            const CTL::String IP = GetLocalIP(IPV4)[0];
            F = Bind(IP,Port);
        }
        else if (this->IPVx == IPV6) {
            const CTL::String IP = GetLocalIP(IPV6)[0];
            F = Bind(IP,Port);
        }
    }
    return F;
}

bool Network::Bind(const CTL::String &IP, const unsigned short Port) {
    return Socket::Bind(IP.c_str(),Port);
}

bool Network::Connect(const CTL::String &IP, const unsigned short Port) {
    return Socket::Connect(IP.c_str(),Port);
}

bool Network::ReceiveData(Packets &packets) {
    ByteHander size = 0;
    CCVar* buffer = new char[packets.GetLength()];
    memset(buffer,0,packets.GetLength());
    if(TorU_ == TORU::TCP) {
        size = Socket::RecvData(buffer,packets.GetLength());
        packets.info = GetClientHost();
    }
    else if(TorU_ == TORU::UDP){
        size = Socket::UDPRecvData(buffer,packets.GetLength(),&packets.info);
    }
    if (size > 0) {
        packets.data.assign(buffer,size);
    }
    delete[] buffer;
    return size > 0;
}

bool Network::SendPackets(Packets &packets) {
    if(TorU_ == TORU::TCP) {
        return Socket::SendByte(packets.data.buffer(),packets.data.size());
    }
    else if(TorU_ == TORU::UDP) {
        return Socket::UDPSendByte(packets.data.buffer(),packets.data.size(),packets.info.IPAddress.c_str(),packets.info.Port);
    }
    return false;
}

