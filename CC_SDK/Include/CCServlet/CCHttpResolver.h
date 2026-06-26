#ifndef CCHTTPRESOLVER_H
#define CCHTTPRESOLVER_H

#include "CC.h"
#include "CCString.h"

#define CR '\r'
#define LF '\n'

/**
 * 字符串缓冲区结构体
 */
struct StringBuffer {
    char *begin = nullptr;//字符串开始位置
    char *end = nullptr;//字符串结束位置
    /**
     * 类型转换操作符，将StringBuffer转换为CTL::String
     * @return 转换后的CTL::String对象
     */
    operator CTL::String() const {
        return CTL::String(begin, end);
    }
};

/**
 * Http请求行的状态
 */
enum class HttpRequestDecodeState {
    INVALID,//无效
    INVALID_METHOD,//无效请求方法
    INVALID_URI,//无效的请求路径
    INVALID_VERSION,//无效的协议版本号
    INVALID_HEADER,//无效请求头

    START,//请求行开始
    METHOD,//请求方法

    BEFORE_URI,//请求连接前的状态，需要'/'开头
    IN_URI,//url处理
    BEFORE_URI_PARAM_KEY,//URL请求参数键之前
    URI_PARAM_KEY,//URL请求参数键
    BEFORE_URI_PARAM_VALUE,//URL的参数值之前
    URI_PARAM_VALUE,//URL请求参数值

    BEFORE_PROTOCOL,//协议解析之前
    PROTOCOL,//协议

    BEFORE_VERSION,//版本开始前
    VERSION_SPLIT,//版本分隔符 '.'
    VERSION,//版本

    HEADER_KEY,

    HEADER_BEFORE_COLON,//冒号之前
    HEADER_AFTER_COLON,//冒号
    HEADER_VALUE,//值

    WHEN_CR,//遇到一个回车之后

    CR_LF,//回车换行

    CR_LF_CR,//回车换行之后的状态

    BODY,//请求体

    COMPLETE,//完成
};

namespace CTL {
    /**
 * Http协议解析类
 */
    class HttpResolver {
    public:
        /**
         * 解析请求http协议
         * @param buf 待解析的缓冲区数据
         */
        void tryDecode(const std::string &buf);

        /**
         * 解析响应http协议
         * @param buf 待解析的缓冲区数据
         */
        void tryDecodeResponse(const std::string &buf);

        /**
         * 解析响应数据
         * @param header 请求头
         * @param buf 待解析的缓冲区数据
         * @param callback 解析回调函数
         * @return 解析结果
         */
        int tryDecode2(const std::string& header,const std::vector<char>& buf,
            const std::function<bool(const char*,size_t)>& callback);

        /**
         * 加密数据
         * @param header 响应头
         * @param buf 待加密的数据缓冲区
         * @param callback 加密回调函数
         * @return 加密结果
         */
        int tryEncrypt(const std::string& header,const std::vector<char>& buf,
            const std::function<bool(const char*,size_t)>& callback);

        /**
         * 获取请求方法
         * @return 请求方法字符串
         */
        const std::string &getMethod() const;

        /**
         * 获取请求路径
         * @return 请求路径字符串
         */
        const std::string &getUrl() const;

        /**
         * 获取请求参数
         * @return 请求参数的键值对映射
         */
        const std::map<std::string, std::string> &getRequestParams() const;

        /**
         * 获取协议类型
         * @return 协议类型字符串
         */
        const std::string &getProtocol() const;

        /**
         * 获取协议版本
         * @return 协议版本字符串
         */
        const std::string &getVersion() const;

        /**
         * 获取请求头
         * @return 请求头的键值对映射
         */
        const std::map<CTL::String, CTL::String> &getHeaders() const;

        /**
         * 获取请求体
         * @return 请求体字符串
         */
        const std::string &getBody() const;

        /**
         * 获取状态码
         * @return 状态码
         */
        int getStatusCode() const;

        /**
         * 获取响应信息
         * @return 响应信息字符串
         */
        const std::string& getReasonPhrase() const;

        /**
         * 获取流式解析状态
         * @return 流式解析状态
         */
        bool isStream2Complete() const;

        /**
         * 获取流式解析剩余字节数
         * @return 剩余字节数
         */
        size_t getStream2Remaining() const;
    private:
        /**
         * 内部解析函数
         * @param buf 待解析的数据缓冲区
         * @param size 缓冲区大小
         */
        void parseInternal(const char *buf, int size);
        void parseInternal2(const char *buf, int size);
        void decodeChunkedBody();

    private:
        std::string _method;//请求方法
        std::string _url;//请求路径[不包含请求参数]
        std::map<std::string, std::string> _requestParams;//请求参数
        std::string _protocol;//协议
        std::string _version;//版本
        std::map<CTL::String, CTL::String> _headers;//所有的请求头
        std::string _body;//请求体
        int _nextPos = 0;//下一个位置的
        HttpRequestDecodeState _decodeState = HttpRequestDecodeState::START;//解析状态
        bool _isChunked = false;
        int _statusCode = 0;
        std::string _reasonPhrase;
        // ---- tryDecode2 通用流式解析相关 ----
        enum class Stream2State {
            IDLE,              // 等待首次调用，解析 header
            CHUNKED_SIZE,      // chunked: 解析 chunk 大小
            CHUNKED_DATA,      // chunked: 读取 chunk 数据
            CHUNKED_TRAILER,   // chunked: 跳过尾部 trailer
            CONTENT_LENGTH,    // Content-Length: 按剩余字节读取
            RAW,               // 无长度标记，连接关闭型
            COMPLETE           // 传输完成
        };
        std::vector<char> _stream2Buf;
        Stream2State _stream2State = Stream2State::IDLE;
        size_t _stream2ParsePos = 0;
        size_t _stream2ChunkSize = 0;
        size_t _stream2ContentRemaining = 0;
        bool _stream2IsChunked = false;
        // ---- tryEncrypt 响应编码相关 ----
        enum class EncryptState {
            IDLE,           // 等待首次调用，解析 header 并发送
            SENDING_BODY,   // 正在发送 body 块
            COMPLETE        // 完成
        };
        EncryptState _encryptState = EncryptState::IDLE;
        bool _encryptIsChunked = false;
    };
}

typedef CTL::HttpResolver CCHttpResolver;

#endif
