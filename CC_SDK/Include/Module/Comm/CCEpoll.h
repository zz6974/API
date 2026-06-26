#ifndef CC_EPOLL_H_
#define CC_EPOLL_H_
#define MAX_EVENTS 1024
#include "Socket/CCSocket.h"



#ifdef _WIN32
#include "Sys/wepoll.h"
#elif __linux__
#include "sys/epoll.h"
#endif


#ifdef _WIN32
typedef HANDLE CC_Epoll_t;
#define EPOLL_CLOEXEC 0
#elif __linux__
typedef int CC_Epoll_t;
#endif

typedef epoll_event CC_EP_EV;

enum
{
	CTL_ADD = 1,
	CTL_DEL = 2,
	CTL_MOD = 3
};

namespace CTL {
	class Epoll {
	public:
		CC_Epoll_t Create(int size = 0);
		int Epoll_CTL(const Socket& socket,CC_EP_EV* ev,int op = CTL_ADD) const;
		int Epoll_Wait(CC_EP_EV* events,int MAX_EVENT,int timeout = -1) const;
		void Close() const;
		static void Close(int fd);
		static void Close(const Socket& fd);
		static ByteHander Read(SOCKET sock,char* buffer,size_t size);
		static bool Write(SOCKET sock,const char* buffer,size_t size);
	private:
		CC_Epoll_t epoll_{};
	};
}



#endif
