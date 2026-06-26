#include "Socket/EPSocket.h"

#include "CCThread.h"
#include "err.h"

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

int CTL::EPClient::send_t(const char *buffer,const int len) {
    if (ssl_client_t && ssl_client_t->getSSL()) {
        size_t bytes_written = 0;
        int result = SSL_write_ex(ssl_client_t->getSSL(), buffer, len, &bytes_written);

        if (result == 1) {
            return static_cast<int>(bytes_written);
        }

        const int ssl_error = SSL_get_error(ssl_client_t->getSSL(), result);

        switch (ssl_error) {
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_READ:
                return 0;

            case SSL_ERROR_ZERO_RETURN:
                send_queue.pop_front();
                return -4;

            case SSL_ERROR_SYSCALL: {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return 0;
                }
                if (errno == EPIPE || errno == ECONNRESET) {
                    send_queue.pop_front();
                    return -4;
                }
                send_queue.pop_front();
                return -3;
            }

            case SSL_ERROR_SSL: {
                ERR_print_errors_fp(stderr);
                send_queue.pop_front();
                return -2;
            }

            default: {
                send_queue.pop_front();
                return -1;
            }
        }
    }
    else {
#ifdef _WIN32
        const int n = ::send(fd, buffer, len, 0);
        if (n == SOCKET_ERROR) {
            const int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) {
                return false;
            }
            send_queue.pop_front();
            return false;
        }
#else
        ssize_t n = ::send(fd, buffer, len, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return false;
            }
            send_queue.pop_front();
            return false;
        }
#endif
        return n;
    }
}

CTL::EPClient::EPClient() {
    fd = -1;
    epfd = {};
}

CTL::EPClient::~EPClient() {
    fd = -1;
    socket.Close();
    if (ssl_client_t) {
        delete ssl_client_t;
        ssl_client_t = nullptr;
    }
}

void CTL::EPClient::setFd(const SOCKET fd_t,const CC_Epoll_t epfd_t) {
    this->fd = fd_t;
    this->epfd = epfd_t;
    socket = Socket(fd);
}

void CTL::EPClient::setSSL(SSL_Client *ssl_client) {
    this->ssl_client_t = ssl_client;
}

SOCKET CTL::EPClient::getFd() const {
    return fd;
}

void CTL::EPClient::close() {
    socket.Close();
}

long CTL::EPClient::read(char *buffer,const long length) const {
    if (ssl_client_t && ssl_client_t->getSSL()) {
        if (ssl_client_t->available()) {
            const ByteHander len = ssl_client_t->read_ex(buffer, length);
            if (len > 0) {
                return len;
            }
            else if (len == -2) {
                return -1;
            }
            else {
                return -2;
            }
        }
    }
    else {
        if (socket.isDataAvailable()) {
            const ByteHander len = socket.RecvData(buffer,length);
            if (len > 0) {
                return len;
            }
            return -2;
        }
    }
    return -1;
}

CTL::EP_Packet CTL::EPClient::recv(const unsigned long length) {
    EP_Packet Packet;
    Packet.status = -1;
    const auto buffer = new char[length];
    CTL::AutoDestruct<char[]> _buffer(buffer);
    const long len = read(buffer, length);
    if (len == -2 || len == 0) {
        Packet.status = -2;
        socket.Close();
    }
    else {
        Packet.data.assign(buffer, len);
        Packet.status = len;
    }
    return Packet;
}

bool CTL::EPClient::is_recv_readable() const {
    // std::unique_lock<std::mutex> lock(recv_queue_mutex);
    // bool F = !recv_queue.empty();
    // return F;
    if (ssl_client_t && ssl_client_t->getSSL()) {
        return ssl_client_t->available();
    }
    return socket.isDataAvailable();
}

bool CTL::EPClient::flash() {
    std::unique_lock<std::mutex> lock(send_queue_mutex);
    if (send_queue.empty()) {
        if (should_close_after_send) {
            return true;
        }
        return true;
    }
    const CTL::ByteArray data = send_queue.front();
    if (fd < 0 || data.IsEmpty()) {
        send_queue.pop_front();
        if (send_queue.empty() && should_close_after_send) {
            return true;
        }
        return send_queue.empty();
    }
    const char* buffer = data.buffer();
    const size_t total = data.size();
    size_t sent = 0;

    while (sent < total) {
        const auto n = send_t(buffer + sent, static_cast<int>(total - sent));

        if (n <= 0) {
            if (n == 0 || n == -5) {
                break;
            }
            send_queue.pop_front();
            if (send_queue.empty() && should_close_after_send) {
                return true;
            }
            return false;
        }

        sent += n;
    }

    if (sent > 0 && sent < total) {
        send_queue.front() = CTL::ByteArray(buffer + sent, total - sent);
        return false;
    }

    if (sent == total) {
        send_queue.pop_front();
    }

    padding--;

    const bool all_sent = send_queue.empty();
    if (all_sent && should_close_after_send) {
        return true;
    }
    return all_sent;
}

void CTL::EPClient::write(const CTL::ByteArray &data) {
    std::unique_lock<std::mutex> lock(send_queue_mutex);
    padding++;
    send_queue.push_back(data);
    modifyEpollEvent(EPOLLIN | EPOLLRDHUP | EPOLLOUT);
}

void CTL::EPClient::modifyEpollEvent(const uint32_t events) const {
    std::unique_lock<std::mutex> lock(epoll_mutex);
    epoll_event ev_t = {};
    ev_t.events = events;
    ev_t.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev_t);
}

void CTL::EPClient::setClientHost(const CCHostInfo &info) {
    this->client_host = info;
}

int CTL::EPClient::get_padding() const {
    return padding;
}

CCHostInfo CTL::EPClient::getClientHost() const {
    return this->client_host;
}

bool CTL::EPClient::is_ssl() const {
    return ssl_client_t && ssl_client_t->getSSL();
}

SSL* CTL::EPClient::getSSL() const {
    if (!ssl_client_t) {
        return nullptr;
    }
    return ssl_client_t->getSSL();
}

void CTL::EPServer::putClient(EPClient *client) {
    std::unique_lock<std::mutex> lock(clients_mutex);
    if (!client) {
        return;
    }
    clients[client->getFd()] = client;
}

void CTL::EPServer::remove(const EPClient *client) {
    {
        std::unique_lock<std::mutex> lock(clients_mutex);
        if (!client) {
            return;
        }
    }
    if (close_func) {
        close_func(client->getFd());
    }
    std::unique_lock<std::mutex> lock(clients_mutex);
    clients[client->getFd()]->close();
    clients.erase(client->getFd());
}

void CTL::EPServer::handle_accept(SOCKET listen_fd) {
    while (true) {
        CTL::Socket client = socket.Accept();
        const CCHostInfo& client_info = client.GetClientHost();
        if (client_info.Port <= 0) {
            break;
        }
        const SOCKET fd = client.GetSOCKET();
        client.SetSocketNonBlocking();
        create_client_connection(fd, client_info);
    }
}

void CTL::EPServer::create_client_connection(SOCKET fd, const CCHostInfo& client_info) {
    auto client_t = new EPClient();
    client_t->setFd(fd, epfd);
    client_t->setClientHost(client_info);
    const CTL::Socket sock = client_t->getFd();
    const auto ssl_client = SSL_CTX.createClient(&sock);
    client_t->setSSL(ssl_client);

    putClient(client_t);

    epoll_event ev_t = {};
    ev_t.events = EPOLLRDHUP;
    ev_t.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev_t);

    if (open_func) {
        try {
            open_func(fd);
        }
        catch (CCException& e) {
            System::Println("open_func Error: {}", e.what());
        }
    }
}

int CTL::EPServer::handle_client_event(const epoll_event& event) {
    const SOCKET fd = event.data.fd;
    int en_count = 0;
    EPClient* client_t = getClient(fd);
    if (!client_t) {
        return en_count;
    }

    const uint32_t events = event.events;

    if (events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
        remove_client(fd, client_t);
        en_count++;
        return en_count;
    }
    // if (events & EPOLLIN) {
    //     handle_read_event(client_t);
    //     en_count++;
    // }
    if (events & EPOLLOUT) {
        const auto padding = client_t->get_padding();
        if (padding > 0) {
            en_count++;
        }
        handle_write_event(client_t);
    }
    return en_count;
}

void CTL::EPServer::handle_read_event(EPClient* client_t) {
    const SOCKET fd = client_t->getFd();
    constexpr size_t BUFFER_SIZE = 4096;
    static thread_local char read_buffer[BUFFER_SIZE] = { 0 };

    // while (true) {
    //     const long length = client_t->read(read_buffer, BUFFER_SIZE);
    //     if (length > 0) {
    //         if (message_func) {
    //             try {
    //                 message_func(fd);
    //             }
    //             catch (CCException& e) {
    //                 System::Println("message_func Error: {}", e.what());
    //             }
    //         }
    //     }
    //     else if (length == -1) {
    //         break;
    //     }
    //     else if (length == -2 || length == 0) {
    //         break;
    //     }
    //     else {
    //         remove_client(fd, client_t);
    //         return;
    //     }
    // }
    client_t->modifyEpollEvent(EPOLLOUT | EPOLLRDHUP);
}

void CTL::EPServer::handle_write_event(EPClient* client_t) {
    const SOCKET fd = client_t->getFd();
    const bool send_done = client_t->flash();

    if (send_done && client_t->get_flash_close()) {
        remove_client(fd, client_t);
    }
}

void CTL::EPServer::remove_client(SOCKET fd, EPClient* client_t) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    remove(client_t);
}

void CTL::EPServer::cleanup_all_clients() {
    const auto list = getClientFds();
    for (SOCKET fd : list) {
        auto client_t = getClient(fd);
        if (client_t) {
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
            remove(client_t);
        }
    }
#ifdef _WIN32

#else
    close(epfd);
    epfd = -1;
#endif
}

void CTL::EPServer::epoll_running(const SOCKET listen_fd) {
    epfd = epoll_create1(EPOLL_CLOEXEC);
    epoll_event ev = {};
#ifdef __linux__
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
#else
    ev.events = EPOLLIN | EPOLLRDHUP;
#endif
    ev.data.fd = listen_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);
    epoll_event events[MAX_EVENTS];
    if (thread_flag) {
        int b_num = thread_num;
        if (thread_num == -1) {
            b_num = CTL::Thread::getHardWare();
        }
        thread_pool.InitStart(b_num,b_num,1000);
    }
    while (Flag) {
        const int n = epoll_wait(epfd, events, MAX_EVENTS, 100);
        if (n <= 0) {
            continue;
        }
        bool is_sleep = true;
        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == listen_fd) {
                if (thread_flag) {
                    thread_pool.AddTask([this,listen_fd]() {
                        handle_accept(listen_fd);
                    });
                }
                else {
                    handle_accept(listen_fd);
                }
                // handle_accept(listen_fd);
                is_sleep = false;
            }
            else {
                int count = handle_client_event(events[i]);
                if (count > 0 && is_sleep) {
                    is_sleep = false;
                }
            }
        }
        if (is_sleep) {
            CTL::Thread::SleepMS(10);
        }
    }
}

CTL::EPServer::EPServer() {
    clients.clear();
    epfd = {};
}

CTL::EPServer::~EPServer() {
    stop();
}

int CTL::EPServer::listenInit(const std::string &ip,const int port_t,const CTL::IPVX ipvx) {
    socket.Init(ipvx,CTL::TCP,CTL::STREAM);
    bool F = socket.Bind(ip.c_str(), port_t);
    if (!F) {
        Flag = false;
        return -1;
    }
    F = socket.Listen(4096);
    if (!F) {
        Flag = false;
        return -2;
    }
    Flag = true;
    return 1;
}

bool CTL::EPServer::initSSL(const std::string &cert_file, const std::string &key_file, const std::string &pass) {
    bool F = SSL_CTX.init(true);
    if (!F) {
        return false;
    }
    F = SSL_CTX.setSSLCertificate(cert_file);
    if (!F) {
        return false;
    }
    F = SSL_CTX.setSSLKey(key_file, pass);
    return F;
}

int CTL::EPServer::running() {
    if (!Flag) {
        return -3;
    }
    const SOCKET listen_fd = socket.GetSOCKET();
    socket.SetSocketNonBlocking();
    epoll_running(listen_fd);
    cleanup_all_clients();
    socket.Close();
    thread_pool.Stop();
    return 0;
}

void CTL::EPServer::stop() {
    Flag = false;
}

bool CTL::EPServer::is_client_connect(const SOCKET fd) {
    std::unique_lock<std::mutex> lock(clients_mutex);
    const auto i = clients.find(fd);
    if (i != clients.end()) {
        return true;
    }
    return false;
}

void CTL::EPServer::setOpenFunc(const EPServer_Open &func) {
    open_func = func;
}

// void CTL::EPServer::setMessageFunc(const EPServer_Message &func) {
//     message_func = func;
// }

void CTL::EPServer::setCloseFunc(const EPServer_Close &func) {
    close_func = func;
}

CTL::EPClient * CTL::EPServer::getClient(const SOCKET fd) {
    std::unique_lock<std::mutex> lock(clients_mutex);
    const auto i = clients.find(fd);
    if (i != clients.end()) {
        return clients[fd];
    }
    return nullptr;
}

void CTL::EPServer::setMultithreading(const bool flag,const int NumberOfThreads) {
    if (!Flag) {
        return;
    }
    thread_flag = flag;
    thread_num = NumberOfThreads;
}

std::vector<SOCKET> CTL::EPServer::getClientFds() {
    std::vector<SOCKET> fds;
    std::unique_lock<std::mutex> lock(clients_mutex);
    const auto len = clients.size();
    fds.reserve(len);
    for (int i = 0; i < len; ++i) {
        fds.push_back(clients[i]->getFd());
    }
    return fds;
}
