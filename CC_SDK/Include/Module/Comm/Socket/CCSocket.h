#ifndef CC_Socket_
#define CC_Socket_
#pragma once

#include "CC.h"
#include "CCString.h"

#define SockError -1
#define CC_Sock_Close 0

// 定义主机信息结构体
struct CCHostInfo
{
    std::string IPAddress;
    int Port = -1;
};

// 定义套接字选项枚举
enum CCOpt
{
	BROADCAST,
	REUSEADDR,
	REUSEPORT,
	NOBLOCK,
	LINGER_t,
	SNDBUF,
	RCVBUF,
	TIMESTAMP,
};

// 定义地址类型枚举
enum CCAddress
{
	Local,
	Any
};


#ifdef _WIN32

#define MSL_N 0

// Windows平台下WSA初始化与清理类
class WinWSADWAInitAndClean
{
public:
	WSADATA wsd{};
	WinWSADWAInitAndClean(){
		if (WSAStartup(MAKEWORD(2, 2), &this->wsd) != 0){
			WSACleanup();
		}
	}
	~WinWSADWAInitAndClean(){
		WSACleanup();
	}
};
static WinWSADWAInitAndClean winsc;

#elif __linux__
#define MSL_N MSG_NOSIGNAL
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (SOCKET)(~0)
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include "poll.h"
typedef int SOCKET;
#endif

// 命名空间CTL，包含网络编程相关类和枚举
namespace CTL {
	enum IPVX
	{
		IPV4 = AF_INET,
	   IPV6 = AF_INET6
   };
	enum TORU
	{
		TCP = IPPROTO_TCP,
		UDP = IPPROTO_UDP
	};
	enum TYPE
	{
		STREAM = SOCK_STREAM,
		DGRAM = SOCK_DGRAM
	};
	struct Ethernet {
		std::string Name;
		std::string IP;
		std::string Mac;
	};

	// Socket类，封装了网络编程的常见操作
	class Socket{
	public:
		SOCKET Socketbit = -1;
		sockaddr_in client = {}, server = {};
		sockaddr_in6 client_6 = {}, server_6 = {};
		short IPVx = 0;
		// 默认构造函数
		Socket() = default;
		// 构造函数，初始化Socket
		Socket(SOCKET sock);
		// 初始化Socket并设置属性
		bool Init(IPVX IPV4orIPV6, TORU TCPorUDP, TYPE Type = TYPE::STREAM);
		// 获取Socket句柄
		SOCKET GetSOCKET() const;
		// 设置Socket选项
		bool SetSockOpt(CCOpt opt) const;
		// 设置Socket为非阻塞模式
		void SetSocketNonBlocking() const;
		// 检查Socket是否有可用数据
		bool isDataAvailable() const;
		// 获取本地IP地址
		static std::vector<CTL::String> GetLocalIP(IPVX ipvx = IPVX::IPV4,int Number = 1);
		// 获取客户端主机信息
		CCHostInfo GetClientHost() const;
		// 获取本地主机信息
		CCHostInfo GetLocalHost() const;
		// 连接到远程主机
		bool Connect(const char* IP, unsigned short Port);
		// 绑定Socket到指定地址和端口
		bool Bind(const char* IP,unsigned short Port);
		// 监听连接请求
		bool Listen(unsigned short UserNum = 10);
		// 接受连接请求
		Socket Accept() const;
		// 发送数据
		bool Send(const char* str) const;
		// 发送数据，带标志
		bool Send(const char *__buf, size_t __n, int __flags) const;
		// 发送数据到指定地址和端口
		bool SendData(const char * str,const int length,sockaddr_in addr_in) const;
		// 发送指定长度的数据
		bool SendByte(const char* str,int len) const;
		// 发送指定长度的数据
		bool SendByte(const char* str,int len,int* ret) const;
		// UDP发送数据
		bool UDPSend(const char* str,const char* IP,int Port) const;
		// UDP发送指定长度的数据
		bool UDPSendByte(const char* str, ByteHander len, const char* IP, int Port) const;
		// UDP发送指定长度的数据
		bool UDPSendByte(void* str, ByteHander len, const char* IP, int Port) const;
		// 接收数据
		ByteHander RecvData(char* buffer,ByteHander lens) const;
		// UDP接收数据
		ByteHander UDPRecvData(char* buffer,ByteHander lens,CCHostInfo* info) const;
		// 关闭Socket
		void Close();
		// 获取数据头部
		static bool GetDateHead(char* data, ByteHander* size);
		// 获取字符串头部
		static bool GetStrHead(char* data,ByteHander size);
		// 重载等号运算符，比较Socket对象是否相等
		bool operator == (Socket socket) const;
		// 重载不等号运算符，比较Socket对象是否不等
		bool operator != (Socket socket) const;
		// 检查连接是否有效
		bool isConnectionAlive() const;
		// 检查Socket是否可写
		static bool IsSocketWritable(SOCKET sock);
		static int GetLastError();
	private:
		int sock = -1;
		bool Stop = false;
		int opt = 1;
	};
	class InetAddress {
		std::string hostAddress;
        std::string hostName;
		IPVX Ip_x = IPVX::IPV4;
    public:
        // 默认构造函数
        InetAddress() = default;
        // 根据IP地址构造
        explicit InetAddress(const std::string& ip) : hostAddress(ip) {}
		explicit InetAddress(IPVX ip_x,const std::string& ip= "");
        // 根据IP地址和主机名构造
        InetAddress(const std::string& ip, const std::string& hostname)
            : hostAddress(ip), hostName(hostname) {}
        // 获取本地主机InetAddress对象
        static InetAddress getLocalHost(IPVX ipvx = IPVX::IPV4);
        // 根据主机名获取InetAddress对象
        static std::vector<InetAddress> getAllByName(const std::string& host);
        // 根据IP地址获取InetAddress对象
        static InetAddress getByName(const std::string& host);
        // 获取IP地址字符串
        std::string getHostAddress() const;
        // 获取主机名
        std::string getHostName() const;
        // 获取任意地址 (相当于Java中的InetAddress.getByName("0.0.0.0"))
        static InetAddress getAnyAddress();
        // 获取回环地址 (相当于Java中的InetAddress.getByName("localhost"))
        static InetAddress getLoopbackAddress();
		// 获取广播地址
		static InetAddress getBroadcastAddress(IPVX ipvx = IPVX::IPV4);
        // 重载相等运算符
        bool operator==(const InetAddress& other) const;
        // 重载不等运算符
        bool operator!=(const InetAddress& other) const;
        // 转换为字符串
        std::string toString() const;
		// 获取IP版本
		IPVX getIPx() const;
		// 获取网卡信息列表
		static std::vector<Ethernet> getEthernetList();
	};
}


#endif
