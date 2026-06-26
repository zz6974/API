#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <stdexcept>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace CTL {

    class SharedMemoryError : public std::runtime_error {
    public:
        explicit SharedMemoryError(const std::string& msg)
            : std::runtime_error(msg) {}
    };

    class SharedMemory {
    public:
        using NotifyCallback = std::function<void(const uint8_t*, size_t)>;

        SharedMemory();
        ~SharedMemory();

        SharedMemory(const SharedMemory&) = delete;
        SharedMemory& operator=(const SharedMemory&) = delete;

        bool create(
            const std::string& name,
            size_t dataSize,
            bool createIfMissing = true
        );

        bool open(
            const std::string& name,
            size_t dataSize
        );

        void close();

        /* ========== 锁 ========== */
        void lockWrite();
        void unlockWrite();

        void lockRead();
        void unlockRead();

        /* ========== IO ========== */
        bool write(const void* data, size_t size, size_t offset = 0);
        bool read(void* buffer, size_t size, size_t offset = 0);

        void* data();
        const void* data() const;

        size_t size() const;

        bool isValid() const;

        static bool exists(const std::string& name);
        static bool remove(const std::string& name);

    private:
        struct Header;

        void* base_ = nullptr;
        size_t totalSize_ = 0;
        std::string name_;

#ifdef _WIN32
        HANDLE hMap_ = INVALID_HANDLE_VALUE;
#else
        int fd_ = -1;
#endif
    };

} // namespace CTL