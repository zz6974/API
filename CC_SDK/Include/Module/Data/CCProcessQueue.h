#pragma once

#include "CCSharedMemory.h"
#include <cstdint>
#include <functional>

#include "CCByteArray.h"

namespace CTL {
    struct ProcessQueue_Header {
        size_t magic = 0xDEADBEEFCAFEBABEULL;
        size_t index;
        size_t data_quantity;
        size_t pack_size;
    };
    struct ProcessQueue_Pack {
        size_t index;
        time_t time;
        size_t size;
    };
    class ProcessQueue {
        SharedMemory shm_;
        size_t queueNum_;
        size_t dataSize_;
        ProcessQueue_Header* header_;
    public:
        ProcessQueue();
        ~ProcessQueue();
        bool create(const std::string& name, size_t dataSize, size_t queueNum);
        bool add(const CTL::ByteArray& data);
    };

}
