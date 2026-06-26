#ifndef CCFIleInStream_H
#define CCFIleInStream_H
#include "CC.h"

using CCInStream = std::ifstream;

namespace CTL {
    class FileInputStream final :public CCInStream{
    public:
        using std::ifstream::basic_ifstream;
        FileInputStream();
        explicit FileInputStream(const CTL::String& filename, std::ios_base::openmode mode = std::ios::in);
        explicit FileInputStream(const std::FILE* file);
        static CTL::String ReadFileDataAll(const CTL::String& filename);
        static CCVector<char> ReadBinaryFile(const CTL::String& filename);
        static CTL::ByteArray ReadFileData(const CTL::String& filename);
        static bool ReadData(FileInputStream &file,CTL::String &line);
        bool ReadData(CTL::ByteArray &data,size_t size = 1024);
        int read();
        int read(char* buffer, size_t size);
        int read(CTL::ByteArray& buffer);
        int read(CTL::ByteArray& buffer, size_t offset, size_t length);
        long skip(long n);
        int available();
        bool open(const String& filename, std::ios_base::openmode mode = std::ios::in);
    protected:

    };
}

typedef CTL::FileInputStream CCFileInStream;


#endif
