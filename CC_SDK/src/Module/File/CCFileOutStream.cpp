#include "CCFileOutStream.h"
#include "CCFile.h"

using namespace CTL;

std::map<String, std::mutex> FileOutStream::mutexMap;
std::mutex FileOutStream::mapMutex;

void FileOutStream::WriteFile(const CTL::String &filename, const CTL::String &content,std::ios::openmode mode) {
    // 获取对应文件的互斥量
    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);

    // 使用ofstream对象打开或创建文件
    std::ofstream file(filename,mode);
    // 检查文件是否成功打开
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return;
    }
    // 写入内容到文件
    file << content;
    // 关闭文件
    file.close();
}

void FileOutStream::WriteFile(const CTL::String &filename, void* content, const size_t size) {
    // 获取对应文件的互斥量
    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);

    std::ofstream file(filename,std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return;
    }
    file.write(static_cast<char*>(content), size);
    file.close();
}

void FileOutStream::Append(const CTL::String &filename, const std::vector<char>& data,std::ios::openmode mode) {
    // 获取对应文件的互斥量
    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);

    // 使用ofstream对象打开或创建文件，并设置为追加模式
    std::ofstream file(filename, mode);
    // 检查文件是否成功打开
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return;
    }
    // 写入数据到文件
    file.write(&data[0], data.size());
    // 关闭文件
    file.close();
}

void FileOutStream::Append(const CTL::String &filename, CTL::String data, std::ios_base::openmode mode) {
    // 获取对应文件的互斥量
    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);

    // 使用ofstream对象打开或创建文件，并设置为追加模式
    std::ofstream file(filename, mode);
    // 检查文件是否成功打开
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return;
    }
    // 写入数据到文件
    file.write(&data[0], data.size());
    // 关闭文件
    file.close();
}

void FileOutStream::Append(const CTL::String &filename, CTL::ByteArray &data, std::ios::openmode mode) {
    // 获取对应文件的互斥量
    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);

    std::ofstream file(filename, mode);
    // 检查文件是否成功打开
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return;
    }
    // 写入数据到文件
    file.write(data.buffer(), data.size());
    // 关闭文件
    file.close();
}

void FileOutStream::Append(const CTL::String &filename, void *content, const size_t size) {
    // 获取对应文件的互斥量
    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);

    std::ofstream file(filename, std::ios::app | std::ios::binary);
    // 检查文件是否成功打开
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return;
    }
    // 写入数据到文件
    file.write(static_cast<char *>(content), size);
    // 关闭文件
    file.close();
}

void FileOutStream::WriteAt(const CTL::String &filename, const size_t position, const char *data, const size_t size) {
    // 获取对应文件的互斥量
    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);

    std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);

    // 检查文件是否成功打开
    if (!file.is_open()) {
        std::cerr << "无法打开文件进行随机访问写入: " << filename << std::endl;
        return;
    }

    // 定位到指定位置
    file.seekp(position, std::ios::beg);

    // 检查定位是否成功
    if (file.fail()) {
        std::cerr << "无法定位到文件位置: " << position << " in " << filename << std::endl;
        file.close();
        return;
    }

    // 在指定位置写入数据
    file.write(data, size);

    // 检查写入是否成功
    if (file.fail()) {
        std::cerr << "写入文件失败: " << filename << std::endl;
        file.close();
        return;
    }

    // 关闭文件
    file.close();
}

FileOutStream::FileOutStream(const CTL::String& Path, std::ios_base::openmode mode){
    m_file = new CCOutStream();
    m_filename = Path;
    if (Path.find(".") == 0) {
        m_filename = CCFile::NormalizePath(Path);
    }
    // 如果文件不存在且模式包含追加或输出，则尝试创建
    if (!std::ifstream(m_filename).good() && (mode & (std::ios::app | std::ios::out))) {
        std::ofstream tmp(m_filename, std::ios::out);
        tmp.close();
    }
    open(m_filename, mode);
    setBuffer(4096);
}

FileOutStream::FileOutStream(const FileOutStream& other){
    m_filename = other.m_filename;
    m_buffer = other.m_buffer;
    m_file = other.m_file;
}

void FileOutStream::ensure_open() const{
    if (!is_open()) {
        throw std::runtime_error("Stream is not open");
    }
}

FileOutStream::FileOutStream(){
    m_file = new CCOutStream();
}

FileOutStream::FileOutStream(const CTL::String &filename, bool append){
    m_file = new CCOutStream();
    m_filename = filename;
    if (filename.find(".") == 0) {
        m_filename = CCFile::NormalizePath(filename);
    }
    std::ios_base::openmode mode = std::ios::binary;
    if (append) {
        mode |= std::ios::app;
        mode |= std::ios::out;
    }
    else {
        mode |= std::ios::out;
    }
    open(m_filename, mode);
    setBuffer(4096);
}

FileOutStream::~FileOutStream() {
    if (is_open()) {
        this->close();
    }
}

void FileOutStream::write(const uint8_t byte) const{
    if (!is_open()) {
        throw std::runtime_error("Stream is not open");
    }

    // 获取当前文件的互斥量
    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    m_file->put(byte);
}

void FileOutStream::write(const std::vector<uint8_t> &data) const{
    if (!is_open()) {
        throw std::runtime_error("Stream is not open");
    }

    // 获取当前文件的互斥量
    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    m_file->write(reinterpret_cast<const char*>(data.data()), data.size());
}

void FileOutStream::write(const char *data, size_t size) const{
    if (!is_open()) {
        throw std::runtime_error("Stream is not open");
    }

    // 获取当前文件的互斥量
    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    m_file->write(&data[0], size);
}

void FileOutStream::write(const std::vector<uint8_t> &data, size_t offset, size_t length) const{
    if (!is_open()) {
        throw std::runtime_error("Stream is not open");
    }

    // 获取当前文件的互斥量
    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    m_file->write(reinterpret_cast<const char*>(&data[0] + offset), length);
}

void FileOutStream::write(const ByteArray& data) const{
    if (!is_open()) {
        throw std::runtime_error("Stream is not open");
    }

    // 获取当前文件的互斥量
    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    m_file->write(&data.buffer()[0], data.size());
}

void FileOutStream::setBuffer(const size_t size) {
    if (size > 0) {
        m_buffer.resize(size);
        m_file->rdbuf()->pubsetbuf(m_buffer.data(), size);
    }
}

bool FileOutStream::open(const CTL::String &Path, const std::ios::openmode mode) const{
#ifdef _WIN32
#ifdef _MSC_VER
#else
#endif
    if (Path.is_str_utf8()) {
        m_file->open(Path.to_CCWString().c_str(),mode);
    }
    else {
        m_file->open(Path,mode);
    }
#elif __linux__
    m_file->open(Path, mode);
#endif
    return is_open();
}

bool FileOutStream::is_open() const{
    if (!m_file) {
        return false;
    }
    return m_file->is_open();
}

void FileOutStream::close(){
    if (m_file) {
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }
}

void FileOutStream::flush() const{
    if (m_file) {
        m_file->flush();
    }
}

CCOutStream* FileOutStream::get_OStream() const{
    return m_file;
}

FileOutStream& FileOutStream::operator<<(const String& bat) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    *m_file << bat;

    return *this;
}

FileOutStream& FileOutStream::operator<<(int val) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    *m_file << val;

    return *this;
}

FileOutStream& FileOutStream::operator<<(long val) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    *m_file << val;

    return *this;
}

FileOutStream& FileOutStream::operator<<(long long val) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    *m_file << val;

    return *this;
}

FileOutStream& FileOutStream::operator<<(unsigned int val) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    *m_file << val;

    return *this;
}

FileOutStream& FileOutStream::operator<<(unsigned long val) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    *m_file << val;

    return *this;
}

FileOutStream& FileOutStream::operator<<(unsigned long long val) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    *m_file << val;

    return *this;
}

FileOutStream& FileOutStream::operator<<(float val) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    *m_file << val;

    return *this;
}

FileOutStream& FileOutStream::operator<<(double val) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    *m_file << val;

    return *this;
}

FileOutStream& FileOutStream::operator<<(long double val) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    *m_file << val;

    return *this;
}

FileOutStream& FileOutStream::operator<<(bool val) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    *m_file << val;

    return *this;
}

FileOutStream& FileOutStream::operator<<(char ch) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    *m_file << ch;

    return *this;
}

FileOutStream& FileOutStream::operator<<(const char* str) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    *m_file << str;

    return *this;
}

FileOutStream& FileOutStream::operator<<(void* ptr) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    *m_file << ptr;

    return *this;
}

// 流状态管理实现
bool FileOutStream::good() const {
    if (!m_file) return false;
    return m_file->good();
}

bool FileOutStream::eof() const {
    if (!m_file) return false;
    return m_file->eof();
}

bool FileOutStream::fail() const {
    if (!m_file) return true;
    return m_file->fail();
}

bool FileOutStream::bad() const {
    if (!m_file) return true;
    return m_file->bad();
}

void FileOutStream::clear(std::ios::iostate state) {
    if (m_file) {
        m_file->clear(state);
    }
}

std::ios::iostate FileOutStream::rdstate() const {
    if (!m_file) return std::ios::badbit;
    return m_file->rdstate();
}

void FileOutStream::exceptions(std::ios::iostate except) {
    if (m_file) {
        m_file->exceptions(except);
    }
}

std::ios::iostate FileOutStream::exceptions() const {
    if (!m_file) return std::ios::goodbit;
    return m_file->exceptions();
}

// 文件位置控制实现
std::streampos FileOutStream::tellp() const {
    ensure_open();
    return m_file->tellp();
}

FileOutStream& FileOutStream::seekp(std::streampos pos) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    m_file->seekp(pos);

    return *this;
}

FileOutStream& FileOutStream::seekp(std::streamoff off, std::ios::seekdir dir) {
    ensure_open();

    std::unique_lock<std::mutex> mapLock(mapMutex);
    auto& fileMutex = mutexMap[m_filename];
    mapLock.unlock();

    std::lock_guard<std::mutex> fileLock(fileMutex);
    m_file->seekp(off, dir);

    return *this;
}

// 格式控制实现
std::ios::fmtflags FileOutStream::flags() const {
    if (!m_file) return std::ios::fmtflags();
    return m_file->flags();
}

std::ios::fmtflags FileOutStream::flags(std::ios::fmtflags fmtfl) {
    if (!m_file) return std::ios::fmtflags();
    return m_file->flags(fmtfl);
}

FileOutStream& FileOutStream::setf(std::ios::fmtflags fmtfl) {
    if (m_file) {
        m_file->setf(fmtfl);
    }
    return *this;
}

FileOutStream& FileOutStream::setf(std::ios::fmtflags fmtfl, std::ios::fmtflags mask) {
    if (m_file) {
        m_file->setf(fmtfl, mask);
    }
    return *this;
}

FileOutStream& FileOutStream::unsetf(std::ios::fmtflags mask) {
    if (m_file) {
        m_file->unsetf(mask);
    }
    return *this;
}

std::streamsize FileOutStream::precision() const {
    if (!m_file) return 0;
    return m_file->precision();
}

std::streamsize FileOutStream::precision(std::streamsize prec) {
    if (!m_file) return 0;
    return m_file->precision(prec);
}

std::streamsize FileOutStream::width() const {
    if (!m_file) return 0;
    return m_file->width();
}

std::streamsize FileOutStream::width(std::streamsize wide) {
    if (!m_file) return 0;
    return m_file->width(wide);
}

char FileOutStream::fill() const {
    if (!m_file) return ' ';
    return m_file->fill();
}

char FileOutStream::fill(char ch) {
    if (!m_file) return ' ';
    return m_file->fill(ch);
}

