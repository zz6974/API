#ifndef CC_BYTEARRAY_H
#define CC_BYTEARRAY_H

#include "CCByte.h"
#include "CCEncode.h"
#include "CCVector.h"
#include "TL/AutoDestruct.h"

typedef long long ByteHander; //
#define HanderSize sizeof(ByteHander)

namespace CTL {
    /**
     * ByteArray类用于处理字节数组，提供多种方式的字节操作和转换。
     */
    class ByteArray{
        std::vector<Byte> bytes;
    public:
        ByteArray() = default;
        /**
         * 构造函数，从std::string初始化字节数组。
         * @param str 用于初始化字节数组的字符串。
         */
        ByteArray(const std::string& str);
        /**
         * 构造函数，将结构体指针初始化字节数组。
         * @param str 结构体指针。
         * @param size 结构体指针大小。
         */
        ByteArray(const void* str, size_t size);
        /**
         * 构造函数，从C风格字符串初始化字节数组。
         * @param str 用于初始化字节数组的C风格字符串。
         */
        ByteArray(const char* str);
        /**
         * 复制构造函数，从另一个ByteArray对象初始化字节数组。
         * @param other 用于初始化的另一个ByteArray对象。
         */
        ByteArray(const ByteArray& other);
        /**
         * 复制构造函数，从另一个std::vector<unsigned char>对象初始化字节数组。
         * @param other 用于初始化的另一个std::vector<unsigned char>对象。
         */
        ByteArray(const std::vector<unsigned char>& other);
        /**
         * 构造函数，从std::vector<Byte>初始化字节数组。
         * @param bytes 用于初始化字节数组的vector。
         */
        ByteArray(const std::vector<Byte>& bytes);
        /**
         * 构造函数，从std::list<Byte>初始化字节数组。
         * @param bytes 用于初始化字节数组的list。
         */
        ByteArray(const std::list<Byte>& bytes);
        /**
         * 构造函数，从Byte指针初始化字节数组。
         * @param bytes 用于初始化字节数组的Byte指针。
         */
        ByteArray(const Byte* bytes);
        /**
         * 构造函数，从初始化列表初始化字节数组。
         * @param initList 用于初始化字节数组的初始化列表。
         */
        ByteArray(std::initializer_list<Byte> initList);
        /**
         * 构造函数，从初始化列表初始化字节数组。
         * @param ver 用于初始化字节数组的初始化列表。
         */
        ByteArray(const std::vector<char>& ver);
        /**
         * 构造函数，创建指定大小的字节数组。
         * @param size 要创建的字节数组的大小。
         */
        ByteArray(ByteHander size);
        /**
         * 添加一个字节到字节数组末尾。
         * @param byte 要添加的字节。
         */
        void add(Byte byte);
        /**
         * 获取指定索引处的字节。
         * @param index 字节的索引。
         * @return 指定索引处的字节。
         */
        [[nodiscard]] Byte get(int index) const;
        /**
         * 设置指定索引处的字节。
         * @param index 要设置字节的索引。
         * @param byte 要设置的新字节。
         */
        void set(int index, Byte byte);
        /**
         * 删除指定索引处的字节。
         * @param index 要删除字节的索引。
         */
        void remove(int index);
        /**
         * 获取字节数组的大小。
         * @return 字节数组的大小。
         */
        [[nodiscard]] size_t size() const;
        /**
         * 清空字节数组。
         */
        void clear();
        /**
         * 用指定的缓冲区和大小赋值字节数组。
         * @param buffer 指定的缓冲区。
         * @param size 缓冲区的大小。
         */
        void assign(char * buffer, ByteHander size);
        /**
        * 用指定的缓冲区和大小赋值字节数组。
        * @param buffer 指定的缓冲区。
        * @param size 缓冲区的大小。
        */
        void assign(const void* buffer, ByteHander size);
        /**
         * 判断字节数组是否为空。
         * @return 字节数组是否为空。
         */
        [[nodiscard]] bool IsEmpty() const;
        /**
         * 在字节数组末尾追加一个字符串。
         * @param str 要追加的字符串。
         */
        void append(const std::string& str);
        /**
         * 在字节数组末尾追加另一个字节数组。
         * @param str 要追加的字节数组。
         */
        void append(const ByteArray& str);
        /**
         * 在字节数组末尾追加另一个数组列表。
         * @param initList 要追加的数组列表。
         */
        void append(std::initializer_list<Byte> initList);
        /**
         * 返回字节数组的开始迭代器。
         * @return 字节数组的开始迭代器。
         */
        std::vector<Byte>::iterator begin();
        /**
         * 返回字节数组的结束迭代器。
         * @return 字节数组的结束迭代器。
         */
        std::vector<Byte>::iterator end();
        /**
         * 返回常量字节数组的开始迭代器。
         * @return 常量字节数组的开始迭代器。
         */
        std::vector<Byte>::const_iterator begin() const;
        /**
         * 返回常量字节数组的结束迭代器。
         * @return 常量字节数组的结束迭代器。
         */
        std::vector<Byte>::const_iterator end() const;
        /**
         * 从字符串创建字节数组。
         * @param str 用于创建字节数组的字符串。
         * @return 从字符串创建的字节数组。
         */
        static ByteArray fromString(const std::string& str);
        /**
         * 将字节数组转换为字符串。
         * @return 字节数组的字符串表示。
         */
        std::string toString() const;
        /**
         * 将字节数组按指定编码格式转换为字符串。
         * @param EncodeStr 编码格式，默认为"GBK"。
         * @return 按指定编码格式转换后的字符串。
         */
        std::string Format(const std::string& EncodeStr = "GBK") const;
        /**
         * 将字节数组按指定编码格式转换为新的字节数组。
         * @param EncodeStr 编码格式，默认为"GBK"。
         * @return 按指定编码格式转换后的新的字节数组。
         */
        ByteArray toFormat(const std::string& EncodeStr = "GBK") const;
        /**
         * 获取字节数组的底层vector。
         * @return 字节数组的底层vector。
         */
        std::vector<Byte> toVector();
        /**
         * 获取字节数组的底层vector。
         * @return 字节数组的底层vector。
         */
        std::vector<Byte> toVector() const;
        /**
         * 获取字节数组的缓冲区。
         * @return 字节数组的缓冲区。
         */
        char* buffer();
        /**
         * 获取字节数组的缓冲区。
         * @return 字节数组的缓冲区。
         */
        [[nodiscard]] const char* buffer() const;
        /**
         * 获取字节数组的缓冲区。
         * @return 新创建的字节数组的缓冲区(需要手动管理内存)
         */
        char* newBuffer() const;
        /**
         * 调整字节数组的大小。
         * @param len 新的大小。
         */
        void resize(size_t len);
        /**
         * 赋值运算符重载，用于字节数组之间的赋值。
         * @param other 要赋值的另一个字节数组。
         */
        void operator = (const ByteArray& other);
        /**
         * 赋值运算符重载，用于字符串到字节数组的赋值。
         * @param str 要赋值的字符串。
         */
        void operator = (const std::string& str);
        /**
         * 赋值运算符重载，用于列表到字节数组的赋值。
         * @param initList 要赋值字节数组的初始化列表。
         */
        void operator = (std::initializer_list<Byte> initList);
        /**
         * 赋值运算符重载，用于列表到字节数组的赋值。
         * @param ver 要赋值字节数组列表。
         */
        void operator = (const std::vector<char>& ver);
        /**
         * 比较运算符重载，用于比较两个字节数组是否相等。
         * @param other 要比较的另一个字节数组。
         * @return 如果两个字节数组相等则返回true，否则返回false。
         */
        bool operator == (const ByteArray& other) const;
        /**
         * 比较运算符重载，用于比较两个字节数组是否不相等。
         * @param other 要比较的另一个字节数组。
         * @return 如果两个字节数组不相等则返回true，否则返回false。
         */
        bool operator != (const ByteArray& other) const;
        /**
         * 加法运算符重载，用于追加另一个字节数组到当前字节数组末尾。
         * @param other 要追加的另一个字节数组。
         */
        void operator += (const ByteArray& other);
        /**
         * 减法运算符重载，用于从当前字节数组中移除另一个字节数组的内容。
         * @param other 要移除的另一个字节数组。
         */
        void operator -= (const ByteArray& other);
        /**
         * 索引运算符重载，用于获取字节数组中的指定索引处的字节。
         * @param index 要获取的字节索引。
         * @return 指定索引处的字节。
         */
        Byte& operator [] (int index);
        /**
         * 索引运算符重载，用于获取字节数组中的指定索引处的字节。
         * @param index 要获取的字节索引。
         * @return 指定索引处的字节。
         */
        Byte operator [] (int index) const;
        /**
        * 将字节数组转换为指定类型的指针。
        * @param other 要转换的字节数组
        * @return 返回指向字节数组的指定类型的指针(需要手动管理内存)
        */
        template<typename T>
        static T* Conversion(const ByteArray& other) {
            const T* t_const = reinterpret_cast<const T*>(other.newBuffer());
            T* t = const_cast<T*>(t_const);
            return t;
        }
        /**
        * 将字节数组转换为指定类型的指针。
        * @return 指向字节数组的指定类型的指针(RAII管理内存)
        */
        template<typename T>
        AutoDestruct<T[]> Conversion() {
            const T* t_const = reinterpret_cast<const T*>(this->newBuffer());
            T* t = const_cast<T*>(t_const);
            return CTL::AutoDestruct<T[]>(t);
        }
        /**
        * 将字节数组转换为指定类型的指针。
        * @param str 要转换的字节数组
        * @param size 字节数组的长度
        * @return 返回指向字节数组的指定类型的指针(RAII管理内存)
        */
        template<typename T>
        void Conversion(T* str, const ByteHander size) {
            const auto c_str = reinterpret_cast<const char*>(str);
            bytes.assign(&c_str[0], &c_str[0] + size);
        }
        /**
        * 获取字节数组的子数组。
        * @param start 子数组的起始索引。
        * @param end 子数组的结束索引。
        * @return 返回子数组。
        */
        [[nodiscard]] ByteArray subBuffer(ByteHander start, ByteHander end) const;
        /**
         * 将字节数组转换为Base64字符串。
         * @return Base64字符串。
         */
        std::string toBase64();
        /**
         * 将Base64字符串转换为字节数组。
         * @param base64Str Base64字符串。
         * @return 转换后的字节数组。
         */
        static ByteArray fromBase64(const std::string& base64Str);
        /**
         * 将指定值填充到数组的某个范围
         * @param value 要填充的值
         * @param start 起始索引
         * @param end 结束索引（不包含）
        */
        void fill(Byte value, size_t start = 0, size_t end = -1);
        /**
         * 复制数组的一部分到另一个ByteArray
         * @param dest 目标ByteArray
         * @param srcStart 源起始索引
         * @param destStart 目标起始索引
         * @param count 复制的元素数量
        */
        void copyTo(ByteArray& dest, size_t srcStart = 0, size_t destStart = 0, size_t count = -1) const;
        /**
         * 克隆当前ByteArray
         * @return 新的ByteArray副本
        */
        ByteArray clone() const;
        /**
         * 查找特定字节第一次出现的位置
         * @param value 要查找的字节
         * @param start 起始搜索位置
         * @return 找到的位置，未找到返回-1
        */
        int indexOf(Byte value, size_t start = 0) const;
        /**
         * 查找特定字节最后一次出现的位置
         * @param value 要查找的字节
         * @param start 起始搜索位置
         * @return 找到的位置，未找到返回-1
         */
        int lastIndexOf(Byte value, size_t start = -1) const;
        /**
         * 查找字节序列第一次出现的位置
         * @param pattern 要查找的字节序列
         * @param start 起始搜索位置
         * @return 找到的位置，未找到返回-1
         */
        int indexOf(const ByteArray& pattern, size_t start = 0) const;
        /**
         * 查找字节序列最后一次出现的位置
         * @param pattern 要查找的字节序列
         * @param start 起始搜索位置
         * @return 找到的位置，未找到返回-1
         */
        int lastIndexOf(const ByteArray& pattern, size_t start = -1) const;
        /**
         * 比较两个ByteArray是否相等
         * @param other 要比较的另一个ByteArray
         * @return 如果相等返回true，否则返回false
         */
        bool equals(const ByteArray& other) const;
        /**
         * 获取字节数组的char数组形式。
         * @return 字节数组的char数组形式。
         */
        std::vector<char> toCharVector() const;
        ByteArray poll(int begin, ByteHander len);
    };

    /**
     * 在字节数组末尾追加另一个字节数组。
     * @param str 要追加的字节数组。
     */
    inline void ByteArray::append(const ByteArray &str) {
        bytes.insert(bytes.end(), str.bytes.begin(), str.bytes.end());
    }

    /**
     * 重载流插入运算符，用于输出字节数组。
     * @param os 输出流。
     * @param arr 要输出的字节数组。
     * @return 输出流。
     */
    inline std::ostream& operator<<(std::ostream& os, const ByteArray& arr) {
        os << "[";
        for (size_t i = 0; i < arr.size(); ++i) {
            if (i != 0) {
                os << ", ";
            }
            os << static_cast<int>(arr.get(i).get());
        }
        os << "]";
        return os;
    }
}

#endif
