#include "CCResponse.h"
#include "CCSystem.h"
#include "CCCookie.h"
#include <algorithm>

CCMutex CTL::Response::_mutex;

CTL::Response::~Response() {

}

void CTL::Response::Init(HttpSocketChannel* channel, CORS* cors) {
	this->channel = channel;
	this->CORS_ = cors;
}

void CTL::Response::setUrl(const String &url) {
	m_url = url;
}

bool CTL::Response::ResponseFile(const CTL::String& Path, const int Status_t) {
	const CCFile file(Path);
	if (!file.isExists()) {
		return false;
	}
	const auto D = CCFileInStream::ReadBinaryFile(Path);
	SetHeader("Content-Type",Request::GetFileType(Path));
	SetHeader("Content-Length",CC::to_String(static_cast<unsigned long>(D.size())));
	SendHeader(Status_t);
	return Flash(D.data(),D.size());
}

bool CTL::Response::Resource(const CCVector<char>& Data, const int Status_t) {
	SendHeader(Status_t);
	return Flash(Data.data(),Data.size());
}

bool CTL::Response::ResourceDownload(const CTL::String &Path, const int SendBufferSize) {
	CCFileInStream file(Path, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("File not found");
	}

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	file.seekg(0, std::ios::beg);

	SetHeader("Connection","close");
	SetHeader("Content-Length", CCSize(size).to_String());
	SetHeader("Content-Type", "application/octet-stream");
	SendHeader(200);

	ByteArray buffer;
	buffer.resize(SendBufferSize);
	while (size > 0) {
		file.read(buffer.buffer(), SendBufferSize);
		const int S = file.gcount();
		if (S > 0) {
			const bool F = Flash(buffer.buffer(), S);
			if (!F) {
				break;
			}
			size -= S;
		}
		else {
			break; // 如果没有读取到数据，退出循环
		}
	}
	file.close();
	return size == 0;
}

void CTL::Response::SendHeader(const int Status_t, const CTL::String& HeaderFlag) {
	CTL::String header;
	header.append("HTTP/1.1 " + CCInt(Status_t).to_String() + " " + HeaderFlag + "\r\n");
	header.append("Server:HTTP->CCWebServlet->Qing_Jiao\r\n");
	for (const auto& item : Headers) {
		CTL::String buf = item.first + ": " + item.second + "\r\n";
		header.append(buf);
	}
	if (CORS_) {
		const auto List = CORS_->GetHeader();
		for (const auto& item : List) {
			CTL::String buf = item.first + ": " + item.second + "\r\n";
			header.append(buf);
		}
	}
	header.append("\r\n");
	Flash(header.data(),header.length());
}

CTL::String CTL::Response::GetHeader(int Status_t, const CTL::String &HeaderFlag) {
	CTL::String header;
	header.append("HTTP/1.1 " + CCInt(Status_t).to_String() + " " + HeaderFlag + "\r\n");
	header.append("Server:HTTP->CCWebServlet->Qing_Jiao\r\n");
	for (const auto& item : Headers) {
		CTL::String buf = item.first + ": " + item.second + "\r\n";
		header.append(buf);
	}
	if (CORS_) {
		const auto List = CORS_->GetHeader();
		for (const auto& item : List) {
			CTL::String buf = item.first + ": " + item.second + "\r\n";
			header.append(buf);
		}
	}
	header.append("\r\n");
	return header;
}

void CTL::Response::ResponseRouting() const {
	CTL::String str;
	char buf[200] = {0};
	str.append("HTTP/1.1 200 OK\r\n");
	str.append("Server:HTTP->CCWeb->OK\r\n");
	str.append("Access-Control-Allow-Origin:*\r\n");
	str.append("Access-Control-Allow-Headers:*, X-Requested-With\r\n");
	str.append("Access-Control-Allow-Methods:GET,POST,PUT,DELETE,OPTIONS\r\n");
	str.append("\r\n");
	Flash(const_cast<char *>(str.c_str()),str.length());
}

void CTL::Response::WriteHeader(const int Status, const CTL::String& ContentType) {
	CTL::String header;
	header.append("HTTP/1.1 " + CCInt(Status).to_String() + " " + ContentType + "\r\n");
	header.append("Server:HTTP->CCWebServlet->Qing_Jiao\r\n");
	for (auto& item : Headers) {
		CTL::String buf = item.first + ": " + item.second + "\r\n";
		header.append(buf);
	}
	if (CORS_) {
		const auto List = CORS_->GetHeader();
		for (const auto& item : List) {
			CTL::String buf = item.first + ": " + item.second + "\r\n";
			header.append(buf);
		}
	}
	header.append("\r\n");
	Flash(header.data(),header.length());
}

void CTL::Response::RangeMediaFile(Request request,const CTL::String& path,CTL::String Content_Type) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        WriteHeader(500, "Internal Server Error");
        Write("Failed to open video file.");
        return;
    }
    const size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
	const CTL::String range = request.GetHeader("Range");
    if (!range.empty()) {
        // Parse the Range header
        std::regex range_regex("bytes=(\\d+)-(\\d*)");
    	std::smatch match;
        if (std::regex_match(range, match, range_regex)) {
	        const size_t start = std::stoul(match[1].str());
            size_t end = match[2].str().empty() ? fileSize - 1 : std::stoul(match[2].str());
            if (end >= fileSize) {
                end = fileSize - 1;
            }
	        const size_t chunkSize = end - start + 1;
            // Set the Content-Range header
            SetHeader("Content-Range", "bytes " + std::to_string(start) + "-" +
            	std::to_string(end) + "/" + std::to_string(fileSize));
            WriteHeader(206, "Partial Content");
            // Seek to the start of the range and read the chunk
            file.seekg(start, std::ios::beg);
            auto buffer = new char[chunkSize];
            file.read(buffer, chunkSize);
            bool F = Flash(buffer, chunkSize);
            delete[] buffer;
        }
    	else {
            WriteHeader(416, "Range Not Satisfiable");
            Write("Invalid Range Header");
        }
    }
    else {
    	if (Content_Type == "-.-") {
    		Content_Type = Request::GetFileType(path);
    	}
        SetHeader("Content-Type", Content_Type);
        SetHeader("Accept-Ranges", "bytes");
        SetHeader("Content-Length", std::to_string(fileSize));
        WriteHeader(200, "OK");
        // Read and send the entire file
	    auto buffer = new char[fileSize];
        file.read(buffer, fileSize);
        bool F = Flash(buffer, fileSize);
        delete[] buffer;
    }
    file.close();
}

void CTL::Response::SendRedirect(const String &url) {
	SetHeader("Location", url);
	SendHeader(302,"Temporary Redirect");
	Flash("",0);
}

CTL::String CTL::Response::getHeader(const String &key) {
	try {
		if (Headers.exist(key)) {
			return Headers[key];
		}
		return "NotFound";
	}
	catch (CCException& e) {
		return "Error";
	}
}

void CTL::Response::BlockTransmission(const std::function<bool(CTL::ByteArray* buff)>& func) {
	SetHeader("Transfer-Encoding", "chunked");
	SetHeader("vary", "origin, access-control-request-method, access-control-request-headers");
	SetHeader("access-control-allow-credentials", "true");
	SetHeader("Strict-Transport-Security", "max-age=31536000; includeSubDomains; preload");
	SendHeader(200);
	CTL::ByteArray data;
	while (true) {
		try {
			data.clear();
			const bool hasMore = func(&data);
			if (data.size() > 0) {
				char hexSize[32] = { 0 };
				snprintf(hexSize, sizeof(hexSize), "%zx", data.size());
				CTL::ByteArray chunk;
				chunk.append(std::string(hexSize));
				chunk.append(std::string("\r\n"));
				chunk.append(data);
				chunk.append(std::string("\r\n"));
				Flash(chunk.buffer(), chunk.size());
			}
			if (!hasMore) {
				break;
			}
		}
		catch (std::exception& e) {
			CTL::System::Println("CTL::Response::BlockTransmission Error: {}", e.what());
		}
	}
	bool F = Flash("0\r\n\r\n", 5);
}

void CTL::Response::SetHeader(const CTL::String& Key, const CTL::String& value) {
	Headers[Key] = value;
}

bool CTL::Response::Write(const CTL::String& Data) const {
	return Flash(Data.data(),Data.length());
}

bool CTL::Response::Flash(const char* buffer, const size_t size) const {
	const auto client = this->channel->getClient();
	for (int i = 0; i < size; i += 4096) {
		const int packetSize = (i + 4096 > size) ? (size - i) : 4096;
		ByteArray buf(buffer + i, packetSize);
		// this->channel->send_queue.Add(buf);
		if (client) {
			client->write(buf);
		}
	}
	return true;
}

bool CTL::Response::Write(const CCVector<char>& Data, const int Status) {
	SetHeader("Connection","close");
	SendHeader(Status);
	return Flash(Data.data(),Data.size());
}

bool CTL::Response::WriteStatus(const CTL::String& Data, const int Status_t) {
    SendHeader(Status_t);
    return Flash(Data.data(), Data.length());
}

bool CTL::Response::Write(const CTL::String& Data, const int Status) {
	SetHeader("Connection","close");
	SendHeader(Status);
	bool F = Flash(Data.data(),Data.length());
	return F;
}

CTL::Response& CTL::Response::Multipart() {
	char buf[64];
	snprintf(buf, sizeof(buf), "----CCWebMultipartBoundary%08x",
		static_cast<unsigned int>(CTL::Thread::getNowMS()));
	_mpBoundary = buf;
	_mpBody.clear();
	_mpFileParts.clear();   // 新增：清空文件引用列表

	SetHeader("Content-Type", CTL::String("multipart/form-data; boundary=") + _mpBoundary);
	return *this;
}

CTL::Response& CTL::Response::MultipartAdd(const CTL::String& name, const CTL::String& value) {
	CTL::String part;
	part.append("--");
	part.append(_mpBoundary);
	part.append("\r\n");
	part.append("Content-Disposition: form-data; name=\"");
	part.append(name);
	part.append("\"\r\n");
	part.append("\r\n");
	part.append(value);
	part.append("\r\n");

	_mpBody.append(part);
	return *this;
}

CTL::Response& CTL::Response::MultipartAddFile(const CTL::String& name, const CTL::String& filePath) {
	CTL::File file(filePath);
	if (!file.isExists()) {
		CTL::System::Println("CTL::Response::MultipartAddFile Error: file not found: {}", filePath.c_str());
		return *this;
	}

	const CTL::String mimeType = Request::GetFileType(filePath);
	const CTL::String fileName = CCFile(filePath).GetName();

	// 记录文件引用，稍后流式发送
	_mpFileParts.push_back({filePath, fileName, mimeType});

	// 构建完整的 multipart 头部（不含文件内容）
	CTL::String part;
	part.append("--");
	part.append(_mpBoundary);
	part.append("\r\n");
	part.append("Content-Disposition: form-data; name=\"");
	part.append(name);
	part.append("\"; filename=\"");
	part.append(fileName);
	part.append("\"\r\n");
	part.append("Content-Type: ");
	part.append(mimeType);
	part.append("\r\n");
	part.append("\r\n");

	_mpBody.append(part);

	// 插入占位标记（后续 MultipartSend 中会被实际文件内容替换）
	constexpr char kPlaceholder[] = "\x1A\x1A\x1A\x1A\x1A\x1A\x1A\x1A";
	_mpBody.append(std::string(kPlaceholder, sizeof(kPlaceholder) - 1));

	// 文件内容后的换行符也保留为普通数据
	_mpBody.append(std::string("\r\n"));

	return *this;
}

CTL::Response& CTL::Response::MultipartAddFile(const CTL::String& name, const CTL::String& filename,
                                                const CTL::ByteArray& data, const CTL::String& mimeType) {
	CTL::String part;
	part.append("--");
	part.append(_mpBoundary);
	part.append("\r\n");
	part.append("Content-Disposition: form-data; name=\"");
	part.append(name);
	part.append("\"; filename=\"");
	part.append(filename);
	part.append("\"\r\n");
	part.append("Content-Type: ");
	part.append(mimeType);
	part.append("\r\n");
	part.append("\r\n");

	_mpBody.append(part);
	_mpBody.append(data);
	_mpBody.append(std::string("\r\n"));

	return *this;
}

CTL::Response& CTL::Response::MultipartAdd(const CTL::String& name, const CTL::ByteArray& data,
                                            const CTL::String& contentType) {
	CTL::String part;
	part.append("--");
	part.append(_mpBoundary);
	part.append("\r\n");
	part.append("Content-Disposition: form-data; name=\"");
	part.append(name);
	part.append("\"\r\n");
	part.append("Content-Type: ");
	part.append(contentType);
	part.append("\r\n");
	part.append("\r\n");

	_mpBody.append(part);
	_mpBody.append(data);
	_mpBody.append(std::string("\r\n"));

	return *this;
}

void CTL::Response::MultipartSend() {
	CTL::String tail;
	tail.append("--");
	tail.append(_mpBoundary);
	tail.append("--\r\n");
	_mpBody.append(tail);

	// 计算所有文件的总大小
	size_t totalFileSize = 0;
	for (const auto& fp : _mpFileParts) {
		CCFile f(fp.filePath);
		totalFileSize += f.GetFileSize();
	}

	// 占位符会被实际文件内容替换，所以总大小 = body大小 - 占位符大小 + 实际文件大小
	constexpr char kPlaceholder[] = "\x1A\x1A\x1A\x1A\x1A\x1A\x1A\x1A";
	constexpr size_t kPlaceholderLen = sizeof(kPlaceholder) - 1;
	const size_t bodySize = _mpBody.size();
	const size_t totalSize = bodySize - (_mpFileParts.size() * kPlaceholderLen) + totalFileSize;

	SetHeader("Content-Length", CC::to_String(static_cast<unsigned long>(totalSize)));
	SendHeader(200);

	// 流式发送 body，遇到占位符时以实际文件内容替代
	const char* bodyPtr = _mpBody.buffer();
	size_t remaining = _mpBody.size();
	size_t fileIdx = 0;

	// 用于文件流式读取的缓冲区（与 ResourceDownload 一致）
	const int kSendBufferSize = 4096;

	while (remaining > 0) {
		const char* placeholderPos = static_cast<const char*>(
			std::search(bodyPtr, bodyPtr + remaining,
			            kPlaceholder, kPlaceholder + kPlaceholderLen));
		const bool found = (placeholderPos != bodyPtr + remaining);

		if (found && fileIdx < _mpFileParts.size()) {
			// 先发送占位符之前的数据
			const size_t prefixLen = placeholderPos - bodyPtr;
			if (prefixLen > 0) {
				Flash(bodyPtr, prefixLen);
			}

			// 流式发送文件内容
			const auto& fp = _mpFileParts[fileIdx];
			CCFileInStream fileStream(fp.filePath, std::ios::in | std::ios::binary);
			if (fileStream.is_open()) {
				CCFile fileObj(fp.filePath);
				size_t fileRemaining = fileObj.GetFileSize();
				ByteArray buffer;
				buffer.resize(kSendBufferSize);
				while (fileRemaining > 0) {
					fileStream.read(buffer.buffer(), kSendBufferSize);
					const int bytesRead = fileStream.gcount();
					if (bytesRead > 0) {
						const bool ok = Flash(buffer.buffer(), bytesRead);
						if (!ok) {
							break;
						}
						fileRemaining -= bytesRead;
					} else {
						break;
					}
				}
				fileStream.close();
			}

			// 跳过占位符
			bodyPtr = placeholderPos + kPlaceholderLen;
			remaining -= (prefixLen + kPlaceholderLen);
			fileIdx++;
		} else {
			// 发送剩余数据
			Flash(bodyPtr, remaining);
			break;
		}
	}
}

// ==================== Cookie 相关实现 ====================

void CTL::Response::SetCookie(const Cookie &cookie) {
	const CTL::String headerValue = cookie.ToSetCookieHeader();
	// 使用 add 方式追加（而非 replace），多个 Set-Cookie 可以共存
	try {
		const CTL::String exist = Headers["Set-Cookie"];
		if (!exist.empty()) {
			Headers["Set-Cookie"] = exist + "\r\nSet-Cookie: " + headerValue;
			return;
		}
	} catch (...) {}
	Headers["Set-Cookie"] = headerValue;
	SetHeader("Connection","close");
}

void CTL::Response::SetCookie(const CTL::String &name, const CTL::String &value) {
	const Cookie c = Cookie::Session(name, value);
	SetCookie(c);
}

void CTL::Response::SetCookie(const CTL::String &name, const CTL::String &value,
	const uint32_t maxAgeSeconds, const bool httpOnly, const bool secure) {
	Cookie c(name, value);
	c.MaxAge  = maxAgeSeconds;
	c.Expires = std::time(nullptr) + maxAgeSeconds;
	c.HttpOnly = httpOnly;
	c.Secure   = secure;
	c.Path     = "/";
	SetCookie(c);
}

void CTL::Response::RemoveCookie(const CTL::String &name, const CTL::String &path) {
	Cookie c = Cookie::Delete(name);
	c.Path = path;
	SetCookie(c);
}

// ==================== Auth 相关实现 ====================

void CTL::Response::RequireBasicAuth(const CTL::String &realm) {
	SetHeader("WWW-Authenticate", "Basic realm=\"" + realm + "\"");
	Write("401 Unauthorized: Basic authentication required", 401);
}

void CTL::Response::RequireBearerAuth(const CTL::String &realm) {
	SetHeader("WWW-Authenticate", "Bearer realm=\"" + realm + "\"");
	Write("401 Unauthorized: Bearer token required", 401);
}

void CTL::Response::SendForbidden(const CTL::String &message) {
	Write(message, 403);
}

void CTL::Response::SendUnauthorized(const CTL::String &message) {
	Write(message, 401);
}
