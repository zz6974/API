#ifndef CCFileOutStream_H
#define CCFileOutStream_H

#include "CC.h"

#ifdef _WIN32
#include <tchar.h>
#endif

using CCOutStream = std::ofstream;

namespace CTL {
    class FileOutStream{
        static std::map<String, std::mutex> mutexMap;
        static std::mutex mapMutex; // 保护 mutexMap 的互斥量
        String m_filename; // 存储文件名
        std::vector<char> m_buffer;
        CCOutStream* m_file;
        void ensure_open() const;
    public:
        // using std::ofstream::basic_ofstream;
        FileOutStream();
        explicit FileOutStream(const String& filename, bool append = false);
        FileOutStream(const String& Path,std::ios::openmode mode);
        FileOutStream(const FileOutStream& other);
        ~FileOutStream();

        void write(uint8_t byte) const;
        void write(const std::vector<uint8_t>& data) const;
        void write(const char* data, size_t size) const;
        void write(const std::vector<uint8_t>& data, size_t offset, size_t length) const;
        void write(const ByteArray& data) const;

        bool open(const CTL::String& Path,std::ios::openmode mode) const;
        [[nodiscard]] bool is_open() const;
        void setBuffer(size_t size);
        void close();
        void flush() const;

        // 基本流操作符重载
        FileOutStream& operator<<(const String& bat);
        FileOutStream& operator<<(int val);
        FileOutStream& operator<<(long val);
        FileOutStream& operator<<(long long val);
        FileOutStream& operator<<(unsigned int val);
        FileOutStream& operator<<(unsigned long val);
        FileOutStream& operator<<(unsigned long long val);
        FileOutStream& operator<<(float val);
        FileOutStream& operator<<(double val);
        FileOutStream& operator<<(long double val);
        FileOutStream& operator<<(bool val);
        FileOutStream& operator<<(char ch);
        FileOutStream& operator<<(const char* str);
        FileOutStream& operator<<(void* ptr);

        // 流状态管理
        [[nodiscard]] bool good() const;
        [[nodiscard]] bool eof() const;
        [[nodiscard]] bool fail() const;
        [[nodiscard]] bool bad() const;
        void clear(std::ios::iostate state = std::ios::goodbit);
        [[nodiscard]] std::ios::iostate rdstate() const;
        void exceptions(std::ios::iostate except);
        [[nodiscard]] std::ios::iostate exceptions() const;

        // 格式控制
        std::ios::fmtflags flags() const;
        std::ios::fmtflags flags(std::ios::fmtflags fmtfl);
        FileOutStream& setf(std::ios::fmtflags fmtfl);
        FileOutStream& setf(std::ios::fmtflags fmtfl, std::ios::fmtflags mask);
        FileOutStream& unsetf(std::ios::fmtflags mask);
        std::streamsize precision() const;
        std::streamsize precision(std::streamsize prec);
        std::streamsize width() const;
        std::streamsize width(std::streamsize wide);
        char fill() const;
        char fill(char ch);

        // 文件位置控制
        [[nodiscard]] std::streampos tellp() const;
        FileOutStream& seekp(std::streampos pos);
        FileOutStream& seekp(std::streamoff off, std::ios::seekdir dir);

        [[nodiscard]] CCOutStream* get_OStream() const;
        //--------------------------------------------------------------------------------------------------------------
        static void WriteFile(const CTL::String& filename, const CTL::String& content,std::ios::openmode mode = std::ios::out | std::ios::binary);
        static void WriteFile(const CTL::String &filename, void* content, size_t size);
        static void Append(const CTL::String& filename, const std::vector<char>& data,std::ios::openmode mode = std::ios::app | std::ios::binary);
        static void Append(const CTL::String& filename, CTL::String data,std::ios::openmode mode = std::ios::app | std::ios::binary);
        static void Append(const CTL::String& filename, CTL::ByteArray& data,std::ios::openmode mode = std::ios::app | std::ios::binary);
        static void Append(const CTL::String& filename, void* content, size_t size);
        static void WriteAt(const CTL::String& filename, size_t position, const char* data, size_t size);
        //--------------------------------------------------------------------------------------------------------------
    private:
        // 获取文件名
        [[nodiscard]] const String& getFilename() const { return m_filename; }
    };
}

typedef CTL::FileOutStream CCFileOutStream;

#endif
