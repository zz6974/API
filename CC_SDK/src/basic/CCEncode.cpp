#include "basic/CCEncode.h"

const char * CTL::Charset::format(const StandardCharsets standardCharsets) {
    if (standardCharsets == StandardCharsets::UTF8) {
        return "UTF-8";
    }
    if (standardCharsets == StandardCharsets::GBK) {
        return "GBK";
    }
    if (standardCharsets == StandardCharsets::UTF16) {
        return "UTF-16";
    }
    if (standardCharsets == StandardCharsets::UTF16BE) {
        return "UTF-16BE";
    }
    if (standardCharsets == StandardCharsets::UTF16LE) {
        return "UTF-16LE";
    }
    if (standardCharsets == StandardCharsets::UTF32) {
        return "UTF-32";
    }
    if (standardCharsets == StandardCharsets::UTF32BE) {
        return "UTF-32BE";
    }
    if (standardCharsets == StandardCharsets::UTF32LE) {
        return "UTF-32LE";
    }
    if (standardCharsets == StandardCharsets::ASCII) {
        return "ASCII";
    }
    if (standardCharsets == StandardCharsets::GB2312) {
        return "GB2312";
    }
    if (standardCharsets == StandardCharsets::ISO8859) {
        return "ISO-8859-1";
    }
    if (standardCharsets == StandardCharsets::KOI8R) {
        return "KOI8-R";
    }
    if (standardCharsets == StandardCharsets::UNICODE_t) {
        return "UNICODE";
    }
    return nullptr;
}

std::string CTL::Encode::Format(const std::string& utf8Str, const std::string &Estr) {
    auto A = Format(utf8Str,"UTF-8",Estr);
    return A;
}

std::string CTL::Encode::Format(const std::string &str, const std::string &from, const std::string &to) {
    std::vector<char> data(str.begin(), str.end());
    auto A = reiconv::encode(from, to, data);
    return std::string(A.begin(), A.end());
}

std::string CTL::Encode::formatString(const std::string &str, const StandardCharsets from, const StandardCharsets to) {
    std::vector<char> data(str.begin(), str.end());
    auto A = reiconv::encode(Charset::format(from), Charset::format(to), data);
    return std::string(A.begin(), A.end());
}

std::vector<char> CTL::Encode::Format(const std::string &str, const StandardCharsets from, const StandardCharsets to) {
    const std::vector<char> data(str.begin(), str.end());
    return reiconv::encode(Charset::format(from), Charset::format(to), data);
}

std::vector<char> CTL::Encode::Format(const std::vector<char>& data, const StandardCharsets from, const StandardCharsets to) {
    return reiconv::encode(Charset::format(from), Charset::format(to), data);
}
