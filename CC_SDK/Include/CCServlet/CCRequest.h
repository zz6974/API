#ifndef CCWeb_Request_H
#define CCWeb_Request_H
#pragma once

#include "CORS.h"
#include "CCNetwork.h"
#include "CCHttpResolver.h"
#include "CC.h"
#include "CCThread.h"
#include "CCFile.h"
#include "CCObject.h"
#include <CCFIleInStream.h>
#include <CCFileOutStream.h>

#include <utility>
#include "CCJSONObject.h"
#include "CCCookie.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "HttpSocketChannel.h"
#include "EventLoop.h"

namespace CTL {
	struct MultipartPart {
		CCMap<CTL::String, CTL::String> Headers;	///< 该 part 头部（Content-Disposition, Content-Type 等）
		CTL::String Name;							///< 表单字段名（name 属性）
		CTL::String FileName;						///< 文件名（filename 属性，普通字段为空）
		CTL::String ContentType;					///< Content-Type 值
		CCVector<char> Body;						///< 原始二进制数据
		bool IsFile = false;						///< 是否为文件上传
	};
    // 请求数据结构体，用于存储HTTP请求的相关信息
	struct RequestData_t{
		CTL::String RawData; // 原始请求数据
		CTL::String Method; // HTTP方法（GET, POST等）
		CTL::String Path; // 请求路径
		CTL::String Version; // HTTP版本
		CCMap<CTL::String, CTL::String> Headers; // 请求头信息
		CCMap<StdString, StdString> Params; // 请求参数
		ByteArray Body; // 请求体内容
		CCVector<char> Buffer; // 数据缓冲区
		CTL::String Buf; // 临时缓冲区
		CCMap<CTL::String, CTL::String> FileHeaders; // 文件头信息（兼容旧API，指向首个 part）
		CTL::String RawMultipartPrefix; ///< 首个 part 的完整原始头部（含 boundary），用于 ReadMultipartBodyToMemory 重建
		CCVector<MultipartPart> MultipartParts; ///< 解析后的所有 multipart parts
		CCMap<CTL::String, CTL::String> Cookies;   ///< 解析后的请求 Cookie
	};

    /**
     * 请求类，用于处理HTTP请求
     */
	class Request{
		using ReceiveFileCallback = std::function<bool(const CCVector<char>&, size_t)>;
		static CCMutex File_Mutex_T;
		static std::atomic<uint8_t> IsRun;
		static int MAX_UPLOAD_FILE_CONCURRENT;
		// CORS对象指针
		CORS* cors = nullptr;
		// int File_Buffer_Max = 4096;
		bool IsFileUploadSize = true;
		HttpSocketChannel* channel = nullptr;
		bool keep_alive = false;
	public:
        /**
         * 默认构造函数
         */
		Request() = default;

        /**
         * 请求数据成员变量
         */
		RequestData_t RequestData;

		/**
		 * 流式多文件上传回调：name=字段名, filename=文件名, contentType=MIME, data=数据块, length=长度, isLast=是否最后一块
		 * 返回 false 中止传输
		 */
		using MultipartFileCallback = std::function<bool(const CTL::String& name, const CTL::String& filename,
			const CTL::String& contentType, const CCVector<char>& data, size_t length, bool isLast)>;

		/**
		 * 流式普通表单字段回调：name=字段名, value=字段值
		 */
		using MultipartFieldCallback = std::function<void(const CTL::String& name, const CTL::String& value)>;

        /**
         * 获取请求参数的值
         * @param key 参数键名
         * @param Body 是否从Body中获取参数
         * @return 参数对应的值
         */
		CTL::String GetParameter(const CTL::String& key,bool Body = false);

        /**
         * 获取文件信息
         * @param key 文件键名
         * @return 文件信息
         */
		CTL::String GetFileInfo(const CTL::String& key);

		/**
		 * 获取所有 multipart parts
		 * @return multipart parts 列表的引用
		 */
		CCVector<MultipartPart>& GetMultipartParts();

		/**
		 * 获取指定名称的第一个 multipart part
		 * @param name 表单字段名
		 * @return 匹配的 part 指针，未找到返回 nullptr
		 */
		MultipartPart* GetMultipartPart(const CTL::String& name);

		/**
		 * 获取指定名称的所有 multipart parts
		 * @param name 表单字段名
		 * @return 匹配的所有 part 指针列表
		 */
		CCVector<MultipartPart*> GetMultipartParts(const CTL::String& name);

		/**
		 * 获取 multipart part 的数量
		 * @return part 数量
		 */
		size_t GetMultipartPartCount() const;

        /**
         * 获取请求头信息
         * @param key 请求头键名
         * @return 请求头对应的值
         */
		CTL::String GetHeader(const CTL::String& key);

        /**
         * 获取JSON对象
         * @return JSON对象
         */
		[[nodiscard]] CTL::JSONObject GetJson() const;

        /**
         * 获取媒体文件
         * @param Path 文件路径
         * @return 是否成功获取文件
         */
		bool GetMediaFiles(const CTL::String& Path);

		/**
		 * 获取媒体文件
		 * @param callback 接收数据回调函数  --> bool --> CCVector<char> --> size_t
		 * @return 是否执行成功
		 */
		bool GetMediaFiles(const ReceiveFileCallback& callback);

		/**
		 * 流式处理 multipart/form-data，从 socket 逐个 part 读取，不将大文件加载到内存
		 * @param fileCallback 文件数据块回调，返回 false 中止传输
		 * @param fieldCallback 普通表单字段回调（可选）
		 * @param readBufSize 每次从 socket 读取的缓冲区大小，默认 4096
		 * @return 是否成功处理完所有 part
		 */
		bool ProcessMultipartStream(const MultipartFileCallback& fileCallback,
			const MultipartFieldCallback& fieldCallback = nullptr,
			size_t readBufSize = 4096);

		/**
		 * 从 socket 读取全部 multipart body 到内存，填充 MultipartParts
		 * ⚠️ 大文件慎用！会全部加载到内存。单文件/小文件场景方便随机访问 part。
		 * 调用后可通过 GetMultipartParts() / GetMultipartPart() 等方法访问。
		 * @return 是否成功
		 */
		bool ReadMultipartBodyToMemory();

        /**
         * 初始化请求对象
         * @param channel 客户端套接字
         * @param cors CORS对象指针
         */
		void Init(HttpSocketChannel* channel, CORS* cors);

        /**
         * 获取文件后缀名
         * @param filePath 文件路径
         * @return 文件后缀名
         */
		static CTL::String GetFileSuffix(const CTL::String& filePath);

        /**
         * 获取文件类型
         * @param str 文件路径或名称
         * @return 文件类型
         */
		static CTL::String GetFileType(const CTL::String& str);

        /**
         * 获取文件大小
         * @param path 文件路径
         * @return 文件大小
         */
		static unsigned int GetFileSize(const CTL::String& path);

        /**
         * URL解码
         * @param in 需要解码的字符串
         * @return 解码后的字符串
         */
		static CTL::String URL_Decode(const CTL::String& in);

        /**
         * 获取文件日期头信息
         * @param in 输入字符串
         * @param map 存储解析结果的映射表
         * @return 处理后的字符串
         */
		static CTL::String GetFileDateHeader(CTL::String& in, std::map<CTL::String, CTL::String>& map);

		/**
		 * 解析完整的 multipart/form-data body，返回所有 parts
		 * @param body 完整的 multipart body 数据
		 * @param boundary boundary 字符串
		 * @return 解析出的所有 MultipartPart
		 */
		static CCVector<MultipartPart> ParseMultipartFormData(const CTL::String& body, const CTL::String& boundary);

        /**
         * 获取表单数据
         * @param input 输入字符串
         * @param key 键名
         * @param value 键值
         */
		static void GetFormData(CTL::String& input, CTL::String& key, CTL::String& value);

        /**
         * 解析键值对
         * @param input 输入字符串
         * @return 键值对映射表
         */
		static CCMap<CTL::String, CTL::String> parseKeyValuePairs(const CTL::String& input);

		/**
		 * 解析键值对
		 * @param Num 最大文件上传并发数
		 */
		static void SetMaxConcurrentFileUploads(int Num);

		/**
		 * 解析键值对
		 * @param Size 设置文件上传接收缓冲区大小
		 */
		void SetFileUploadSize(int Size);

		/**
		 * 设置是否保持连接
		 * @param Flag 是否保持连接
		 */
		void SetKeepAlive(bool Flag);

		/**
		 * 获取是否保持连接
		 * @return 是否保持连接
		 */
		[[nodiscard]] bool isKeepAlive() const;

		// ==================== Cookie 相关 ====================

		/**
		 * 解析请求头中的 Cookie 到 RequestData.Cookies 映射
		 * 通常在请求初始化后调用一次
		 */
		void ParseCookies();

		/**
		 * 获取指定名称的 Cookie 值
		 * @param name Cookie 名称
		 * @return Cookie 值，若不存在返回空字符串
		 */
		CTL::String GetCookie(const CTL::String& name);

		/**
		 * 获取所有 Cookie
		 * @return Name -> Value 映射的引用
		 */
		CCMap<CTL::String, CTL::String>& GetCookies();

		/**
		 * 获取所有 Cookie（只读）
		 */
		[[nodiscard]] const CCMap<CTL::String, CTL::String>& GetCookies() const;

		/**
		 * 检查是否存在指定名称的 Cookie
		 * @param name Cookie 名称
		 */
		[[nodiscard]] bool HasCookie(const CTL::String& name) const;

		// ==================== Auth 相关 ====================

		/**
		 * 获取 Authorization 头的原始值
		 * @return 如 "Basic xxx" 或 "Bearer xxx"
		 */
		[[nodiscard]] CTL::String GetAuthorizationHeader() const;

		/**
		 * 解析 Basic Auth 凭据
		 * @param outUsername 输出用户名
		 * @param outPassword 输出密码
		 * @return 是否成功解析
		 */
		bool GetBasicAuthCredentials(CTL::String& outUsername, CTL::String& outPassword) const;

		/**
		 * 获取 Bearer Token（JWT 或自定义 Token）
		 * @return Bearer Token 字符串，无则返回空
		 */
		[[nodiscard]] CTL::String GetBearerToken() const;

		/**
		 * 获取请求来源 IP 地址
		 * @return 客户端 IP 字符串
		 */
		[[nodiscard]] CTL::String GetClientIP() const;

		/**
		 * 获取 User-Agent 头
		 */
		[[nodiscard]] CTL::String GetUserAgent() const;

		/**
		 * 获取请求体
		 * @return 请求体
		 */
		ByteArray GetBody() const;
	private:
		static void FileWriting(const CTL::String& path,const CCVector<char>& buffer, size_t size);
		static String getFileName(const CTL::String& contentDisposition);
		/**
		 * 从原始头部字符串解析 multipart part 的元信息
		 * @param headerStr 形如 "Content-Disposition: form-data; name=\"xxx\"; filename=\"yyy\"\r\nContent-Type: ..."
		 * @param outName 输出字段名
		 * @param outFilename 输出文件名（无则为空）
		 * @param outContentType 输出 Content-Type
		 * @return 是否为文件类型
		 */
		static bool ParsePartHeaders(const CTL::String& headerStr,
			CTL::String& outName, CTL::String& outFilename, CTL::String& outContentType);
	};

	class BoundaryMatcher {
		String boundary_pattern;
		size_t matched_length = 0;
	public:
		explicit BoundaryMatcher(String pattern) : boundary_pattern(std::move(pattern)) {}
		bool match_data(const CCVector<char>& data) {
			for (const char i : data) {
				if (matched_length < boundary_pattern.length() &&
					i == boundary_pattern[matched_length]) {
						matched_length++;
						if (matched_length == boundary_pattern.length()) {
							return true; // 完全匹配
						}
					}
				else {
					matched_length = 0; // 不匹配时重置
				}
			}
			return false;
		}
		void reset() {
			matched_length = 0;
		}
		[[nodiscard]] size_t get_matched_length() const {
			return matched_length;
		}
	};

    /**
     * 获取请求头信息
     * @param key 请求头键名
     * @return 请求头对应的值
     */
	inline CTL::String Request::GetHeader(const CTL::String& key) {
		return RequestData.Headers[key];
	}
}

#endif
