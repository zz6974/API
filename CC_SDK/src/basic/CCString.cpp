#include <cassert>
#include "CCString.h"
#define PC
using namespace CTL;

String::String(const std::initializer_list<std::string> list){
    for (auto& i : list) {
        this->append(i);
    }
}

String::String(const CCSStream &stream) {
    this->assign(stream.str());
}

Byte * CTL::String::toBytes() const {
    return (Byte*)this->data();
}

Byte * CTL::String::toBytes() {
    return (Byte*)this->data();
}

CTL::String::String(const basic_string<char> &basicString) {
    this->assign(basicString);
}

CTL::String CTL::String::Encoder(const char * standardCharsets) const {
    if (is_str_utf8()) {
        auto A = Encode::Format(data(),Charset::format(UTF8),standardCharsets);
        return A;
    }
    else {
        auto A = Encode::Format(data(),Charset::format(GBK),standardCharsets);
        return A;
    }
}

CTL::String CTL::String::Encoder(const char * standardCharsets) {
    if (is_str_utf8()) {
        auto A = Encode::Format(data(),Charset::format(UTF8),standardCharsets);
        return A;
    }
    else {
        auto A = Encode::Format(data(),Charset::format(GBK),standardCharsets);
        return A;
    }
}

CTL::String::String(const size_t Size) {
    this->resize(Size);
}

int CTL::String::to_int() const {
    return atoi(data());
}

float CTL::String::to_float() const {
    return atof(data());
}

double CTL::String::to_double() const {
    return atof(data());
}

int CTL::String::to_int() {
    return atoi(data());
}

float CTL::String::to_float() {
    return atof(data());
}

double CTL::String::to_double() {
    return atof(data());
}

bool String::is_str_utf8() const{
    const char* str = this->c_str();
    unsigned int nBytes = 0;//UFT8可用1-6个字节编码,ASCII用一个字节
    unsigned char chr = *str;
    bool bAllAscii = true;
    for (unsigned int i = 0; str[i] != '\0'; ++i){
        chr = *(str + i);
        //判断是否ASCII编码,如果不是,说明有可能是UTF8,ASCII用7位编码,最高位标记为0,0xxxxxxx
        if (nBytes == 0 && (chr & 0x80) != 0){
            bAllAscii = false;
        }
        if (nBytes == 0) {
            //如果不是ASCII码,应该是多字节符,计算字节数
            if (chr >= 0x80) {
                if (chr >= 0xFC && chr <= 0xFD){
                    nBytes = 6;
                }
                else if (chr >= 0xF8){
                    nBytes = 5;
                }
                else if (chr >= 0xF0){
                    nBytes = 4;
                }
                else if (chr >= 0xE0){
                    nBytes = 3;
                }
                else if (chr >= 0xC0){
                    nBytes = 2;
                }
                else{
                    return false;
                }
                nBytes--;
            }
        }
        else{
            //多字节符的非首字节,应为 10xxxxxx
            if ((chr & 0xC0) != 0x80){
                return false;
            }
            //减到为零为止
            nBytes--;
        }
    }
    //违返UTF8编码规则
    if (nBytes != 0) {
        return false;
    }
    if (bAllAscii){ //如果全部都是ASCII, 也是UTF8
        return true;
    }
    return true;
}

bool String::is_str_gbk() const{
    const char* str = this->c_str();
    unsigned int nBytes = 0;//GBK可用1-2个字节编码,中文两个 ,英文一个
    unsigned char chr = *str;
    bool bAllAscii = true; //如果全部都是ASCII,
    for (unsigned int i = 0; str[i] != '\0'; ++i){
        chr = *(str + i);
        if ((chr & 0x80) != 0 && nBytes == 0){// 判断是否ASCII编码,如果不是,说明有可能是GBK
            bAllAscii = false;
        }
        if (nBytes == 0) {
            if (chr >= 0x80) {
                if (chr >= 0x81 && chr <= 0xFE){
                    nBytes = +2;
                }
                else{
                    return false;
                }
                nBytes--;
            }
        }
        else{
            if (chr < 0x40 || chr>0xFE){
                return false;
            }
            nBytes--;
        }//else end
    }
    if (nBytes != 0) {   //违返规则
        return false;
    }
    if (bAllAscii){ //如果全部都是ASCII, 也是GBK
        return true;
    }
    return true;
}

std::vector<std::string> String::split(const std::string& str, const std::string& delimiter){
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);
    while (end != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    tokens.push_back(str.substr(start));
    return tokens;
}

std::vector<std::string> String::split(const std::string& str) const{
    return split(this->data(),str);
}

String String::operator=(const String& other){
    this->assign(other);
    return*this;
}

String String::to_gbk() const{
    if (is_str_utf8()) {
        return Encode::Format(this->data(),"UTF-8","GBK");
    }
    return this->data();
}

String String::to_utf8() const{
    if (is_str_gbk()) {
        return Encode::Format(this->data(),"GBK","UTF-8");
    }
    return this->data();
}





