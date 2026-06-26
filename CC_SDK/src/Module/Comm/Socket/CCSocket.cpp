#include "Socket/CCSocket.h"
#include <cstring>
#ifdef _WIN32
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

using namespace CTL;

CTL::Socket::Socket(const SOCKET sock) {
    Socketbit = sock;
}

bool CTL::Socket::Init(IPVX IPV4orIPV6, TORU TCPorUDP, TYPE Type){
#ifdef _WIN32
    CTL::Socket sc{};
    this->Socketbit = socket(IPV4orIPV6, TCPorUDP == TCP ? SOCK_STREAM : SOCK_DGRAM, TCPorUDP);
    if (this->Socketbit == INVALID_SOCKET){
        return false;
    }
    this->sock = TCPorUDP;
    this->IPVx = IPV4orIPV6;
    sc.sock = this->sock;
    sc.IPVx = this->IPVx;
    sc.Socketbit = this->Socketbit;
    return true;
#elif __linux__
    CTL::Socket sc;
    Socketbit = socket(IPV4orIPV6, Type, TCPorUDP);
    if(Socketbit == 0)
    {
        Socketbit = -1;
        return false;
    }
    int err = setsockopt(Socketbit, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(!err){
#ifdef SO_REUSEPORT
        err = setsockopt(Socketbit, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif
    }
    if(err)
    {
        Socketbit = -2;
        return false;
    }
    this->IPVx = IPV4orIPV6;
    sc.IPVx = this->IPVx;
    sc.Socketbit = Socketbit;
    return true;
#endif
}

SOCKET CTL::Socket::GetSOCKET() const {
    return Socketbit;
}

bool CTL::Socket::SetSockOpt(CCOpt opt) const {
#ifdef _WIN32
    switch(opt) {
        case CCOpt::BROADCAST:
        {
            int broadcastPermission = 1;
            if (setsockopt(this->Socketbit, SOL_SOCKET, SO_BROADCAST,
                          (const char*)&broadcastPermission, sizeof(broadcastPermission)) < 0) {
                return false;
            }
            break;
        }
        case CCOpt::REUSEADDR:
        {
            int reuseAddr = 1;
            if (setsockopt(this->Socketbit, SOL_SOCKET, SO_REUSEADDR,
                          (const char*)&reuseAddr, sizeof(reuseAddr)) < 0) {
                return false;
            }
            break;
        }
#ifdef SO_REUSEPORT
        case CCOpt::REUSEPORT:
        {
            int reusePort = 1;
            if (setsockopt(this->Socketbit, SOL_SOCKET, SO_REUSEPORT,
                          (const char*)&reusePort, sizeof(reusePort)) < 0) {
                return false;
            }
            break;
        }
#endif
        case CCOpt::NOBLOCK:
        {
            u_long mode = 1;
            if (ioctlsocket(this->Socketbit, FIONBIO, &mode) == SOCKET_ERROR) {
                return false;
            }
            break;
        }
        case CCOpt::LINGER_t:
        {
            linger l = {1, 0}; // l_onoff=1, l_linger=0
            if (setsockopt(this->Socketbit, SOL_SOCKET, SO_LINGER,
                          (const char*)&l, sizeof(l)) < 0) {
                return false;
            }
            break;
        }
        case CCOpt::SNDBUF:
        {
            int bufferSize = 65536; // 默认发送缓冲区大小
            if (setsockopt(this->Socketbit, SOL_SOCKET, SO_SNDBUF,
                          (const char*)&bufferSize, sizeof(bufferSize)) < 0) {
                return false;
            }
            break;
        }
        case CCOpt::RCVBUF:
        {
            int bufferSize = 65536; // 默认接收缓冲区大小
            if (setsockopt(this->Socketbit, SOL_SOCKET, SO_RCVBUF,
                          (const char*)&bufferSize, sizeof(bufferSize)) < 0) {
                return false;
            }
            break;
        }
        case CCOpt::TIMESTAMP:
        {
#ifdef SO_TIMESTAMP
            int timestamp = 1;
            if (setsockopt(this->Socketbit, SOL_SOCKET, SO_TIMESTAMP,
                          (const char*)&timestamp, sizeof(timestamp)) < 0) {
                return false;
            }
#endif
            break;
        }
        default:
            return false;
    }
    return true;
#elif __linux__
    switch(opt) {
        case CCOpt::BROADCAST:
        {
            int broadcastPermission = 1;
            if (setsockopt(this->Socketbit, SOL_SOCKET, SO_BROADCAST,
                          (const char*)&broadcastPermission, sizeof(broadcastPermission)) < 0) {
                return false;
            }
            break;
        }
        case CCOpt::REUSEADDR:
        {
            int reuseAddr = 1;
            if (setsockopt(this->Socketbit, SOL_SOCKET, SO_REUSEADDR,
                          (const char*)&reuseAddr, sizeof(reuseAddr)) < 0) {
                return false;
            }
            break;
        }
#ifdef SO_REUSEPORT
        case CCOpt::REUSEPORT:
        {
            int reusePort = 1;
            if (setsockopt(this->Socketbit, SOL_SOCKET, SO_REUSEPORT,
                          (const char*)&reusePort, sizeof(reusePort)) < 0) {
                return false;
                          }
            break;
        }
#endif
        case CCOpt::NOBLOCK:
        {
            int flags = fcntl(this->Socketbit, F_GETFL, 0);
            if (flags == -1) return false;
            flags |= O_NONBLOCK;
            if (fcntl(this->Socketbit, F_SETFL, flags) == -1) {
                return false;
            }
            break;
        }
        case CCOpt::LINGER_t:
        {
            struct linger l = {1, 0}; // l_onoff=1, l_linger=0
            if (setsockopt(this->Socketbit, SOL_SOCKET, SO_LINGER,
                          (const char*)&l, sizeof(l)) < 0) {
                return false;
            }
            break;
        }
        case CCOpt::SNDBUF:
        {
            int bufferSize = 65536; // 默认发送缓冲区大小
            if (setsockopt(this->Socketbit, SOL_SOCKET, SO_SNDBUF,
                          (const char*)&bufferSize, sizeof(bufferSize)) < 0) {
                return false;
            }
            break;
        }
        case CCOpt::RCVBUF:
        {
            int bufferSize = 65536; // 默认接收缓冲区大小
            if (setsockopt(this->Socketbit, SOL_SOCKET, SO_RCVBUF,
                          (const char*)&bufferSize, sizeof(bufferSize)) < 0) {
                return false;
            }
            break;
        }
        case CCOpt::TIMESTAMP:
        {
#ifdef SO_TIMESTAMP
            int timestamp = 1;
            if (setsockopt(this->Socketbit, SOL_SOCKET, SO_TIMESTAMP,
                          (const char*)&timestamp, sizeof(timestamp)) < 0) {
                return false;
            }
#endif
            break;
        }
        default:
            return false;
    }
    return true;
#endif
}

std::vector<CTL::String> CTL::Socket::GetLocalIP(IPVX ipvx,int Number)
{
#ifdef _WIN32
    std::vector<CTL::String> IPList;
    int result = 0;
    // 获取主机名
    char hostName[NI_MAXHOST];
    if (gethostname(hostName, NI_MAXHOST) != 0) {
        std::cerr << "gethostname failed" << std::endl;
        WSACleanup();
        return {};
    }
    struct addrinfo hints, *info;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // AF_INET for IPv4 only
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    // 获取地址信息
    result = getaddrinfo(hostName, nullptr, &hints, &info);
    if (result != 0) {
        std::cerr << "getaddrinfo failed: " << result << std::endl;
        return {};
    }
    char ipString[INET6_ADDRSTRLEN] = {0};
    // 遍历所有地址信息并打印IPv4地址
    for (struct addrinfo* p = info; p != nullptr; p = p->ai_next) {
        void* addr;

        // 对于IPv4，使用ipv4_mapping来兼容IPv6地址
        if (p->ai_family == ipvx) {
            if (p->ai_family == IPV4) {
                const auto ipv4 = reinterpret_cast<struct sockaddr_in *>(p->ai_addr);
                addr = &(ipv4->sin_addr);
            }
            else if (p->ai_family == IPV6) {
                auto* ipv6 = reinterpret_cast<struct sockaddr_in6 *>(p->ai_addr);
                addr = &(ipv6->sin6_addr);
            }
            else {
                continue;
            }
        }
        else {
            continue;
        }
        // 将地址转换为字符串
        inet_ntop(p->ai_family, addr, ipString, sizeof(ipString));
        IPList.emplace_back(ipString);
        // std::cout << "IPv4 address: " << ipString << std::endl;
    }
    // 释放地址信息
    freeaddrinfo(info);
    return IPList;
#elif __linux__
    std::vector<CTL::String> IPList;
    struct ifaddrs *ifAddrStruct = nullptr;
    struct ifaddrs *ifa = nullptr;
    void *tmpAddrPtr = nullptr;

    if (getifaddrs(&ifAddrStruct) != 0) {
        return IPList;
    }

    char addressBuffer[50][INET6_ADDRSTRLEN];
    int d = 0;

    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        // 检查ifa_addr是否为空
        if (ifa->ifa_addr == nullptr) {
            continue;
        }

        // 检查是否为IPv4或IPv6
        if (ifa->ifa_addr->sa_family == AF_INET) {
            // IPv4地址处理
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;

            // 防止缓冲区溢出
            if (d >= 50) {
                break;
            }

            // 使用正确的长度参数
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer[d], INET_ADDRSTRLEN);
            IPList.emplace_back(addressBuffer[d]);
            d++;
        }
        else if (ifa->ifa_addr->sa_family == AF_INET6) {
            // IPv6地址处理
            tmpAddrPtr = &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;

            // 防止缓冲区溢出
            if (d >= 50) {
                break;
            }

            // 使用INET6_ADDRSTRLEN而不是INET_ADDRSTRLEN
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer[d], INET6_ADDRSTRLEN);
            IPList.emplace_back(addressBuffer[d]);
            d++;
        }
    }

    if (ifAddrStruct != nullptr) {
        freeifaddrs(ifAddrStruct);
    }
    return IPList;
#endif

}

bool CTL::Socket::Connect(const char* IP, unsigned short Port) {
#ifdef _WIN32
    Stop = true;
    if (this->IPVx == IPV4) {
        server.sin_family = AF_INET;
        server.sin_port = htons(Port);
        server.sin_addr.s_addr = inet_addr(IP);
        if (server.sin_addr.s_addr == INADDR_NONE) {
            // 如果不是有效IP地址，尝试DNS解析
            struct addrinfo hints = {}, *result = nullptr;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            if (getaddrinfo(IP, nullptr, &hints, &result) != 0) {
                std::cerr << "DNS resolution failed for: " << IP << std::endl;
                return false;
            }

            const auto* addr_in = reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
            server.sin_addr = addr_in->sin_addr;
            freeaddrinfo(result);
        }
        int len = sizeof(server);
        int i = connect(this->Socketbit,reinterpret_cast<sockaddr *>(&server),len);
        if (i == SOCKET_ERROR){
            return false;
        }
    }
    else if (this->IPVx == IPV6) {
        server_6.sin6_family = AF_INET6;
        server_6.sin6_port = htons(Port);
        if (inet_pton(AF_INET6, IP, &server_6.sin6_addr) != 1) {
            // 如果不是有效 IPv6 地址，尝试 DNS 解析
            struct addrinfo hints = {}, *result = nullptr;
            hints.ai_family = AF_INET6;
            hints.ai_socktype = SOCK_STREAM;

            if (getaddrinfo(IP, nullptr, &hints, &result) != 0) {
                std::cerr << "DNS resolution failed for: " << IP << std::endl;
                return false;
            }

            // 确保获取到的是 IPv6 地址
            if (result->ai_family != AF_INET6) {
                std::cerr << "DNS resolution did not return IPv6 address for: " << IP << std::endl;
                freeaddrinfo(result);
                return false;
            }

            const auto* addr_in6 = reinterpret_cast<struct sockaddr_in6*>(result->ai_addr);
            server_6.sin6_addr = addr_in6->sin6_addr;
            freeaddrinfo(result);
        }
        int len = sizeof(sockaddr_in6);
        int i = connect(this->Socketbit,reinterpret_cast<sockaddr *>(&server_6),len);
        if (i == SOCKET_ERROR){
            return false;
        }
    }
    return true;
#elif __linux__
    Stop = true;
    if (this->IPVx == IPV4) {
        server.sin_family = AF_INET;
        server.sin_port = htons(Port);
        server.sin_addr.s_addr = inet_addr(IP);
        if (server.sin_addr.s_addr == INADDR_NONE) {
            // 如果不是有效IP地址，尝试DNS解析
            struct addrinfo hints = {}, *result = nullptr;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            if (getaddrinfo(IP, nullptr, &hints, &result) != 0) {
                std::cerr << "DNS resolution failed for: " << IP << std::endl;
                return false;
            }

            const auto* addr_in = reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
            server.sin_addr = addr_in->sin_addr;
            freeaddrinfo(result);
        }
        int len = sizeof(server);
        int i = connect(this->Socketbit,(sockaddr*)&server,len);
        if (i == SOCKET_ERROR){
            return false;
        }
    }
    else if (this->IPVx == IPV6) {
        server_6.sin6_family = AF_INET6;
        server_6.sin6_port = htons(Port);
        if (inet_pton(AF_INET6, IP, &server_6.sin6_addr) != 1) {
            // 如果不是有效 IPv6 地址，尝试 DNS 解析
            struct addrinfo hints = {}, *result = nullptr;
            hints.ai_family = AF_INET6;
            hints.ai_socktype = SOCK_STREAM;

            if (getaddrinfo(IP, nullptr, &hints, &result) != 0) {
                std::cerr << "DNS resolution failed for: " << IP << std::endl;
                return false;
            }

            // 确保获取到的是 IPv6 地址
            if (result->ai_family != AF_INET6) {
                std::cerr << "DNS resolution did not return IPv6 address for: " << IP << std::endl;
                freeaddrinfo(result);
                return false;
            }

            const auto* addr_in6 = reinterpret_cast<struct sockaddr_in6*>(result->ai_addr);
            server_6.sin6_addr = addr_in6->sin6_addr;
            freeaddrinfo(result);
        }
        int len = sizeof(sockaddr_in6);
        int i = connect(this->Socketbit,reinterpret_cast<sockaddr *>(&server_6),len);
        if (i == SOCKET_ERROR){
            return false;
        }
    }
    return true;
#endif
}

bool CTL::Socket::Bind(const char* IP, unsigned short Port){
#ifdef _WIN32
    if (IPVx == IPV4) {
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_port = htons(Port);
        server.sin_addr.s_addr = inet_addr(IP);
        int i = bind(this->Socketbit, reinterpret_cast<sockaddr *>(&server), sizeof(server));
        if (i == SOCKET_ERROR){
            int error = WSAGetLastError();
            std::cerr << "bind " << Port << " failed with error: " << error << std::endl;
            this->Socketbit = -1;
            closesocket(this->Socketbit);
            return false;
        }
    }
    else if (IPVx == IPV6) {
        constexpr int len = sizeof(sockaddr_in6); // "fe80:0:0:0:f73c:715:87e5:14b1"
        memset(&server_6, 0, len);
        server_6.sin6_family = AF_INET6;
        server_6.sin6_port = htons(Port);
        if (inet_pton(AF_INET6, IP, &server_6.sin6_addr) != 1) {
            std::cerr << "Invalid IPv6 address: " << IP << std::endl;
            return false;
        }
        const auto s = reinterpret_cast<sockaddr *>(&server_6);
        const int i = bind(this->Socketbit,s , len);
        if (i == SOCKET_ERROR){
            const int error = WSAGetLastError();
            std::cerr << "bind " << Port << " failed with error: " << error << std::endl;
            this->Socketbit = -1;
            closesocket(this->Socketbit);
            return false;
        }
    }
    return true;
#elif __linux__
    if (IPVx == IPV4) {
        // 设置地址结构
        memset(&server, 0, sizeof(server));
        server.sin_family = IPVx;
        server.sin_port = htons(Port);
        server.sin_addr.s_addr = inet_addr(IP);
        // 绑定套接字到地址和端口
        if (bind(this->Socketbit, (struct sockaddr *)&server, sizeof(server)) < 0) {
            std::cerr << "bind " << Port << " failed with error "<< std::endl;
            return false;
        }
    }
    else if (IPVx == IPV6) {
        memset(&server_6, 0, sizeof(sockaddr_in6));
        server_6.sin6_family = AF_INET6;
        server_6.sin6_port = htons(Port);
        inet_pton(AF_INET6, IP, &server_6.sin6_addr);
        constexpr int len = sizeof(sockaddr_in6);
        const auto s = reinterpret_cast<sockaddr *>(&server_6);
        const int i = bind(this->Socketbit,s , len);
        if (i < 0){
            std::cerr << "bind " << Port << " failed with error "<< std::endl;
            return false;
        }
    }
    return true;
#endif
}

bool CTL::Socket::Listen(unsigned short UserNum)
{
#ifdef _WIN32
    Stop = true;
    int iLisRet = listen(this->Socketbit, UserNum);
    if (iLisRet == SOCKET_ERROR){
        closesocket(this->Socketbit);
        return false;
    }
    return true;
#elif __linux__
    Stop = true;
    int err = listen(Socketbit, UserNum);
    if(err < 0)
    {
        return false;
    }
    return true;
#endif
}

CTL::Socket CTL::Socket::Accept() const {
#ifdef _WIN32
    CTL::Socket sc;
    if (this->IPVx == IPV4) {
        SOCKADDR_IN serveraddrfrom{};
        int Len = sizeof(serveraddrfrom);
        SOCKET s = accept(this->Socketbit, reinterpret_cast<SOCKADDR *>(&serveraddrfrom), &Len);
        sc.Socketbit = s;
        sc.sock = this->sock;
        sc.client = serveraddrfrom;
    }
    else if (this->IPVx == IPV6) {
        sockaddr_in6 addrClient = {};// 保存客户端IP地址端口
        int len = sizeof(sockaddr_in6);//地址结构大小改变 sizeof(sockaddr_in6)
        SOCKET s = accept(this->Socketbit, reinterpret_cast<SOCKADDR *>(&addrClient), &len);
        sc.Socketbit = s;
        sc.sock = this->sock;
        sc.client_6 = addrClient;
    }
    sc.IPVx = this->IPVx;
    return sc;
#elif __linux__
    CTL::Socket sc;
    if (this->IPVx == IPV4) {
        sockaddr_in serveraddrfrom{};
        socklen_t Len = sizeof(serveraddrfrom);
        SOCKET s = accept(this->Socketbit, reinterpret_cast<sockaddr *>(&serveraddrfrom), &Len);
        sc.Socketbit = s;
        sc.sock = this->sock;
        sc.client = serveraddrfrom;
    }
    else if (this->IPVx == IPV6) {
        sockaddr_in6 addrClient = {};// 保存客户端IP地址端口
        socklen_t len = sizeof(sockaddr_in6);//地址结构大小改变 sizeof(sockaddr_in6)
        SOCKET s = accept(this->Socketbit, reinterpret_cast<sockaddr *>(&addrClient), &len);
        sc.Socketbit = s;
        sc.sock = this->sock;
        sc.client_6 = addrClient;
    }
    sc.IPVx = this->IPVx;
    return sc;
#endif
}

bool CTL::Socket::Send(const char* str) const {
    return SendByte(str, strlen(str));
}

bool CTL::Socket::SendByte(const char* str, int len) const {
#ifdef _WIN32
    int ires = send(this->Socketbit, str, len, MSL_N);
    if (ires == -1){
        return false;
    }
    return true;
#elif __linux__
    const auto bytesSent = send(Socketbit, str, len, MSL_N);
    if(bytesSent == -1){
        return false;
    }
    return true;
#endif
}

bool Socket::SendByte(const char *str, int len, int *ret) const {
#ifdef _WIN32
    const auto bytesSent = send(Socketbit, str, len, MSL_N);
#elif __linux__
    const auto bytesSent = send(Socketbit, str, len, MSL_N);
#endif
    if (bytesSent == SOCKET_ERROR) {
        // 处理发送错误
        const int err = GetLastError();
        if (err == 32) {
            *ret = -32;
        }
        return false;
        // 根据错误码决定是否重试
    }
    else if (bytesSent < len) {
        *ret = bytesSent;
        return false;
    }
    *ret = bytesSent;
    return true;
}

bool CTL::Socket::UDPSend(const char* str, const char* IP, int Port) const {
   return UDPSendByte(str, strlen(str), IP, Port);
}

bool CTL::Socket::UDPSendByte(const char* str,ByteHander len ,const char* IP, int Port) const {
#ifdef _WIN32
    if (this->IPVx == IPV4) {
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(Port);
        serverAddr.sin_addr.s_addr = inet_addr(IP);
        if (sendto(this->Socketbit, str, len, 0, reinterpret_cast<SOCKADDR *>(&serverAddr),
            sizeof(serverAddr)) == SOCKET_ERROR){
            return false;
            }
    }
    else if (this->IPVx == IPV6) {
        sockaddr_in6 serverAddr{};
        serverAddr.sin6_family = AF_INET6;
        serverAddr.sin6_port = htons(Port);
        if (inet_pton(AF_INET6, IP, &serverAddr.sin6_addr) != 1) {
            std::cerr << "Invalid IPv6 address: " << IP << std::endl;
            return false;
        }
        if (sendto(this->Socketbit, str, len, 0, reinterpret_cast<sockaddr *>(&serverAddr),
            sizeof(serverAddr)) == SOCKET_ERROR){
            return false;
            }
    }
    return true;
#elif __linux__
    if (this->IPVx == IPV4) {
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(Port);
        serverAddr.sin_addr.s_addr = inet_addr(IP);
        if (sendto(this->Socketbit, str, len, 0, reinterpret_cast<sockaddr *>(&serverAddr),
            sizeof(serverAddr)) == SOCKET_ERROR){
            return false;
            }
    }
    else if (this->IPVx == IPV6) {
        sockaddr_in6 serverAddr;
        serverAddr.sin6_family = AF_INET6;
        serverAddr.sin6_port = htons(Port);
        if (inet_pton(AF_INET6, IP, &serverAddr.sin6_addr) != 1) {
            std::cerr << "Invalid IPv6 address: " << IP << std::endl;
            return false;
        }
        if (sendto(this->Socketbit, str, len, 0, reinterpret_cast<sockaddr *>(&serverAddr),
            sizeof(serverAddr)) == SOCKET_ERROR){
            return false;
            }
    }
    return true;
#endif
}

bool Socket::UDPSendByte(void *str, ByteHander len, const char *IP, int Port) const {
#ifdef _WIN32
    if (this->IPVx == IPV4) {
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(Port);
        serverAddr.sin_addr.s_addr = inet_addr(IP);
        if (sendto(this->Socketbit, static_cast<char *>(str), len, 0, reinterpret_cast<SOCKADDR *>(&serverAddr),
            sizeof(serverAddr)) == SOCKET_ERROR){
            return false;
            }
    }
    else if (this->IPVx == IPV6) {
        sockaddr_in6 serverAddr{};
        serverAddr.sin6_family = AF_INET6;
        serverAddr.sin6_port = htons(Port);
        if (inet_pton(AF_INET6, IP, &serverAddr.sin6_addr) != 1) {
            std::cerr << "Invalid IPv6 address: " << IP << std::endl;
            return false;
        }
        if (sendto(this->Socketbit, static_cast<char *>(str), len, 0, reinterpret_cast<sockaddr *>(&serverAddr),
            sizeof(serverAddr)) == SOCKET_ERROR){
            return false;
            }
    }
    return true;
#elif __linux__
    if (this->IPVx == IPV4) {
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(Port);
        serverAddr.sin_addr.s_addr = inet_addr(IP);
        if (sendto(this->Socketbit, str, len, 0, reinterpret_cast<sockaddr *>(&serverAddr),
            sizeof(serverAddr)) == SOCKET_ERROR){
            return false;
            }
    }
    else if (this->IPVx == IPV6) {
        sockaddr_in6 serverAddr;
        serverAddr.sin6_family = AF_INET6;
        serverAddr.sin6_port = htons(Port);
        if (inet_pton(AF_INET6, IP, &serverAddr.sin6_addr) != 1) {
            std::cerr << "Invalid IPv6 address: " << IP << std::endl;
            return false;
        }
        if (sendto(this->Socketbit, str, len, 0, reinterpret_cast<sockaddr *>(&serverAddr),
            sizeof(serverAddr)) == SOCKET_ERROR){
            return false;
            }
    }
    return true;
#endif
}

ByteHander CTL::Socket::RecvData(char* buffer, const ByteHander lens) const {
    const auto len = recv(Socketbit,buffer,lens,0);
    return len;
}

ByteHander CTL::Socket::UDPRecvData(char* buffer, ByteHander lens,CCHostInfo* info) const {
#ifdef _WIN32
    if (this->IPVx == IPV4) {
        sockaddr_in A = {};
        int clientSize = sizeof(A);
        int bytesReceived = recvfrom(this->Socketbit, buffer, lens, 0,reinterpret_cast<SOCKADDR *>(&A), &clientSize);
        if (info) {
            char IP[INET_ADDRSTRLEN] = {0};
            inet_ntop(IPVx, &A.sin_addr, IP, INET_ADDRSTRLEN);
            info->IPAddress = IP;
            info->Port = ntohs(A.sin_port);
        }
        return bytesReceived;
    }
    else if (this->IPVx == IPV6) {
        sockaddr_in6 A = {};
        int clientSize = sizeof(A);
        int bytesReceived = recvfrom(this->Socketbit, buffer, lens, 0,reinterpret_cast<SOCKADDR *>(&A), &clientSize);
        if (info) {
            char IP[INET6_ADDRSTRLEN] = {0};
            inet_ntop(IPVx, &A.sin6_addr, IP, INET6_ADDRSTRLEN);
            info->IPAddress = IP;
            info->Port = ntohs(A.sin6_port);
        }
        return bytesReceived;
    }
    return 0;
#elif __linux__
    if (this->IPVx == IPV4) {
        sockaddr_in A = {};
        socklen_t clientSize = sizeof(A);
        int bytesReceived = recvfrom(this->Socketbit, buffer, lens, 0,reinterpret_cast<sockaddr *>(&A), &clientSize);
        if (info) {
            char IP[INET_ADDRSTRLEN] = {0};
            inet_ntop(IPVx, &A.sin_addr, IP, INET_ADDRSTRLEN);
            info->IPAddress = IP;
            info->Port = ntohs(A.sin_port);
        }
        return bytesReceived;
    }
    else if (this->IPVx == IPV6) {
        sockaddr_in6 A = {};
        socklen_t clientSize = sizeof(A);
        int bytesReceived = recvfrom(this->Socketbit, buffer, lens, 0,reinterpret_cast<sockaddr *>(&A), &clientSize);
        if (info) {
            char IP[INET6_ADDRSTRLEN] = {0};
            inet_ntop(IPVx, &A.sin6_addr, IP, INET6_ADDRSTRLEN);
            info->IPAddress = IP;
            info->Port = ntohs(A.sin6_port);
        }
        return bytesReceived;
    }
    return 0;
#endif
}

void CTL::Socket::Close()
{
#ifdef _WIN32
    Stop = false;
    closesocket(this->Socketbit);
    this->Socketbit = -1;
#elif __linux__
    Stop = false;
    close(Socketbit);
    this->Socketbit = -1;
#endif
}

bool CTL::Socket::GetDateHead(char* data, ByteHander* size)
{
#ifdef _WIN32
    int i = sizeof(data);
    memcpy(size,data,i);
    if (size!=0)
    {
        return true;
    }
    return false;
#elif __linux__
    int i = sizeof(data);
    memcpy(size,data,i);
    if (size!=0)
    {
        return true;
    }
    return false;
#endif
}

bool CTL::Socket::GetStrHead(char* data, ByteHander size)
{
#ifdef _WIN32
    for (int i = 0; i < HanderSize; i++)
    {
        data[i] = '\0';
    }
    data[0] = '-';
    memcpy(data, &size, sizeof(ByteHander));
    if (data[0] == '-')
    {
        return false;
    }
    return true;
#elif __linux__
    for (int i = 0; i < HanderSize; i++)
    {
        data[i] = '\0';
    }
    data[0] = '-';
    memcpy(data, &size, sizeof(ByteHander));
    if (data[0] == '-')
    {
        return false;
    }
    return true;
#endif
}

bool CTL::Socket::Send(const char *__buf, size_t __n, int __flags) const {
#ifdef _WIN32
    int err = send(Socketbit, (char*)__buf, __n, __flags);
    if(err == -1)
    {
        int errorCode = WSAGetLastError(); // Windows
        // std::cerr << "Send failed with error code: " << errorCode << std::endl;
        // 处理错误码
        return false;
    }
    return true;
#elif __linux__
    int err = send(Socketbit, __buf, __n, __flags);
    if(err == -1)
    {
        return false;
    }
    return true;
#endif
}

bool CTL::Socket::SendData(const char* str,const int length, sockaddr_in addr_in) const {
#ifdef _WIN32
    bool F = sendto(this->Socketbit, str, length, 0, (sockaddr*)&addr_in, sizeof(addr_in)) != SOCKET_ERROR;
    return F;
#elif __linux__
    bool F = sendto(this->Socketbit, str, length, 0, (sockaddr*)&addr_in, sizeof(addr_in)) != -1;
    return F;
#endif
}

bool CTL::Socket::operator==(CTL::Socket socket) const {
#ifdef _WIN32
    if(socket.Socketbit == this->Socketbit)
    {
        return true;
    }
    return false;
#elif __linux__
    if(socket.Socketbit == this->Socketbit)
    {
        return true;
    }
    return false;
#endif
}

bool CTL::Socket::operator!=(CTL::Socket socket) const {
#ifdef _WIN32
    if(socket.Socketbit != this->Socketbit)
    {
        return true;
    }
    return false;
#elif __linux__
    if(socket.Socketbit != this->Socketbit)
    {
        return true;
    }
    return false;
#endif
}

CCHostInfo CTL::Socket::GetClientHost() const {
    CCHostInfo hostInfo{};
    if (this->IPVx == IPV4) {
        char IP[INET_ADDRSTRLEN] = {0};
        inet_ntop(IPVx, &client.sin_addr, IP, INET_ADDRSTRLEN);
        hostInfo.IPAddress = IP;
        hostInfo.Port = ntohs(client.sin_port);
    }
    else if (this->IPVx == IPV6) {
        char IP[INET6_ADDRSTRLEN] = {0};
        inet_ntop(IPVx, &client_6.sin6_addr, IP, INET6_ADDRSTRLEN);
        hostInfo.IPAddress = IP;
        hostInfo.Port = ntohs(client_6.sin6_port);
    }
    return hostInfo;
}

CCHostInfo CTL::Socket::GetLocalHost() const {
    CCHostInfo hostInfo;
    if (this->IPVx == IPV4) {
        char IP[INET_ADDRSTRLEN] = {0};
        inet_ntop(IPVx, &server.sin_addr, IP, INET_ADDRSTRLEN);
        hostInfo.IPAddress = IP;
        hostInfo.Port = ntohs(server.sin_port);
    }
    else if (this->IPVx == IPV6) {
        char IP[INET6_ADDRSTRLEN] = {0};
        inet_ntop(IPVx, &server_6.sin6_addr, IP, INET6_ADDRSTRLEN);
        hostInfo.IPAddress = IP;
        hostInfo.Port = ntohs(server_6.sin6_port);
    }
    return hostInfo;
}

void CTL::Socket::SetSocketNonBlocking() const {
#ifdef _WIN32
    if (this->Socketbit == INVALID_SOCKET || this->Socketbit < 0) {
        std::cerr << "SetSocketNonBlocking: Invalid socket descriptor" << std::endl;
        return;
    }
    u_long mode = 1;
    if (ioctlsocket(this->Socketbit, FIONBIO, &mode) == SOCKET_ERROR)
    {
        std::cerr << "ioctlsocket failed with error: " << WSAGetLastError() << std::endl;
    }
#elif __linux__
    if (this->Socketbit < 0) {
        std::cerr << "SetSocketNonBlocking: Invalid socket descriptor" << std::endl;
        return;
    }
    int flags = fcntl(this->Socketbit, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl(F_GETFL)");
        return;
    }
    flags |= O_NONBLOCK;
    if (fcntl(this->Socketbit, F_SETFL, flags) == -1)
    {
        perror("fcntl(F_SETFL)");
    }
#endif
}

bool CTL::Socket::isDataAvailable() const {
#ifdef _WIN32
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(this->Socketbit, &readfds);
    timeval timeout = {0, 0}; // Non-blocking check
    int ret = select(0, &readfds, nullptr, nullptr, &timeout);
    if (ret == SOCKET_ERROR)
    {
        //std::cerr << "select failed with error: " << WSAGetLastError() << std::endl;
        return false;
    }
    return FD_ISSET(this->Socketbit, &readfds);
#elif __linux__
    pollfd pfd{};
    pfd.fd = this->Socketbit;
    pfd.events = POLLIN;
    pfd.revents = 0;
    int ret = poll(&pfd, 1, 0); // Non-blocking check
    if (ret == -1)
    {
        perror("poll");
        return false;
    }
    return pfd.revents & POLLIN;
#endif
}

bool CTL::Socket::isConnectionAlive() const {
#ifdef _WIN32
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(Socketbit, &fds);
    timeval timeout = { 0, 0 }; // 非阻塞模式立即返回
    int ret = select(Socketbit + 1, &fds, nullptr, &fds, &timeout);
    if (ret < 0) {
        // 错误处理
        return false;
    }
    else if (ret > 0) {
        if (FD_ISSET(Socketbit, &fds)) {
            if (FD_ISSET(Socketbit, &fds)) {
                // 检查异常条件
                return false;
            }
        }
    }
    return true;
#elif __linux__
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(Socketbit, &fds);
    timeval timeout = { 0, 0 }; // 非阻塞模式立即返回
    int ret = select(Socketbit + 1, &fds, NULL, &fds, &timeout);
    if (ret < 0) {
        // 错误处理
        return false;
    } else if (ret > 0) {
        if (FD_ISSET(Socketbit, &fds)) {
            if (FD_ISSET(Socketbit, &fds)) {
                // 检查异常条件
                return false;
            }
        }
    }
    return true;
#endif
}

bool CTL::Socket::IsSocketWritable(const SOCKET sock) {
#ifdef _WIN32
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(sock, &writefds);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int selectResult = select(0, nullptr, &writefds, nullptr, &timeout);
    if (selectResult == -1) {
        // std::cerr << "Select failed with error code: " << WSAGetLastError() << std::endl;
        return false;
    } else if (selectResult == 0) {
        // std::cerr << "Timeout occurred. Socket not writable." << std::endl;
        return false;
    }
    return true;
#else
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(sock, &writefds);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int selectResult = select(sock + 1, nullptr, &writefds, nullptr, &timeout);
    if (selectResult == -1) {
        // std::cerr << "Select failed with error code: " << errno << std::endl;
        return false;
    } else if (selectResult == 0) {
        // std::cerr << "Timeout occurred. Socket not writable." << std::endl;
        return false;
    }
    return true;
#endif
}

int Socket::GetLastError() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

InetAddress::InetAddress(const IPVX ip_x,const std::string& ip) {
    this->Ip_x = ip_x;
    if (ip.empty()) {
        const auto IPS = Socket::GetLocalIP(this->Ip_x);
        if (!IPS.empty()) {
            this->hostAddress = IPS[0];
        }
        else {
            this->hostAddress = "0.0.0.0";
        }
    }
    else {
        this->hostAddress = ip;
    }
}

InetAddress InetAddress::getLocalHost(IPVX ipvx) {
    const auto ips = Socket::GetLocalIP(ipvx, 1);
    if (!ips.empty()) {
        return InetAddress(ipvx,ips[0].c_str());
    }
    return InetAddress(ipvx,"127.0.0.1");
}

std::vector<InetAddress> InetAddress::getAllByName(const std::string &host) {
    std::vector<InetAddress> addresses;

    // 如果是IP地址格式，直接返回
    if (host.find('.') != std::string::npos) {
        addresses.emplace_back(host);
        return addresses;
    }

    // 这里应该实现DNS解析，简化处理
    // 实际实现中需要调用getaddrinfo等系统函数
    addresses.emplace_back("127.0.0.1", host);
    return addresses;
}

InetAddress InetAddress::getByName(const std::string &host) {
    auto all = getAllByName(host);
    return all.empty() ? InetAddress() : all[0];
}

std::string InetAddress::getHostAddress() const {
    return hostAddress;
}

std::string InetAddress::getHostName() const {
    if (!hostName.empty()) {
        return hostName;
    }
    return hostAddress; // 如果没有主机名，返回IP地址
}

InetAddress InetAddress::getAnyAddress() {
    return InetAddress("0.0.0.0");
}

InetAddress InetAddress::getLoopbackAddress() {
    return InetAddress("127.0.0.1");
}

InetAddress InetAddress::getBroadcastAddress(IPVX ipvx){
    sockaddr_in serverAddr{};
    serverAddr.sin_family = ipvx;
    serverAddr.sin_addr.s_addr = INADDR_BROADCAST;
    char IP[INET_ADDRSTRLEN] = {0};
    inet_ntop(ipvx, &serverAddr.sin_addr, IP, INET_ADDRSTRLEN);
    return InetAddress(ipvx,IP);
}

bool InetAddress::operator==(const InetAddress &other) const {
    return this->hostAddress == other.hostAddress;
}

bool InetAddress::operator!=(const InetAddress &other) const {
    return !(*this == other);
}

std::string InetAddress::toString() const {
    return hostName.empty() ? hostAddress : (hostName + "/" + hostAddress);
}

IPVX InetAddress::getIPx() const {
    return this->Ip_x;
}

std::vector<Ethernet> InetAddress::getEthernetList() {
    std::vector<Ethernet> ethernetList;
#ifdef _WIN32
    // PIP_ADAPTER_INFO 结构体指针存储本机网卡信息
    PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
    // 得到结构体大小，用于 GetAdaptersInfo 参数
    unsigned long stSize = sizeof(IP_ADAPTER_INFO);
    // 调用 GetAdaptersInfo 函数，填充 pIpAdapterInfo 指针变量;其中 stSize 参数既是一个输入量也是一个输出量
    int nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);

    if (ERROR_BUFFER_OVERFLOW == nRel) {
        // 如果函数返回的是 ERROR_BUFFER_OVERFLOW
        // 则说明 GetAdaptersInfo 参数传递的内存空间不够，同时其传出 stSize，表示需要的空间大小
        // 这也是说明为什么 stSize 既是一个输入量也是一个输出量
        // 释放原来的内存空间
        delete pIpAdapterInfo;
        // 重新申请内存空间用来存储所有网卡信息
        pIpAdapterInfo = reinterpret_cast<PIP_ADAPTER_INFO>(new BYTE[stSize]);
        // 再次调用 GetAdaptersInfo 函数，填充 pIpAdapterInfo 指针变量
        nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
    }

    if (ERROR_SUCCESS == nRel) {
        // 可能有多网卡，因此通过循环去判断
        while (pIpAdapterInfo) {
            Ethernet eth;

            // 获取网卡名称
            eth.Name = pIpAdapterInfo->Description;

            // 获取 MAC 地址
            char macStr[18] = {0};
            if (pIpAdapterInfo->AddressLength > 0 && pIpAdapterInfo->AddressLength <= 6) {
                sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                    pIpAdapterInfo->Address[0],
                    pIpAdapterInfo->Address[1],
                    pIpAdapterInfo->Address[2],
                    pIpAdapterInfo->Address[3],
                    pIpAdapterInfo->Address[4],
                    pIpAdapterInfo->Address[5]);
                eth.Mac = macStr;
            }

            // 获取 IP 地址（可能网卡有多 IP，因此通过循环去判断）
            IP_ADDR_STRING *pIpAddrString = &(pIpAdapterInfo->IpAddressList);
            do {
                eth.IP = pIpAddrString->IpAddress.String;
                if (!eth.Name.empty()) {
                    ethernetList.push_back(eth);
                }
                pIpAddrString = pIpAddrString->Next;
            } while (pIpAddrString);

            pIpAdapterInfo = pIpAdapterInfo->Next;
        }
    }

    // 释放内存空间
    if (pIpAdapterInfo) {
        delete pIpAdapterInfo;
    }
#elif __linux__
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == 0) {
        for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr || (ifa->ifa_flags & IFF_LOOPBACK)) {
                continue;
            }

            Ethernet eth;
            eth.Name = ifa->ifa_name;

            // 获取 IP 地址
            if (ifa->ifa_addr->sa_family == AF_INET) {
                sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &addr->sin_addr, ipStr, INET_ADDRSTRLEN);
                eth.IP = ipStr;

                // 获取 MAC 地址
                struct ifreq ifr;
                struct ifconf ifc;
                char buf[1024];
                int fd = socket(AF_INET, SOCK_DGRAM, 0);

                if (fd != -1) {
                    ifc.ifc_len = sizeof(buf);
                    ifc.ifc_buf = buf;
                    if (ioctl(fd, SIOCGIFCONF, &ifc) != -1) {
                        struct ifreq* it = ifc.ifc_req;
                        const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

                        for (; it != end; ++it) {
                            if (strcmp(it->ifr_name, ifa->ifa_name) == 0) {
                                if (ioctl(fd, SIOCGIFHWADDR, &ifr) != -1) {
                                    unsigned char* mac = reinterpret_cast<unsigned char*>(ifr.ifr_hwaddr.sa_data);
                                    char macStr[18];
                                    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                                        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                                    eth.Mac = macStr;
                                }
                                break;
                            }
                        }
                    }
                    close(fd);
                }

                ethernetList.push_back(eth);
            }
        }
        freeifaddrs(ifaddr);
    }
#endif
    return ethernetList;
}








