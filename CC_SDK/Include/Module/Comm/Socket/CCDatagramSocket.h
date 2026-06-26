#ifndef DATAGRAM_SOCKET_H
#define DATAGRAM_SOCKET_H

#include <utility>

#include "CCSocket.h"
#include "CCByteArray.h"

namespace CTL {
    class DatagramPacket {
        ByteArray dataBuffer;
        long packetLength = 0;
        InetAddress address;
        int port = 0;
    public:
        // 构造函数：创建用于接收数据包的对象
        DatagramPacket();
        // 构造函数：创建用于接收数据包的对象
        explicit DatagramPacket(size_t length);
        // 构造函数：创建用于发送数据包的对象
        DatagramPacket(const char* data, size_t length, const InetAddress& addr, int portNum);
        // 构造函数：创建用于发送数据包的对象（使用vector）
        DatagramPacket(const ByteArray& data, InetAddress  addr, const int portNum)
            : dataBuffer(data), packetLength(data.size()), address(std::move(addr)), port(portNum) {}
        // 获取数据缓冲区（const版本）
        [[nodiscard]] const char* getData() const;
        // 获取数据缓冲区长度
        [[nodiscard]] long getLength() const;
        // 设置数据长度
        void setLength(size_t length);
        // 获取缓冲区容量
        [[nodiscard]] size_t getCapacity() const;
        // 获取目标地址
        [[nodiscard]] InetAddress getAddress() const;
        // 设置目标地址
        void setAddress(const InetAddress& addr);
        // 获取目标端口
        [[nodiscard]] int getPort() const;
        // 设置目标端口
        void setPort(int portNum);
        // 设置数据（从字符数组）
        void setData(const char* data, size_t length);
        // 设置数据（从vector）
        void setData(const ByteArray& data);
        // 重置数据包（清空内容）
        void reset();
        // 获取数据为字符串（如果数据是文本）
        [[nodiscard]] std::string getDataAsString() const;
        // 获取数据为字节数组
        [[nodiscard]] ByteArray getDataAsByteArray() const;
    };
    class DatagramSocket {
        Socket* socket = nullptr;
        IPVX IP_x = IPV4;
        std::string hostAddress;
        bool isClosed_t = true, isBound_t = false;
        int Port = 0;
        bool Init(int Port, const InetAddress &address);
    public:
        DatagramSocket();
        explicit DatagramSocket(int Port, const InetAddress &address = InetAddress::getAnyAddress());
        ~DatagramSocket();
        // 绑定到特定地址和端口
        bool bind(int Port, const InetAddress &address = InetAddress::getAnyAddress());
        // 发送数据包
        [[nodiscard]] bool send(const DatagramPacket& packet) const;
        // 接收数据包
        void receive(DatagramPacket& packet) const;
        // 关闭套接字
        void close();
        // 检查套接字是否已绑定
        [[nodiscard]] bool isBound() const;
        // 检查套接字是否已关闭
        [[nodiscard]] bool isClosed() const;
        // 获取本地端口
        [[nodiscard]] int getLocalPort() const;
        // 获取本地地址
        [[nodiscard]] InetAddress getLocalAddress() const;
        // 是否有数据可读
        [[nodiscard]] bool available() const;
        // 获取Socket
        [[nodiscard]] Socket* getSocket() const;
        // 广播数据包
        void broadcast(const DatagramPacket& packet) const;
        // 设置参数
        bool setSockOpt(CCOpt opt) const;
        // 获取端口
        static std::vector<int> assignPortNumber(const String& Host,int start,int end,int Number = 1,bool isRandom = false);
    };
}



#endif
