#ifndef CC_NETWORK_H
#define CC_NETWORK_H

#include "Socket/CCSocket.h"
#include "CC.h"

/**
 * @namespace CTL
 * @brief 该命名空间包含与网络通信相关的类和函数。
 */
namespace CTL {
    /**
     * @class Packets
     * @brief Packets 类用于封装数据包和主机信息，以便进行网络传输。
     */
    class Packets {
    public:
        ByteArray data{}; /**< @brief 存储数据包的字节数据。 */
        CCHostInfo info{}; /**< @brief 存储数据包的源主机信息。 */
    public:
        /**
         * @brief 默认构造函数，初始化数据包长度为 1024。
         */
        Packets() {
            length = 1024;
        }
        /**
         * @brief 带参数的构造函数。
         * @param len 指定的数据包长度。
         * @param host 主机信息，默认为 "0.0.0.0:0"。
         */
        explicit Packets(const ByteHander len, const CCHostInfo& host = {"0.0.0.0", 0}) {
            length = len;
            info = host;
        }
        /**
         * @brief 获取当前数据包的长度。
         * @return 返回数据的大小。
         */
        [[nodiscard]] ByteHander GetLength() const {
            return data.size();
        }
        /**
         * @brief 将数据包内容转换为字符串对象。
         * @return 返回数据包内容的字符串表示形式。
         */
        String to_String() const;
        /**
         * @brief 将数据包内容转换为字符串对象并清空原始数据包内容。
         * @return 返回数据包内容的字符串表示形式。
         */
        String to_String();
    private:
        ByteHander length = 0; /**< @brief 存储数据包的长度。 */
    };

    /**
     * @class Network
     * @brief Network 类用于管理网络连接和数据传输，继承自 Socket 类。
     */
    class Network : public Socket {
    public:
        using Socket::Socket; /**< @brief 继承 Socket 类的构造函数。 */
    public:
        /**
         * @brief 使用 Socket 对象构造。
         * @param sock 用于初始化 Network 对象的 Socket 对象。
         */
        Network(const Socket& sock);
        /**
         * @brief 复制构造函数。
         * @param sock 要复制的 Network 对象。
         */
        Network(const Network& sock);
        /**
         * @brief 使用 Socket 对象构造。
         * @param sock 用于初始化 Network 对象的 Socket 对象。
         */
        Network(Socket& sock);
        /**
         * @brief 复制构造函数。
         * @param sock 要复制的 Network 对象。
         */
        Network(Network& sock);
        /**
         * @brief 指定端口和网络类型构造。
         * @param Port 要绑定的端口号。
         * @param toru 网络类型，默认为 TCP。
         * @param ip IP 版本，默认为 IPV4。
         */
        explicit Network(int Port, TORU toru = TCP, IPVX ip = IPV4);
        /**
         * @brief 仅指定网络类型构造。
         * @param toru 网络类型，默认为 TCP。
         * @param ip IP 版本，默认为 IPV4。
         */
        explicit Network(TORU toru = TCP, IPVX ip = IPV4);
        /**
         * @brief 使用指定的类型和 IP 版本初始化网络。
         * @param toru 网络类型，默认为 TCP。
         * @param ip IP 版本，默认为 IPV4。
         */
        void Initialize(TORU toru = TCP, IPVX ip = IPV4);
        /**
         * @brief 绑定到指定地址和端口。
         * @param address 要绑定的网络地址。
         * @param Port 要绑定的端口号，默认为 0。
         * @return 成功返回 true，否则返回 false。
         */
        bool Bind(CCAddress address, unsigned short Port = 0);
        /**
         * @brief 绑定到指定 IP 和端口。
         * @param IP 要绑定的 IP 地址。
         * @param Port 要绑定的端口号。
         * @return 成功返回 true，否则返回 false。
         */
        bool Bind(const CTL::String& IP, unsigned short Port);
        /**
         * @brief 连接到指定 IP 和端口。
         * @param IP 要连接的 IP 地址。
         * @param Port 要连接的端口号。
         * @return 成功返回 true，否则返回 false。
         */
        bool Connect(const CTL::String& IP, unsigned short Port);
        /**
         * @brief 接收数据包。
         * @param packets 接收到的数据包。
         * @return 成功返回 true，否则返回 false。
         */
        bool ReceiveData(Packets& packets);
        /**
         * @brief 发送数据包。
         * @param packets 要发送的数据包。
         * @return 成功返回 true，否则返回 false。
         */
        bool SendPackets(Packets& packets);
    private:
        TORU TorU_ = TCP; /**< @brief 存储当前网络类型，默认为 TCP。 */
    };
}


#endif
