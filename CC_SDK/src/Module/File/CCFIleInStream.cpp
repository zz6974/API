#include "CCFIleInStream.h"

using namespace CTL;

FileInputStream::FileInputStream() : CCInStream() {

}

FileInputStream::FileInputStream(const CTL::String &filename, std::ios_base::openmode mode) {
    open(filename, mode);
}

FileInputStream::FileInputStream(const std::FILE *file) {
    if (file) {
        auto fb = std::unique_ptr<std::filebuf>(new std::filebuf());
        fb->open(reinterpret_cast<const char *>(file), std::ios_base::in);
        std::istream::rdbuf(fb.release());
    }
}

CTL::String FileInputStream::ReadFileDataAll(const CTL::String &filename) {
    FileInputStream inputFile;
    const bool F = inputFile.open(filename, std::ios::in);
    if (!inputFile.is_open() || !F) {
        std::cerr << "Unable to open the file: " << filename << std::endl;
        return nullptr;
    }
    CTL::String fileContent((std::istreambuf_iterator<char>(inputFile)),
                            std::istreambuf_iterator<char>());
    inputFile.close();
    return fileContent;
}

CCVector<char> FileInputStream::ReadBinaryFile(const CTL::String &filename) {
    FileInputStream file;
    const bool F = file.open(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open() || !F) {
        throw std::runtime_error("无法打开文件");
    }
    // 获取文件大小
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    // 分配缓冲区
    std::vector<char> buffer(size);
    // 读取文件内容
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("读取文件失败");
    }
    // 关闭文件
    file.close();
    return buffer;
}

CTL::ByteArray FileInputStream::ReadFileData(const CTL::String &filename) {
    FileInputStream file;
    const bool F = file.open(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open() || !F) {
        throw std::runtime_error("无法打开文件");
    }
    // 获取文件大小
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    // 分配缓冲区
    std::vector<char> buffer(size);
    CTL::ByteArray buffer_t;
    // 读取文件内容
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("读取文件失败");
    }
    // 关闭文件
    file.close();
    buffer_t.assign(buffer.data(),size);
    return buffer_t;
}

bool FileInputStream::ReadData(FileInputStream &file, CTL::String &line) {
    const bool ret = std::getline(file, line) ? true : false;
    return ret;
}

bool FileInputStream::ReadData(ByteArray &data, const size_t size) {
    if (!this->is_open()) {
        throw std::runtime_error("无法打开文件");
    }
    bool A = false;
    // const std::unique_ptr<char[]> buffer(new char[size]);
    auto* buffer = new char[size];
    std::ifstream::read(buffer, size);
    const std::streamsize bytesRead = this->gcount();
    if (bytesRead > 0) {
        data.clear();
        data.assign(buffer, static_cast<unsigned int>(bytesRead));
        A = true;
        delete[] buffer;
    }
    return A;
}

int FileInputStream::read() {
    if (!this->is_open()) {
        throw std::runtime_error("Stream is closed");
    }

    char byte;
    this->get(byte);
    if (this->good()) {
        return static_cast<int>(static_cast<unsigned char>(byte));
    }
    return -1; // EOF
}

int FileInputStream::read(char *buffer, const size_t size) {
    if (!this->is_open()) {
        throw std::runtime_error("Stream is closed");
    }
    if (!buffer || size == 0) {
        throw std::runtime_error("buffer is null");
    }
    // 保存当前位置（用于计算实际读取的字节数）
    std::streampos startPos = this->tellg();
    std::ifstream::read(buffer, static_cast<std::streamsize>(size));
    const std::streamsize bytesRead = this->gcount();
    if (bytesRead == 0 && this->eof()) {
        return -1; // EOF
    }
    return static_cast<int>(bytesRead);
}

int FileInputStream::read(CTL::ByteArray &buffer) {
    if (buffer.size() <= 0) {
        buffer.resize(1024);
    }
    return read(buffer, 0, buffer.size());
}

int FileInputStream::read(CTL::ByteArray &buffer, const size_t offset, const size_t length) {
    if (buffer.size() <= 0) {
        buffer.resize(1024);
    }
    if (!this->is_open()) {
        throw std::runtime_error("Stream is closed");
    }
    if (offset + length > buffer.size()) {
        throw std::out_of_range("Buffer overflow");
    }
    // 保存当前位置（用于计算实际读取的字节数）
    std::streampos startPos = this->tellg();

    std::ifstream::read(buffer.buffer() + offset, static_cast<std::streamsize>(length));
    const std::streamsize bytesRead = this->gcount();
    if (bytesRead == 0 && this->eof()) {
        return -1; // EOF
    }
    return static_cast<int>(bytesRead);
}

long FileInputStream::skip(const long n) {
    if (!this->is_open()) {
        throw std::runtime_error("Stream is closed");
    }

    const std::streampos currentPos = this->tellg();
    this->seekg(n, std::ios_base::cur);

    // 检查是否跳到了文件末尾之后
    if (this->fail()) {
        this->clear();
        this->seekg(0, std::ios_base::end);
        const std::streampos endPos = this->tellg();
        return static_cast<long>(endPos - currentPos);
    }

    const std::streampos newPos = this->tellg();
    return static_cast<long>(newPos - currentPos);
}

int FileInputStream::available() {
    if (!this->is_open()) {
        throw std::runtime_error("Stream is closed");
    }
    const std::streampos currentPos = this->tellg();
    this->seekg(0, std::ios_base::end);
    const std::streampos endPos = this->tellg();
    this->seekg(currentPos);
    return static_cast<int>(endPos - currentPos);
}

bool FileInputStream::open(const String& filename, const openmode mode){
#ifdef _WIN32
#ifdef _MSC_VER
#else
#endif
    if (filename.is_str_utf8()) {
        std::ifstream::open(filename.to_CCWString().c_str(),mode);
    }
    else {
        std::ifstream::open(filename,mode);
    }
#elif __linux__
    std::ifstream::open(filename, mode);
#endif
    return is_open();
}
