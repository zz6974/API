/*
 * 文件名：CCByte_CCENCODE_H
 * 作者：
 * 日期：
 *
 * It provides support for the encodings:
 * 以下是支持的编码格式:
 *
European languages
ASCII, ISO-8859-{1,2,3,4,5,7,9,10,13,14,15,16},
KOI8-R, KOI8-U, KOI8-RU,
CP{1250,1251,1252,1253,1254,1257}, CP{850,866,1131},
Mac{Roman,CentralEurope,Iceland,Croatian,Romania},
Mac{Cyrillic,Ukraine,Greek,Turkish},
Macintosh
Semitic languages
ISO-8859-{6,8}, CP{1255,1256}, CP862, Mac{Hebrew,Arabic}
Japanese
EUC-JP, SHIFT_JIS, CP932, ISO-2022-JP, ISO-2022-JP-2, ISO-2022-JP-1,
ISO-2022-JP-MS
Chinese
EUC-CN, HZ, GBK, CP936, GB18030, GB18030:2022, EUC-TW, BIG5, CP950,
BIG5-HKSCS, BIG5-HKSCS:2004, BIG5-HKSCS:2001, BIG5-HKSCS:1999,
ISO-2022-CN, ISO-2022-CN-EXT
Korean
EUC-KR, CP949, ISO-2022-KR, JOHAB
Armenian
ARMSCII-8
Georgian
Georgian-Academy, Georgian-PS
Tajik
KOI8-T
Kazakh
PT154, RK1048
Thai
ISO-8859-11, TIS-620, CP874, MacThai
Laotian
MuleLao-1, CP1133
Vietnamese
VISCII, TCVN, CP1258
Platform specifics
HP-ROMAN8, NEXTSTEP
Full Unicode
UTF-8
UCS-2, UCS-2BE, UCS-2LE
UCS-4, UCS-4BE, UCS-4LE
UTF-16, UTF-16BE, UTF-16LE
UTF-32, UTF-32BE, UTF-32LE
UTF-7
C99, JAVA
Full Unicode, in terms of 'uint16_t' or 'uint32_t'
(with machine dependent endianness and alignment)
UCS-2-INTERNAL, UCS-4-INTERNAL
When configured with the option -DENABLE_EXTRA, it also provides support for a few extra encodings:

European languages
CP{437,737,775,852,853,855,857,858,860,861,863,865,869,1125}
Semitic languages
CP864
Japanese
EUC-JISX0213, Shift_JISX0213, ISO-2022-JP-3
Chinese
BIG5-2003 (experimental)
Turkmen
TDS565
Platform specifics
ATARIST, RISCOS-LATIN1
EBCDIC compatible (not ASCII compatible, very rarely used)
European languages
IBM-{037,273,277,278,280,282,284,285,297,423,500,870,871,875,880},
IBM-{905,924,1025,1026,1047,1112,1122,1123,1140,1141,1142,1143},
IBM-{1144,1145,1146,1147,1148,1149,1153,1154,1155,1156,1157,1158},
IBM-{1165,1166,4971}
Semitic languages
IBM-{424,425,12712,16804}
Persian
IBM-1097
Thai
IBM-{838,1160}
Laotian
IBM-1132
Vietnamese
IBM-{1130,1164}
Indic languages
IBM-1137
 *
 * */

// 防止重复包含头文件
#ifndef CCBYTE_CCENCODE_H
#define CCBYTE_CCENCODE_H

// Windows系统特定的预处理指令
#ifdef _WIN32

#endif

// 包含必要的头文件
#include "cppp/reiconv.hpp"
#include "CC.h"

// 使用cppp库的基础命名空间
using namespace cppp::base;

// 定义标准字符集枚举
enum StandardCharsets{
    UTF8,
    GBK,
    GB2312,
    UNICODE_t,
    UTF16,
    UTF16BE,
    UTF16LE,
    UTF32,
    UTF32BE,
    UTF32LE,
    ASCII,
    ISO8859,
    KOI8R
};

// 定义CTL命名空间
namespace CTL{
    // 定义Charset命名空间
    namespace Charset {
        // 根据标准字符集枚举返回对应的字符集格式字符串
        const char* format(StandardCharsets standardCharsets);
    }

    // 定义Encode类
    class Encode{
    public:
        // 格式化字符串，从UTF-8编码转换为目标编码
        static std::string Format(const std::string& utf8Str,const std::string& str);

        // 格式化字符串，从源编码转换为目标编码
        static std::string Format(const std::string& str,const std::string &from,const std::string &to);

        // 格式化字符串，从源编码转换为目标编码
        static std::string formatString(const std::string& str,StandardCharsets from,StandardCharsets to);

        // 格式化字符串，从源编码转换为目标编码，并以字符向量形式返回结果
        static std::vector<char> Format(const std::string& str,StandardCharsets from,StandardCharsets to);

        // 格式化字符数据，从源编码转换为目标编码
        static std::vector<char> Format(const std::vector<char>& data,StandardCharsets from,StandardCharsets to);
    };
}

#endif
