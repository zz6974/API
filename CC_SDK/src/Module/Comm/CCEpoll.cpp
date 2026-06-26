#include "CCEpoll.h"

CC_Epoll_t CTL::Epoll::Create(const int size) {
	epoll_ = epoll_create1(size);
	return epoll_;
}

int CTL::Epoll::Epoll_CTL(const Socket& socket, CC_EP_EV* ev, const int op) const {
	return epoll_ctl(epoll_, op, socket.Socketbit, ev);
}

int CTL::Epoll::Epoll_Wait(CC_EP_EV* events, const int MAX_EVENT, const int timeout) const {
	return epoll_wait(epoll_, events, MAX_EVENT, timeout);
}

void CTL::Epoll::Close() const {
#ifdef _WIN32

#elif __linux__
    close(epoll_);
#endif
}

void CTL::Epoll::Close(const int fd) {
#ifdef _WIN32
	closesocket(fd);
#elif __linux__
    close(fd);
#endif
}

void CTL::Epoll::Close(const Socket& fd) {
#ifdef _WIN32
	closesocket(fd.Socketbit);
#elif __linux__
    close(fd.Socketbit);
#endif
}

ByteHander CTL::Epoll::Read(const SOCKET sock, char* buffer, const size_t size) {
	return recv(sock, buffer, size,0);
}

bool CTL::Epoll::Write(const SOCKET sock, const char* buffer, const size_t size) {
	return send(sock, buffer, size,0) == -1;
}
