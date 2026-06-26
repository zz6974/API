#ifndef CC_BYTE_H
#define CC_BYTE_H

#include <cstdint>
#include <cstring>
#include "iostream"

namespace CTL {
    class Byte {
        uint8_t value; // 存储单个字节的数据
    public:
        // 构造函数
        Byte(const uint8_t& val = 0) : value(val) {}
        Byte(const char& c) {
            value = c;
        }
        Byte(const int& val) {
            value = val;
        }
        Byte(const size_t& val) {
            value = val;
        }
        // 获取当前字节的值
        [[nodiscard]] uint8_t get() const {
            return value;
        }
        // 设置当前字节的值
        void set(const uint8_t val) {
            value = val;
        }
        // 按位与操作
        Byte operator&(const Byte& other) const {
            return Byte(value & other.value);
        }
        // 按位或操作
        Byte operator|(const Byte& other) const {
            return Byte(value | other.value);
        }
        // 按位异或操作
        Byte operator^(const Byte& other) const {
            return Byte(value ^ other.value);
        }
        // 按位取反操作
        Byte operator~() const {
            return Byte(~value);
        }
        // 左移操作
        Byte operator<<(int n) const {
            return Byte(value << n);
        }
        // 右移操作
        Byte operator>>(int n) const {
            return Byte(value >> n);
        }
        bool operator==(const Byte& other) const {
            return value == other.value;
        }
        bool operator!=(const Byte& other) const {
            return value != other.value;
        }
        bool operator<(const Byte& other) const {
            return value < other.value;
        }
        bool operator>(const Byte& other) const {
            return value > other.value;
        }
        bool operator<=(const Byte& other) const {
            return value <= other.value;
        }
        bool operator>=(const Byte& other) const {
            return value >= other.value;
        }
        bool operator==(const int& other) const {
            return value == other;
        }
        bool operator!=(const int& other) const {
            return value != other;
        }
        bool operator<(const int& other) const {
            return value < other;
        }
        bool operator>(const int& other) const {
            return value > other;
        }
        bool operator<=(const int& other) const {
            return value <= other;
        }
        bool operator>=(const int& other) const {
            return value >= other;
        }
        Byte& operator=(const Byte& other) = default;
        Byte& operator=(uint8_t val) {
            value = val;
            return *this;
        }
        Byte operator++() {
            ++value;
            return *this;
        }
        Byte operator++(int) {
            Byte temp = *this;
            ++value;
            return temp;
        }
        Byte operator--() {
            --value;
            return *this;
        }
        Byte operator--(int) {
            Byte temp = *this;
            --value;
            return temp;
        }
        Byte operator+=(const Byte& other) {
            value += other.value;
            return *this;
        }
        Byte operator-=(const Byte& other) {
            value -= other.value;
            return *this;
        }
        Byte operator*=(const Byte& other) {
            value *= other.value;
            return *this;
        }
        Byte operator/=(const Byte& other) {
            value /= other.value;
            return *this;
        }
        Byte operator%=(const Byte& other) {
            value %= other.value;
            return *this;
        }
        Byte operator<<=(int n) {
            value <<= n;
            return *this;
        }
        Byte operator>>=(int n) {
            value >>= n;
            return *this;
        }
        operator uint8_t() const {
            return value;
        }
        operator uint8_t() {
            return value;
        }
        operator char() const {
            return value;
        }
        operator char() {
            return value;
        }
        // 提供显式的转换操作符
        operator int() const {
            return static_cast<int>(value);
        }
        operator int() {
            return static_cast<int>(value);
        }
        // 输出字节的二进制表示
        friend std::ostream& operator<<(std::ostream& os, const Byte& byte) {
            for (int i = 7; i >= 0; --i) {
                os << ((byte.value >> i) & 1);
            }
            return os;
        }
        friend std::istream& operator>>(std::istream& is, Byte& byte) {
            uint8_t val;
            is >> val;
            byte.value = val;
            return is;
        }
        static Byte* fromCString(const char* cstr) {
            size_t length = strlen(cstr);
            auto* bytes = new Byte[length + 1];
            for (size_t i = 0; i < length; ++i) {
                bytes[i] = cstr[i];
            }
            bytes[length] = '\0';
            return bytes;
        }
    };
}

#endif
