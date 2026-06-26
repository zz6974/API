#include "CCWebServlet.h"
#include <regex>
#include "CCResources.h"
#include "CCSystem.h"
#include "CCGZip.h"
#include "CCAuth.h"

using namespace CTL;

CTL::WebServlet::WebServlet() {
#if __linux__
	signal(SIGPIPE, SIG_IGN);
#endif
	server = new EPServer;
}

CTL::WebServlet::~WebServlet() {
	Stop();
	if (CORS_Header) {
		delete CORS_Header;
		CORS_Header = nullptr;
	}
	delete server;
	server = nullptr;
}

bool CTL::WebServlet::Init(const InetAddress &address, const int Port) const {
	return server->listenInit(address.getHostAddress(), Port,this->IPvx);
}

bool WebServlet::Init(const String &address, const int Port) const {
	return server->listenInit(address, Port,this->IPvx);
}

bool CTL::WebServlet::SetWebSocket(const CTL::WSBind& bind) {
	WebSocketFlag = true;
	return m_WebSocket.InitBind(bind);
}

bool WebServlet::InitSSL(const std::string &cert_file, const std::string &key_file, const std::string &pass) const {
	return server->initSSL(cert_file, key_file, pass);
}

int CTL::WebServlet::Running(const bool Ws, const bool InitStaticFlag) {
	if (!server) {
		return -1;
	}
	ServerFlag = true;
	if (InitStaticFlag) {
		InitStatic();
	}
	M_S_POOl.InitStart(M_Pool_Size, M_Pool_Max, M_Pool_KeepAliveTime);
	server->setMultithreading(true, M_Pool_Size);
	if (Ws && WebSocketFlag) {
		m_WebSocket.setEPSocket(server);
		m_WebSocket.InitTick(300);
	}
	server->setOpenFunc([this](const SOCKET& fd) {
		M_S_POOl.AddTask(&WebServlet::ProcessingClientRequests,this,fd);
	});
	server->running();
	M_S_POOl.Stop();
	return 0;
}

void CTL::WebServlet::Stop() {
	if (server) {
		server->stop();
		server = nullptr;
	}
	event_loop_t.stop();
	ServerFlag = false;
	m_WebSocket.Stop();
}

bool CTL::WebServlet::AddStaticPath(const CTL::String& Path) {
	const CCFile file(Path);
	if (file.isExists()) {
		StaticPath.add(Path);
		return true;
	}
	return false;
}

bool WebServlet::SetResources(const CTL::String &Path) {
	// 从 Pak 中获取指定目录下所有文件（递归，仅文件）
	const auto fileList = CTL::PakReader::getPakListAll(Path.c_str(), true);

	if (fileList.empty()) {
		return false;
	}

	bool loadedAny = false;

	for (const auto& pakItem : fileList) {
		// 拼接完整 pak 内路径: Path + "/" + pakItem.name
		std::string fullPath;
		if (Path.empty()) {
			fullPath = pakItem.name;
		} else {
			fullPath = Path.c_str();
			fullPath += "/";
			fullPath += pakItem.name;
		}

		// 从 Pak 读取文件内容（自动解压）
		std::vector<unsigned char> data = CTL::Resources::get(fullPath, CTL::RES_PAK);
		if (data.empty()) {
			continue;
		}

		StaticResource res;
		res.Name = pakItem.name;
		res.Path = fullPath;
		res.Type = Request::GetFileType(res.Name);
		res.Content = CCVector<char>(data.begin(), data.end());

		// GZip 压缩（与 DirInitStatic 保持一致）
		if (GZipFlag) {
			res.ContentGZip = GZip::compress(res.Content.data(), res.Content.size());
		}

		// 以 URL 路径格式作为 key，匹配 HandleRequest 中的查找逻辑
		// HandleRequest 中: Path = ProcessPath(request.RequestData.Path) → NormalizePath
		// 无 StaticPath 匹配时 ProcessPath 返回原 URL 路径（如 "/html/index.html"）
		String key = CCFile::NormalizePath("/" + pakItem.name);
		StaticResources.put(key, res);
		loadedAny = true;
	}

	return loadedAny;
}

void CTL::WebServlet::SetWebServlet(const CTL::String& UrlPath, const RequestFunc& RFunc) {
	RequestFun.put(UrlPath, RFunc);
}

void CTL::WebServlet::SetWebServletFun(RequestFunc RFunc) {
	WebSignalFun = std::move(RFunc);
}

void CTL::WebServlet::SetWebServletWithAuth(const CTL::String& UrlPath,
                                            std::shared_ptr<IAuthProvider> authProvider,
                                            const RequestFunc& RFunc) {
	AuthMiddleware auth(std::move(authProvider), RFunc);
	RequestFun.put(UrlPath, auth.GetHandler());
}

void CTL::WebServlet::SetGlobalAuthProvider(std::shared_ptr<IAuthProvider> authProvider) {
	m_globalAuthProvider = std::move(authProvider);
}

void CTL::WebServlet::SetWebCORS(const CORS& cors) {
	CORS_Header = new CORS(cors);
}

CTL::WebSocketServer* CTL::WebServlet::GetWebSocket()  {
	return &m_WebSocket;
}

void WebServlet::SetTimeout(const unsigned long MS) {
	this->TimingTime = MS;
}

void WebServlet::SetReadHeaderBufferSize(const unsigned int Size) {
	this->ReadHBuff = Size;
}

void WebServlet::SetThreadPool(const size_t Size, const size_t Max, const int KeepAliveTime) {
	M_Pool_Size = Size;
	M_Pool_Max = Max;
	M_Pool_KeepAliveTime = KeepAliveTime;
}

void WebServlet::SetIPType(const IPVX IPvx) {
	this->IPvx = IPvx;
}

void CTL::WebServlet::MessagePrint(const CTL::String& str) {
	CC::TimePrintln(str);
}

bool CTL::WebServlet::Permission(const HttpSocketChannel* client_channel) const{
	if (!client_channel) {
		return false;
	}
	const auto socket = client_channel->getClient();
	if (!socket) {
		return false;
	}
	bool Restrict = false;
	if (CORS_Header) {
		if (CORS_Header->RestrictIP()) {
			CCVar L = CORS_Header->GetAllowOrigin();
			for (const auto& i : L) {
				if (i == socket->getClientHost().IPAddress) {
					Restrict = true;
					break;
				}
			}
		}
		else {
			Restrict = true;
		}
	}
	else {
		Restrict = true;
	}
	return Restrict;
}

void CTL::WebServlet::ProcessingClientRequests(const SOCKET& fd) {
	const auto socket = server->getClient(fd);
	if (!socket) {
		return;
	}
	const auto client_channel = new HttpSocketChannel(socket);
	AutoDestruct<HttpSocketChannel> client_t(client_channel);
	client_channel->setTimeOut(this->TimingTime);
	const auto start_time = Thread::getNowMS();
	client_channel->setStartTime(start_time);
	client_channel->KeepAliceTime = this->KeepTimeOut;
	while (true) {
		String Data = client_channel->readHeader();
		if(Data.empty() || Data == SocketClientNULL) {
			return;
		}
		Request request;
		CCHttpResolver resolver;
		resolver.tryDecode(Data);
		request.RequestData.RawData = Data;
		request.RequestData.Body = resolver.getBody();
		request.RequestData.Headers = resolver.getHeaders();
		request.RequestData.Method = resolver.getMethod();
		request.RequestData.Path = resolver.getUrl();
		request.RequestData.Version = resolver.getVersion();
		request.RequestData.Params = resolver.getRequestParams();
		if(request.RequestData.Path.empty()) {
			return;
		}
		request.Init(client_channel,this->CORS_Header);
		client_channel->KeepAliceCount++;
		if (client_channel->KeepAliceCount >= this->KeepMax) {
			client_channel->keepAliceOut = true;
		}
		try {
			client_channel->URL = request.RequestData.Path;
			const int i = ProcessingDataType(client_channel,request);
			if (i == 1) {
				client_channel->release();
				return;
			}
			HandleRequest(client_channel,request);
		}
		catch (CCException& e) {
			MessagePrint("ProcessingClientRequests -> " + CTL::String(e.what()));
			ErrorHandling(client_channel,request,500);
		}
		if (client_channel->keepAliceOut) {
			break;
		}
	}
}

int CTL::WebServlet::ProcessingDataType(HttpSocketChannel* client_channel,Request& request) {
	const auto client = client_channel->getClient();
	if (!client) {
		return -2;
	}
	auto st = request.RequestData.Headers["content-type"];
	if(st.empty()){
		st = request.RequestData.Headers["Content-Type"];
	}
	if (st.empty()) {
		for (const auto& i : request.RequestData.Headers) { //Sec-WebSocket-Extensions
			if (i.first.find("Sec-WebSocket-Extensions") != CTL::String::npos) {
				Thread::addTask([this](const SOCKET& fd,const CTL::String& data) {
					this->m_WebSocket.HandleClient(fd,data);
				}, client->getFd(),request.RequestData.RawData);
				return 1;
			}
		}
	}
	if (request.RequestData.RawData.find("json") != CTL::String::npos) {
		st = "application/json";
	}
	if(st.find("multipart/form-data") != CTL::String::npos){ //multipart/form-data; boundary=--------------------------479759322608369375041773
		request.RequestData.FileHeaders = GetMultipart_Form_DataInfo(client_channel, request.RequestData.Buf);
		// 提取原始 multipart 前缀（用于 ReadMultipartBodyToMemory）
		request.RequestData.RawMultipartPrefix = request.RequestData.FileHeaders["__RawMultipartPrefix__"];
		request.RequestData.FileHeaders.erase("__RawMultipartPrefix__");
		for (auto i : request.RequestData.Buf) {
			request.RequestData.Buffer.push_back(i);
		}
		return 2;
	}
	else if(st.find("application/octet-stream") != CTL::String::npos) {
		return 3;
	}
	else if(st.find("application/x-www-form-urlencoded") != CTL::String::npos) {
		request.RequestData.Body = GetApp_Form_Data(client_channel,request);
		return 4;
	}
	else if (st.find("application/json") != CTL::String::npos){
		request.RequestData.Body = GetApp_Form_Data(client_channel,request);
		return 5;
	}
	else {
		try {
			request.RequestData.Body = GetRequestAll(client_channel,request);
			return 6;
		}
		catch (CCException& e) {
			return -1;
		}
	}
}

void CTL::WebServlet::HandleRequest(HttpSocketChannel* client_channel, Request& request) {
	const bool Restrict = Permission(client_channel);
	if (!Restrict) {
		return;
	}
	Response response;
	response.setUrl(request.RequestData.Path);
	response.Init(client_channel,this->CORS_Header);
	String connection_header = "";
	try {
		connection_header = request.RequestData.Headers.at("Connection");
	}
	catch (...){}
	if (connection_header == "keep-alive") {
		request.SetKeepAlive(true);
		response.SetHeader("Connection", "keep-alive");
		client_channel->keepAliceOut = false;
		const String str = String::format("timeout={}, max={}",this->KeepTimeOut,this->KeepMax);
		response.SetHeader("Keep-Alive", str);
	}
	else {
		client_channel->keepAliceOut = true;
	}
	if (client_channel->keepAliceOut) {
		response.SetHeader("Connection","close");
	}
	if (request.RequestData.Method == "GET") {
		const auto CC_S_Fun = RequestFun[request.RequestData.Path];
		if (CC_S_Fun) {
			CC_S_Fun->operator()(request,response);
			const String ConnStr = response.getHeader("Connection");
			if (CC::toLowerCase(ConnStr) == "close") {
				client_channel->keepAliceOut = true;
			} else {
				// 处理器显式设置了 keep-alive（如 SSE 长连接），不要强制关闭
				client_channel->keepAliceOut = false;
			}
		}
		else {
			// ★ 先尝试 WebSignalFun 回退（支持动态路由如 /api/camera/1/frame）
			if (WebSignalFun) {
				WebSignalFun(request, response);
				const String ConnStr = response.getHeader("Connection");
				if (CC::toLowerCase(ConnStr) == "close") {
					client_channel->keepAliceOut = true;
				}
				return;
			}
			String Path = ProcessPath(request.RequestData.Path);
			Path = CCFile::NormalizePath(Path);
			if(!source_map_){
				Path = removeMapSuffix(Path);
			}
			response.SetHeader("Content-type", Request::GetFileType(Path));
			response.SetHeader("Content-Length", CCInt(Request::GetFileSize(Path)).to_String());
			response.SetHeader("Pragma", "public");
			const auto cacheTime = CTL::String::format("public,max-age={}",this->CacheTime);
			response.SetHeader("Cache-Control", cacheTime);
			const auto Data = StaticResources.get(Path);
			if (Data) {
				const auto ZipStrFlag = request.RequestData.Headers.at("Accept-Encoding");
				if (ZipStrFlag.find("gzip") != CTL::String::npos &&
					GZipFlag && !Data->ContentGZip.empty()) {
					response.SetHeader("Content-Encoding","gzip");
					response.SetHeader("Content-Length",
						CC::to_String((unsigned long)Data->ContentGZip.size()));
					if (!response.Resource(Data->ContentGZip)) {

					}
				}
				else {
					if (!response.Resource(Data->Content)) {

					}
				}
			}
			else {
				const CCFile file(Path);
				if (!file.isExists()) {
					ErrorHandling(client_channel,request);
					return;
				}
				if (GZipFlag && file.GetFileSize() >= 1 * MB_Unit_t) {
					const auto D = CCFileInStream::ReadBinaryFile(Path);
					const auto GZipData = GZip::compress(D.data(),D.size());
					response.SetHeader("Content-Length",
						CC::to_String((unsigned long)GZipData.size()));
					if (!response.Resource(GZipData)) {

					}
				}
				else {
					if (!response.ResponseFile(Path,200)) {

					}
				}
			}
		}
	}
	else if (request.RequestData.Method == "OPTIONS") {
		client_channel->keepAliceOut = true;
		response.ResponseRouting();
	}
	else {
		// MessagePrint("不支持的请求方法: " + request.RequestData.Method);
		client_channel->keepAliceOut = true;
		const auto CC_S_Fun = RequestFun[request.RequestData.Path];
		if (CC_S_Fun) {
			CC_S_Fun->operator()(request,response);
		}
		else {
			if (WebSignalFun) {
				// 全局认证检查
				if (m_globalAuthProvider) {
					AuthResult authResult = m_globalAuthProvider->Authenticate(request);
					if (!authResult.Success) {
						m_globalAuthProvider->OnAuthFailed(response, authResult);
						return;
					}
				}
				WebSignalFun(request,response);
			}
		}
	}
}

CTL::String CTL::WebServlet::ProcessPath(const CTL::String& path) const { // Could not connect: Connection refused
	String Path = path;
	for (const auto& i : StaticPath) {
		String str = CCFile::NormalizePath(i + "/" + path);
		try {
			CCFile f(str);
			if (f.isExists()) {
				Path = i + "/" + path;
				break;
			}
		}
		catch (CCException& e) {
			MessagePrint("ProcessPath -> " + str + " -> " + CTL::String(e.what()));
			Path = i + "/" + path;
			break;
		}
	}
	return Path;
}

void CTL::WebServlet::InitStatic(int fileSize) {
	this->FileSize = FileSize;
	StaticResources.Clear();
	for (const auto& snd : StaticPath) {
		DirInitStatic(snd);
	}
}

void WebServlet::SetMaxConcurrentFileUploads(const int Num) {
	CTL::Request::SetMaxConcurrentFileUploads(Num);
}

void CTL::WebServlet::DirInitStatic(const CTL::String &Path) {
	const auto List = CCFile::GetDirectoryContents(Path);
	for (CCVar file : List) {
		try {
			if(!file.IsDirectory) {
				try {
					if (file.Size >= FileSize) {
						StaticResource D;
						D.Path = file.Path;
						const String P = CCFile::NormalizePath(D.Path);
						D.Content = CCFileInStream::ReadBinaryFile(D.Path);
						D.Name = file.Name;
						D.Type = Request::GetFileType(D.Name);
						if (GZipFlag) {
							D.ContentGZip = GZip::compress(D.Content.data(),D.Content.size());
						}
						StaticResources.put(P,D);
					}
				}
				catch (const std::exception& e) {
					MessagePrint("Cannot read file: " +
						file.Path + " -> " + CTL::String(e.what()));
					continue;
				}
			}
			else {
				DirInitStatic(file.Path);
			}
		}
		catch (CCException& exception) {
			MessagePrint(" DirInitStatic -> " +
				file.Path + " -> " + CTL::String(exception.what()));
			// 继续处理其他文件而不是中断整个过程
			continue;
		}
	}
}

bool CTL::WebServlet::hasMapSuffix(const std::string& path) {
	const CCFile file(path);
	if (!file.isExists()) {
		return false;
	}
	// 获取文件扩展名
	CC_FS::path p(path);
	std::string extension = p.extension().string();
	// 判断扩展名是否为 .map
	return extension == ".map";
}

std::string CTL::WebServlet::removeMapSuffix(const std::string& path) {
	if (hasMapSuffix(path)) {
		// 获取文件名（不包括目录）
		// 删除 .map 后缀
		return path.substr(0, path.size() - 4);
	}
	return path;
}

CTL::String CTL::WebServlet::GetApp_Form_Data(const HttpSocketChannel* client_channel, Request& HTTPRequests) {
	CTL::String st = HTTPRequests.RequestData.Headers["Content-Length"];
	if(st.empty()) {
		st = HTTPRequests.RequestData.Headers["content-length"];
	}
	CTL::String data;
	const auto contentLen = st.to_int();
	const auto client = client_channel->getClient();
	if (!client) {
		return SocketClientNULL;
	}

	constexpr size_t CHUNK_SIZE = 4096;
	char buffer[CHUNK_SIZE];
	size_t totalRead = 0;
	int ET = 0;

	while (true) {
		size_t toRead = CHUNK_SIZE;
		if (contentLen > 0) {
			const size_t remaining = static_cast<size_t>(contentLen) - totalRead;
			if (remaining == 0) break;
			if (remaining < toRead) toRead = remaining;
		}

		const auto length = client_channel->receive(buffer, toRead);
		if (length > 0) {
			ET = 0;
			data.append(buffer, static_cast<size_t>(length));
			totalRead += static_cast<size_t>(length);
			if (contentLen > 0 && totalRead >= static_cast<size_t>(contentLen)) {
				break;
			}
			if (totalRead >= MaxDataSize) {
				break;
			}
		}
		else if (length == -2) {
			if (contentLen > 0) {
				if (ET >= 1) {
					break;
				}
				ET++;
			} else {
				break;
			}
			Thread::Sleep(1);
		}
		else {
			break;
		}
	}
	return data;
}

CCMap<CTL::String, CTL::String> CTL::WebServlet::GetMultipart_Form_DataInfo(const HttpSocketChannel* client_channel, CTL::String& RecvMessage) {
	CCVector<char> data;
	const auto client = client_channel->getClient();
	if (!client) {
		return {};
	}
	int ET = 0;
	while (true) {
		char Buffer[CC_R_BUFFER_SIZE] = {0};
		const auto length = client_channel->receive(Buffer, CC_R_BUFFER_SIZE);
		if(length > 0) {
			ET = 0;
			data.insert(data.end(), Buffer, Buffer + length);

			if (data.size() > MaxDataSize) {
				break;
			}

			// 直接在 vector 尾部搜索 \r\n\r\n，避免每次全量 string 拷贝
			if (data.size() >= 4) {
				const size_t searchStart = data.size() > 256 ? data.size() - 256 : 0;
				for (size_t i = searchStart; i + 3 < data.size(); ++i) {
					if (data[i] == '\r' && data[i+1] == '\n' && data[i+2] == '\r' && data[i+3] == '\n') {
						RecvMessage = std::string(data.begin(), data.end());
						goto header_found;
					}
				}
			}
		}
		else if (length == -2) {
			if (ET >= 100) {
				break;
			}
			ET++;
			Thread::SleepMS(1);
		}
		else {
			break;
		}
	}
	header_found:
		CCMap<CTL::String, CTL::String> map;
		const CTL::String rawPrefix = RecvMessage;
		map["__RawMultipartPrefix__"] = rawPrefix;
		data.clear();
		RecvMessage = Request::GetFileDateHeader(RecvMessage, map);
		return map;
}

void WebServlet::ErrorHandling(HttpSocketChannel* client_channel, Request& request,int ErrorCode) const {
	const auto client = client_channel->getClient();
	if (!client) {
		return;
	}
	// const auto socket = client->getSocket();
	// if (!socket) {
	// 	return;
	// }
	Response response = {};
	response.Init(client_channel,this->CORS_Header);
	CTL::String html;
	if (ErrorCode == 404) {
		html = CTL::String::format("<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>404 Not Found</title></head><body><h1>"
					"404 Not Found</h1><p>The requested URL was not found on this server.</p></body></html>");
	}
	else if (ErrorCode == 405) {
		html = CTL::String::format("<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>405 Method Not Allowed</title></head><body><h1>"
					"405 Method Not Allowed</h1><p>The requested method is not allowed for this URL.</p></body></html>");
	}
	else if (ErrorCode == 500) {
		html = CTL::String::format("<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>500 Internal Server Error</title></head><body><h1>");
	}
	response.Write(html,ErrorCode);
}

String WebServlet::FormatRequest(const String &str) {
	// 匹配并移除 http://或https://开头的完整URL部分（支持IPv4/IPv6）
	std::regex pattern(R"((?:http|https)://\[[^\]]+\](?::\d+)?|(?:http|https)://[^/]+(?::\d+)?)");

	// 替换为相对路径
	std::string result = std::regex_replace(str.c_str(), pattern, "");

	// 处理可能残留的空格（根据实际需求决定是否保留）
	result = std::regex_replace(result, std::regex("^\\s+"), "");

	return result;
}

ByteArray WebServlet::GetRequestAll(const HttpSocketChannel *client_channel, Request &HTTPRequests) const {
	CTL::String st = HTTPRequests.RequestData.Headers["Content-Length"];
	if(st.empty()) {
		st = HTTPRequests.RequestData.Headers["content-length"];
	}
	ByteArray data;
	const auto contentLen = st.to_int();
	const auto client = client_channel->getClient();
	if (!client) {
		return SocketClientNULL;
	}

	constexpr size_t CHUNK_SIZE = 4096;
	char buffer[CHUNK_SIZE];
	size_t totalRead = 0;
	int ET = 0;

	while (true) {
		size_t toRead = CHUNK_SIZE;
		if (contentLen > 0) {
			const size_t remaining = static_cast<size_t>(contentLen) - totalRead;
			if (remaining == 0) break;
			if (remaining < toRead) toRead = remaining;
		}

		const auto length = client_channel->receive(buffer, toRead);
		if (length > 0) {
			ET = 0;
			data.append(CTL::ByteArray(buffer,length));
			totalRead += static_cast<size_t>(length);
			if (contentLen > 0 && totalRead >= static_cast<size_t>(contentLen)) {
				break;
			}
			if (totalRead >= MaxDataSize) {
				break;
			}
		}
		else if (length == -2) {
			if (contentLen > 0) {
				if (ET >= 1) {
					break;
				}
				ET++;
			} else {
				break;
			}
			Thread::Sleep(1);
		}
		else {
			break;
		}
	}
	return data;
}

void WebServlet::SetSourceMap(const bool flag){
	this->source_map_ = flag;
}

void WebServlet::SetCacheTime(const unsigned long CacheTime_t) {
	this->CacheTime = CacheTime_t;
}

void WebServlet::SetGZipFlag(const bool Flag) {
	this->GZipFlag = Flag;
}

void WebServlet::SetKeepAlice(const uint32_t KeepTimeOut_t, const uint32_t KeepMax_t) {
	this->KeepTimeOut = KeepTimeOut_t;
	this->KeepMax = KeepMax_t;
}

void WebServlet::SetMaxDataSize(const size_t Size) {
	MaxDataSize = Size;
}


