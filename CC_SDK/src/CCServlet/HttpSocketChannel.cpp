#include "CCServlet/HttpSocketChannel.h"
#include "CCSystem.h"
#include "CCThread.h"

CCMutex CTL::HttpSocketChannel::_mutex_;

int CTL::HttpSocketChannel::receive(char *buffer, const size_t size) const {
    if (!client_t) {
        return -1;
    }
    if (client_t->is_recv_readable()) {
        const auto input = client_t->recv(size);
        if (input.status > 0) {
            const auto size_t = input.data.size();
            std::memcpy(buffer, input.data.buffer(),size_t);
            return size_t;
        }
        else if (input.status == -2){
            return -1;
        }
    }
    return -2;
}

void CTL::HttpSocketChannel::setTimeOut(const int ms) {
    this->TimingTime = ms;
}

void CTL::HttpSocketChannel::setReadFun(const ReadCallBack &cb) {
    CCUniqueLock lock(_mutex_read);
    this->read_cb = cb;
}

int CTL::HttpSocketChannel::getTimeout() const {
    return TimingTime;
}

void CTL::HttpSocketChannel::release() {
    is_release = false;
}

void CTL::HttpSocketChannel::setStartTime(const size_t time) {
    this->start_time = time;
}

void CTL::HttpSocketChannel::MessagePrint(const CTL::String &str, const Logger_Level level) const {
    if (this->level < level) {
        return;
    }
    const CCTimeData time;
    const auto fot = CTL::String::format("[{}] HttpSocketChannel {}: {}",
        time.to_String().c_str(),level, str.c_str());
    CTL::System::Println(fot);
}

CTL::HttpSocketChannel::HttpSocketChannel(EPClient* client) {
    this->client_t = client;
}

CTL::HttpSocketChannel::~HttpSocketChannel() {
    CCUniqueLock lock(_mutex_);
    if (client_t && is_release) {
        client_t->flash_close();
    }
}

CTL::EPClient * CTL::HttpSocketChannel::getClient() const {
    return this->client_t;
}

CTL::String CTL::HttpSocketChannel::readHeader() {
    if (this->client_t) {
        String data;
        data.reserve(4096); // 预分配内存
        int ET = 1;
        while (true){
            const auto end = Thread::getNowMS();
            const size_t keep_out = (end - this->start_time);
            if (keep_out >= this->KeepAliceTime * 1000) {
                keepAliceOut = true;
            }
            if (keepAliceOut) {
                break;
            }
            char buffer[CC_R_BUFFER_SIZE] = {0};
            const auto length = receive(buffer,CC_R_BUFFER_SIZE);
            if(length > 0){
                data.append(buffer, length); // 块追加代替逐个字符处理
                const auto pos = data.find("\r\n\r\n");
                if (pos != CTL::String::npos) {
                    data.resize(pos + 4); // 截断到header结束位置
                    break;
                }
            }
            else if (length == -2) {
                if (ET >= this->TimingTime * 100) {
                    this->MessagePrint("readHeaderHTTPS -> Timeout", Logger_Level::Warning);
                    break;
                }
                ET++;
                Thread::Sleep(1);
            }
            else{
                break;
            }
        }
        return data;
    }
    return SocketClientNULL;
}
