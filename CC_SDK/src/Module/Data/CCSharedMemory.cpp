#include "CCSharedMemory.h"
#include <cstring>
#include <cerrno>

namespace CTL {

#pragma pack(push, 1)
struct SharedMemory::Header {
#ifdef _WIN32
    SRWLOCK rwLock;
#else
    pthread_mutex_t mutex;
#endif
    uint64_t dataSize;
    uint64_t magic = 0xDEADBEEFCAFEBABEULL;
};
#pragma pack(pop)

/* ================== 构造 / 析构 ================== */

SharedMemory::SharedMemory() = default;

SharedMemory::~SharedMemory() {
    close();
}

/* ================== 创建 ================== */

bool SharedMemory::create(
    const std::string& name,
    size_t dataSize,
    bool createIfMissing
) {
    close();
    name_ = name;
    totalSize_ = sizeof(Header) + dataSize;

#ifdef _WIN32
    hMap_ = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        static_cast<DWORD>(totalSize_),
        name.c_str()
    );

    if (!hMap_ || hMap_ == INVALID_HANDLE_VALUE)
        return false;

    base_ = MapViewOfFile(
        hMap_,
        FILE_MAP_ALL_ACCESS,
        0, 0, totalSize_
    );

    if (!base_) {
        CloseHandle(hMap_);
        hMap_ = INVALID_HANDLE_VALUE;
        return false;
    }

    Header* hdr = static_cast<Header*>(base_);
    if (GetLastError() != ERROR_ALREADY_EXISTS) {
        InitializeSRWLock(&hdr->rwLock);
        hdr->dataSize = dataSize;
        hdr->magic = 0xDEADBEEFCAFEBABEULL;
    }

#else
    fd_ = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd_ == -1)
        return false;

    if (ftruncate(fd_, totalSize_) == -1) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    base_ = mmap(nullptr, totalSize_,
        PROT_READ | PROT_WRITE,
        MAP_SHARED, fd_, 0);

    if (base_ == MAP_FAILED) {
        ::close(fd_);
        fd_ = -1;
        base_ = nullptr;
        return false;
    }

    Header* hdr = static_cast<Header*>(base_);
    if (hdr->magic != 0xDEADBEEFCAFEBABEULL) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
#ifdef __OHOS__
#else
        pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
#endif
        pthread_mutex_init(&hdr->mutex, &attr);
        pthread_mutexattr_destroy(&attr);

        hdr->dataSize = dataSize;
        hdr->magic = 0xDEADBEEFCAFEBABEULL;
    }
#endif

    return true;
}

/* ================== 打开 ================== */

bool SharedMemory::open(
    const std::string& name,
    size_t dataSize
) {
    return create(name, dataSize, false);
}

/* ================== 关闭 ================== */

void SharedMemory::close() {
    if (!base_) return;

#ifdef _WIN32
    UnmapViewOfFile(base_);
    if (hMap_ != INVALID_HANDLE_VALUE) {
        CloseHandle(hMap_);
    }
    hMap_ = INVALID_HANDLE_VALUE;
#else
    munmap(base_, totalSize_);
    if (fd_ != -1) {
        ::close(fd_);
    }
    fd_ = -1;
#endif

    base_ = nullptr;
    totalSize_ = 0;
}

/* ================== 锁 ================== */

void SharedMemory::lockWrite() {
    Header* hdr = static_cast<Header*>(base_);
#ifdef _WIN32
    AcquireSRWLockExclusive(&hdr->rwLock);
#else
    pthread_mutex_lock(&hdr->mutex);
#endif
}

void SharedMemory::unlockWrite() {
    Header* hdr = static_cast<Header*>(base_);
#ifdef _WIN32
    ReleaseSRWLockExclusive(&hdr->rwLock);
#else
    pthread_mutex_unlock(&hdr->mutex);
#endif
}

void SharedMemory::lockRead() {
#ifdef _WIN32
    Header* hdr = static_cast<Header*>(base_);
    AcquireSRWLockShared(&hdr->rwLock);
#else
    lockWrite(); // pthread_mutex 不支持读锁
#endif
}

void SharedMemory::unlockRead() {
#ifdef _WIN32
    Header* hdr = static_cast<Header*>(base_);
    ReleaseSRWLockShared(&hdr->rwLock);
#else
    unlockWrite();
#endif
}

/* ================== IO ================== */

bool SharedMemory::write(const void* data, size_t size, size_t offset) {
    if (!base_) return false;

    Header* hdr = static_cast<Header*>(base_);
    if (offset + size > hdr->dataSize)
        return false;

    lockWrite();
    memcpy(
        static_cast<uint8_t*>(base_) + sizeof(Header) + offset,
        data,
        size
    );
    unlockWrite();
    return true;
}

bool SharedMemory::read(void* buffer, size_t size, size_t offset) {
    if (!base_) return false;

    Header* hdr = static_cast<Header*>(base_);
    if (offset + size > hdr->dataSize)
        return false;

    lockRead();
    memcpy(
        buffer,
        static_cast<const uint8_t*>(base_) + sizeof(Header) + offset,
        size
    );
    unlockRead();
    return true;
}

void* SharedMemory::data() {
    return static_cast<uint8_t*>(base_) + sizeof(Header);
}

const void* SharedMemory::data() const {
    return static_cast<const uint8_t*>(base_) + sizeof(Header);
}

size_t SharedMemory::size() const {
    if (!base_) return 0;
    return static_cast<const Header*>(base_)->dataSize;
}

bool SharedMemory::isValid() const {
    return base_ != nullptr;
}

/* ================== 静态工具 ================== */

bool SharedMemory::exists(const std::string& name) {
#ifdef _WIN32
    HANDLE h = OpenFileMappingA(FILE_MAP_READ, FALSE, name.c_str());
    if (h) { CloseHandle(h); return true; }
    return false;
#else
    int fd = shm_open(name.c_str(), O_RDONLY, 0666);
    if (fd != -1) { ::close(fd); return true; }
    return false;
#endif
}

bool SharedMemory::remove(const std::string& name) {
#ifdef _WIN32
    return true; // Windows 自动回收
#else
    return shm_unlink(name.c_str()) == 0;
#endif
}

} // namespace CTL