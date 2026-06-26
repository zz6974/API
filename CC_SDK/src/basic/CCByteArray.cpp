#include "basic/CCByteArray.h"
using namespace CTL;

static const char base64_chars[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static const unsigned char base64_decode_table[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,
    0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,
    0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

ByteArray::ByteArray(const std::string &str) {
    bytes.assign(str.begin(), str.end());
}

ByteArray::ByteArray(const void *str, const size_t size) {
    this->Conversion<>(str,size);
}

ByteArray::ByteArray(const char *str) {
    bytes.assign(str, str + strlen(str));
}

ByteArray::ByteArray(const ByteArray &other) {
    bytes = other.bytes;
}

ByteArray::ByteArray(const std::vector<unsigned char> &other) {
    this->bytes.assign(other.begin(), other.end());
}

ByteArray::ByteArray(const std::vector<Byte> &bytes) {
    this->bytes.assign(bytes.begin(), bytes.end());
}

ByteArray::ByteArray(const std::list<Byte> &bytes) {
    this->bytes.assign(bytes.begin(), bytes.end());
}

ByteArray::ByteArray(const Byte *bytes) {
    this->bytes.assign(bytes, bytes + strlen(reinterpret_cast<const char *>(bytes)));
}

ByteArray::ByteArray(const std::initializer_list<Byte> initList) {
    bytes.assign(initList);
}

ByteArray::ByteArray(const std::vector<char> &ver) {
    this->assign(ver.data(), ver.size());
}

ByteArray::ByteArray(const ByteHander size) {
    bytes.resize(size);
}

void ByteArray::add(const Byte byte) {
    bytes.push_back(byte);
}

Byte ByteArray::get(const int index) const {
    if (index < 0 || index >= bytes.size()) {
        throw std::out_of_range("Index out of range");
    }
    return bytes[index];
}

void ByteArray::set(const int index, const Byte byte) {
    if (index < 0 || index >= bytes.size()) {
        throw std::out_of_range("Index out of range");
    }
    bytes[index] = byte;
}

void ByteArray::remove(const int index) {
    if (index < 0 || index >= bytes.size()) {
        throw std::out_of_range("Index out of range");
    }
    bytes.erase(bytes.begin() + index);
}

size_t ByteArray::size() const {
    return bytes.size();
}

void ByteArray::clear() {
    bytes.clear();
}

void ByteArray::assign(char *buffer, const ByteHander size) {
    if (size <= -1) {
        return;
    }
    bytes.assign(buffer, buffer + size);
}

void ByteArray::assign(const void *buffer, const ByteHander size) {
    if (size <= -1) {
        return;
    }
    const char* cstr = static_cast<const char*>(buffer);
    bytes.assign(cstr, cstr + size);
}

bool ByteArray::IsEmpty() const {
    return bytes.empty();
}

void ByteArray::append(const std::string& str) {
    bytes.insert(bytes.end(), str.begin(), str.end());
}

void ByteArray::append(const std::initializer_list<Byte> initList) {
    bytes.insert(bytes.end(), initList.begin(), initList.end());
}

std::vector<Byte>::iterator ByteArray::begin() {
    return bytes.begin();
}

std::vector<Byte>::iterator ByteArray::end() {
    return bytes.end();
}

std::vector<Byte>::const_iterator ByteArray::begin() const {
    return bytes.begin();
}

std::vector<Byte>::const_iterator ByteArray::end() const {
    return bytes.end();
}

ByteArray ByteArray::fromString(const std::string &str) {
    ByteArray byteArray;
    for (const char c : str) {
        byteArray.add(Byte(static_cast<unsigned char>(c)));
    }
    return byteArray;
}

std::string ByteArray::toString() const {
    std::string result;
    for (const Byte& byte : bytes) {
        result += static_cast<char>(byte.get());
    }
    return result;
}

std::string ByteArray::Format(const std::string &EncodeStr) const {
    return Encode::Format(this->toString(), EncodeStr);
}

ByteArray ByteArray::toFormat(const std::string &EncodeStr) const {
    return ByteArray(Encode::Format(this->toString(), EncodeStr));
}

std::vector<Byte> ByteArray::toVector() {
    return bytes;
}

std::vector<Byte> ByteArray::toVector() const{
    return bytes;
}

char * ByteArray::buffer() {
    return reinterpret_cast<char*>(bytes.data());
}

const char* ByteArray::buffer() const {
    return reinterpret_cast<const char*>(bytes.data());
}

char * ByteArray::newBuffer() const {
    const auto str = new char[bytes.size() + 1];
    std::memcpy(str, bytes.data(), bytes.size());
    str[bytes.size()] = '\0';
    return str;
}

void ByteArray::resize(const size_t len) {
    bytes.resize(len);
}

void ByteArray::operator=(const ByteArray &other) {
    bytes = other.bytes;
}

void ByteArray::operator=(const std::string &str) {
    bytes.assign(str.begin(), str.end());
}

void ByteArray::operator=(const std::initializer_list<Byte> initList) {
    bytes = initList;
}

void ByteArray::operator=(const std::vector<char>& ver) {
    this->assign(ver.data(), ver.size());
}

bool ByteArray::operator==(const ByteArray &other) const {
    return bytes == other.bytes;
}

bool ByteArray::operator!=(const ByteArray &other) const {
    return bytes != other.bytes;
}

void ByteArray::operator+=(const ByteArray &other) {
    bytes.insert(bytes.end(), other.bytes.begin(), other.bytes.end());
}

void ByteArray::operator-=(const ByteArray &other) {
    bytes.erase(other.bytes.begin(), other.bytes.end());
}

Byte& ByteArray::operator[](const int index) {
    if (index < 0 || index >= bytes.size()) {
        throw std::out_of_range("Index out of range");
    }
    return *(bytes.data() + index);
}

Byte ByteArray::operator[](const int index) const {
    if (index < 0 || index >= bytes.size()) {
        throw std::out_of_range("Index out of range");
    }
    return *(bytes.data() + index);
}

ByteArray ByteArray::subBuffer(const ByteHander start, const ByteHander end) const {
    if (start > end || end > bytes.size()) {
        throw std::out_of_range("subBuffer Index out of range");
    }
    return {bytes.data() + start, static_cast<size_t>(end - start)};
}

std::string ByteArray::toBase64() {
    std::string ret;
    int i = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];
    auto it = bytes.begin();
    while (it != bytes.end()) {
        char_array_3[i++] = *(it++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++) {
                ret += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }
    if (i) {
        int j = 0;
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++) {
            ret += base64_chars[char_array_4[j]];
        }

        while ((i++ < 3)) {
            ret += '=';
        }
    }
    return ret;
}

ByteArray ByteArray::fromBase64(const std::string &base64Str) {
    ByteArray result;
    unsigned char char_array_4[4];
    unsigned char char_array_3[3];
    size_t i = 0;
    size_t j = 0;

    auto it = base64Str.begin();
    while (it != base64Str.end() && *it != '=') {
        // 跳过非Base64字符
        if (*it == ' ' || *it == '\r' || *it == '\n') {
            ++it;
            continue;
        }

        char_array_4[i++] = static_cast<unsigned char>(*it);
        ++it;

        if (i == 4) {
            for (j = 0; j < 4; j++) {
                char_array_4[j] = base64_decode_table[char_array_4[j]];
            }

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (j = 0; j < 3; j++) {
                result.add(Byte(char_array_3[j]));
            }
            i = 0;
        }
    }

    // 处理剩余字符
    if (i > 0) {
        for (j = i; j < 4; j++) {
            char_array_4[j] = 0;
        }

        for (j = 0; j < i; j++) {
            char_array_4[j] = base64_decode_table[char_array_4[j]];
        }

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; j < i - 1; j++) {
            result.add(Byte(char_array_3[j]));
        }
    }

    return result;
}

void ByteArray::fill(const Byte value, const size_t start, size_t end) {
    if (bytes.empty()) return;

    if (end == static_cast<size_t>(-1)) {
        end = bytes.size();
    }

    if (start >= bytes.size() || end > bytes.size() || start > end) {
        throw std::out_of_range("Index out of range");
    }

    std::fill(bytes.begin() + start, bytes.begin() + end, value);
}

void ByteArray::copyTo(ByteArray &dest, const size_t srcStart, const size_t destStart, size_t count) const {
    if (bytes.empty()) return;

    if (srcStart >= bytes.size()) {
        throw std::out_of_range("Source start index out of range");
    }

    if (count == static_cast<size_t>(-1)) {
        count = bytes.size() - srcStart;
    }

    if (srcStart + count > bytes.size()) {
        throw std::out_of_range("Copy count exceeds source size");
    }

    // 确保目标数组有足够的空间
    if (destStart + count > dest.bytes.size()) {
        dest.bytes.resize(destStart + count);
    }
    std::copy_n(bytes.begin() + srcStart, count, dest.bytes.begin() + destStart);
}

ByteArray ByteArray::clone() const {
    return ByteArray(*this);
}

int ByteArray::indexOf(const Byte value, const size_t start) const {
    if (bytes.empty() || start >= bytes.size()) {
        return -1;
    }

    const auto it = std::find(bytes.begin() + start, bytes.end(), value);
    if (it != bytes.end()) {
        return static_cast<int>(std::distance(bytes.begin(), it));
    }
    return -1;
}

int ByteArray::lastIndexOf(const Byte value, size_t start) const {
    if (bytes.empty()) {
        return -1;
    }

    if (start == static_cast<size_t>(-1) || start >= bytes.size()) {
        start = bytes.size() - 1;
    }

    for (int i = static_cast<int>(start); i >= 0; --i) {
        if (bytes[i] == value) {
            return i;
        }
    }
    return -1;
}

int ByteArray::indexOf(const ByteArray &pattern, const size_t start) const {
    if (pattern.bytes.empty() || bytes.empty() || start >= bytes.size()) {
        return -1;
    }

    auto it = std::search(bytes.begin() + start, bytes.end(),
                         pattern.bytes.begin(), pattern.bytes.end());
    if (it != bytes.end()) {
        return static_cast<int>(std::distance(bytes.begin(), it));
    }
    return -1;
}

int ByteArray::lastIndexOf(const ByteArray &pattern, size_t start) const {
    if (pattern.bytes.empty() || bytes.empty()) {
        return -1;
    }

    if (pattern.bytes.size() > bytes.size()) {
        return -1;
    }

    if (start == static_cast<size_t>(-1) || start >= bytes.size()) {
        start = bytes.size() - 1;
    }

    // 限制start确保不会越界
    if (start < pattern.bytes.size() - 1) {
        return -1;
    }

    size_t end = start - pattern.bytes.size() + 1;
    for (int i = static_cast<int>(start); i >= static_cast<int>(end); --i) {
        bool match = true;
        for (size_t j = 0; j < pattern.bytes.size(); ++j) {
            if (bytes[i - static_cast<int>(pattern.bytes.size()) + 1 + j] != pattern.bytes[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            return i - static_cast<int>(pattern.bytes.size()) + 1;
        }
    }
    return -1;
}

bool ByteArray::equals(const ByteArray &other) const {
    return *this == other;
}

std::vector<char> ByteArray::toCharVector() const{
    return std::vector<char>(bytes.begin(), bytes.end());
}

ByteArray ByteArray::poll(const int begin,ByteHander len) {
    if (begin < 0 || begin >= bytes.size()) {
        return {};
    }

    if (len == 0) {
        return {};
    }

    // 确保不会越界
    if (begin + len > bytes.size()) {
        len = bytes.size() - begin;
    }

    // 提取数据
    ByteArray result;
    result.assign(&buffer()[0] + begin, len);

    // 从原数组中移除
    bytes.erase(bytes.begin() + begin, bytes.begin() + begin + len);

    return result;
}

