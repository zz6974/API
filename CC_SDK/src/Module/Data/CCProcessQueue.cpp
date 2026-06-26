#include "CCProcessQueue.h"
#include <atomic>
#include <cstring>

CTL::ProcessQueue::ProcessQueue() {
    queueNum_ = 0;
    dataSize_ = 0;
    header_ = nullptr;
}

CTL::ProcessQueue::~ProcessQueue() {

}

bool CTL::ProcessQueue::create(const std::string &name,const size_t dataSize,const size_t queueNum) {
    queueNum_ = queueNum;
    dataSize_ = dataSize + sizeof(ProcessQueue_Pack);
    const bool F = shm_.create(name,dataSize_ * queueNum + sizeof(ProcessQueue_Header));
    header_ = static_cast<ProcessQueue_Header *>(shm_.data());
    if (header_->magic != 0xDEADBEEFCAFEBABEULL) {
        header_->magic = 0;
        header_->index = 0;
        header_->data_quantity = queueNum;
        header_->pack_size = dataSize_;
    }
    return F;
}

bool CTL::ProcessQueue::add(const CTL::ByteArray &data) {
    shm_.lockWrite();
    ProcessQueue_Pack pack;
    CTL::ByteArray buff;
    buff.Conversion(&pack,sizeof(ProcessQueue_Pack));
    buff.append(data);
    const auto ind = header_->index;
    header_->index++;
    bool F = shm_.write(header_,sizeof(ProcessQueue_Header),0);
    if (F) {
        F = shm_.write(buff.buffer(),buff.size(),
                sizeof(ProcessQueue_Header) + ind * dataSize_);
    }
    shm_.unlockWrite();
    return F;
}
