#include "CCRequest.h"
#include "CCSystem.h"
#include "CCCookie.h"

using namespace CTL;

CCMutex CTL::Request::File_Mutex_T;
std::atomic<uint8_t> CTL::Request::IsRun(0);
int CTL::Request::MAX_UPLOAD_FILE_CONCURRENT = 10;

const std::unordered_map<std::string, std::string> mime_types = {
    {"html", "text/html"},
    {"htm", "text/html"},
    {"css", "text/css"},
    {"js", "application/javascript"},
    {"png", "image/png"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"gif", "image/gif"},
    {"ico", "image/x-icon"},
    {"svg", "image/svg+xml"},
    {"txt", "text/plain"},
    {"xml", "text/xml"},
    {"json", "application/json"},
    {"pdf", "application/pdf"},
    {"zip", "application/zip"},
    {"001", "application/x-001"},
    {"301", "application/x-301"},
    {"323", "text/h323"},
    {"906", "application/x-906"},
    {"907", "drawing/907"},
    {"a11", "application/x-a11"},
    {"acp", "audio/x-mei-aac"},
    {"ai", "application/postscript"},
    {"aif", "audio/aiff"},
    {"aifc", "audio/aiff"},
    {"aiff", "audio/aiff"},
    {"anv", "application/x-anv"},
    {"asa", "text/asa"},
    {"asf", "video/x-ms-asf"},
    {"asx", "video/x-ms-asf"},
    {"asp", "text/asp"},
    {"au", "audio/basic"},
    {"avi", "video/avi"},
    {"awf", "application/vnd.adobe.workflow"},
    {"biz", "text/xml"},
    {"bmp", "application/x-bmp"},
    {"bot", "application/x-bot"},
    {"c4t", "application/x-c4t"},
    {"c90", "application/x-c90"},
    {"cal", "application/x-cals"},
    {"cat", "application/vnd.ms-pki.seccat"},
    {"cdf", "application/x-netcdf"},
    {"cdr", "application/x-cdr"},
    {"cel", "application/x-cel"},
    {"cer", "application/x-x509-ca-cert"},
    {"cg4", "application/x-g4"},
    {"cgm", "application/x-cgm"},
    {"cit", "application/x-cit"},
    {"class", "java/*"},
    {"cml", "text/xml"},
    {"cmp", "application/x-cmp"},
    {"cmx", "application/x-cmx"},
    {"cot", "application/x-cot"},
    {"crl", "application/pkix-crl"},
    {"crt", "application/x-x509-ca-cert"},
    {"csi", "application/x-csi"},
    {"cut", "application/x-cut"},
    {"dbf", "application/x-dbf"},
    {"dbm", "application/x-dbm"},
    {"dbx", "application/x-dbx"},
    {"dcd", "text/xml"},
    {"dcx", "application/x-dcx"},
    {"der", "application/x-x509-ca-cert"},
    {"dgn", "application/x-dgn"},
    {"dib", "application/x-dib"},
    {"dll", "application/x-msdownload"},
    {"doc", "application/msword"},
    {"dot", "application/msword"},
    {"drw", "application/x-drw"},
    {"dtd", "text/xml"},
    {"dwf", "application/x-dwg"},
    {"dwg", "application/x-dwg"},
    {"dxb", "application/x-dwg"},
    {"dxf", "application/x-dwg"},
    {"edn", "application/vnd.adobe.edn"},
    {"emf", "application/x-emf"},
    {"eml", "message/rfc822"},
    {"ent", "text/xml"},
    {"epi", "application/x-epi"},
    {"eps", "application/x-ps"},
    {"etd", "application/x-ebx"},
    {"exe", "application/x-msdownload"},
    {"fax", "image/fax"},
    {"fdf", "application/vnd.fdf"},
    {"fif", "application/fractals"},
    {"fo", "text/xml"},
    {"frm", "application/x-frm"},
    {"g4", "application/x-g4"},
    {"ig4", "application/x-g4"},
    {"gbr", "application/x-gbr"},
    {"gl2", "application/x-gl2"},
    {"gp4", "application/x-gp4"},
    {"hgl", "application/x-hgl"},
    {"hmr", "application/x-hmr"},
    {"hpg", "application/x-hpgl"},
    {"hpl", "application/x-hpl"},
    {"hqx", "application/mac-binhex40"},
    {"hrf", "application/x-hrf"},
    {"hta", "application/hta"},
    {"htc", "text/x-component"},
    {"htt", "text/webviewhtml"},
    {"htx", "text/html"},
    {"icb", "image/x-icon"},
    {"iff", "application/x-iff"},
    {"igs", "application/x-igs"},
    {"iii", "application/x-iphone"},
    {"img", "application/x-img"},
    {"ins", "application/x-internet-signup"},
    {"isp", "application/x-internet-signup"},
    {"IVF", "video/x-ivf"},
    {"java", "java/*"},
    {"jfif", "image/jpeg"},
    {"jpe", "image/jpeg"},
    {"jsp", "text/html"},
    {"la1", "audio/x-liquid-file"},
    {"lar", "application/x-laplayer-reg"},
    {"latex", "application/x-latex"},
    {"lavs", "audio/x-liquid-secure"},
    {"lbm", "application/x-lbm"},
    {"lmsff", "audio/x-la-lms"},
    {"ls", "application/x-javascript"},
    {"mocha", "application/x-javascript"},
    {"ltr", "application/x-ltr"},
    {"m1v", "video/x-mpeg"},
    {"m2v", "video/x-mpeg"},
    {"mp2v", "video/x-mpeg"},
    {"mps", "video/x-mpeg"},
    {"mpv", "video/x-mpeg"},
    {"mpx", "video/x-mpeg"},
    {"m3u", "audio/mpegurl"},
    {"m4e", "video/mpeg4"},
    {"mac", "application/x-mac"},
    {"man", "application/x-troff-man"},
    {"math", "text/xml"},
    {"mdb", "application/msaccess"},
    {"mfp", "application/x-shockwave-flash"},
    {"mht", "message/rfc822"},
    {"mhtml", "message/rfc822"},
    {"mi", "application/x-mi"},
    {"mid", "audio/mid"},
    {"midi", "audio/mid"},
    {"mil", "application/x-mil"},
    {"mml", "text/xml"},
    {"mnd", "audio/x-musicnet-download"},
    {"mns", "audio/x-musicnet-stream"},
    {"movie", "video/x-sgi-movie"},
    {"mp1", "audio/mp1"},
    {"mp2", "audio/mp2"},
    {"mp3", "audio/mp3"},
    {"mp4", "video/mpeg4"},
    {"mpg4", "video/mpeg4"},
    {"mpa", "video/mpeg"},
    {"mpg", "video/mpeg"},
    {"mpeg", "video/mpeg"},
    {"mpga", "audio/rn-mpeg"},
    {"mpp", "application/vnd.ms-project"},
    {"mpd", "application/vnd.ms-project"},
    {"mpt", "application/vnd.ms-project"},
    {"mpw", "application/vnd.ms-project"},
    {"mtx", "text/xml"},
    {"mxp", "application/x-mmxp"},
    {"net", "image/pnetvue"},
    {"nrf", "application/x-nrf"},
    {"nws", "message/rfc822"},
    {"odc", "text/x-ms-odc"},
    {"out", "application/x-out"},
    {"p10", "application/pkcs10"},
    {"p12", "application/x-pkcs12"},
    {"pfx", "application/x-pkcs12"},
    {"p7b", "application/x-pkcs7-certificates"},
    {"p7c", "application/pkcs7-mime"},
    {"p7m", "application/pkcs7-signature"},
    {"p7s", "application/pkcs7-signature"},
    {"pc5", "application/x-pc5"},
    {"pci", "application/x-pci"},
    {"pcl", "application/x-pcl"},
    {"pcx", "application/x-pcx"},
    {"pdx", "application/vnd.adobe.pdx"},
    {"pgl", "application/x-pgl"},
    {"pic", "application/x-pic"},
    {"pko", "application/vnd.ms-pki.pko"},
    {"pl", "application/x-perl"},
    {"plx", "application/x-perl"},
    {"plg", "text/html"},
    {"pls", "audio/scpls"},
    {"pld", "audio/scpls"},
    {"plt", "application/x-ps"},
    {"prn", "application/x-ps"},
    {"prt", "application/x-ps"},
    {"ps", "application/x-ps"},
    {"pub", "application/x-ps"},
    {"ptn", "application/x-ptn"},
    {"pwz", "application/vnd.ms-powerpoint"},
    {"ra", "audio/x-pn-realaudio"},
    {"ram", "audio/x-pn-realaudio"},
    {"ras", "application/x-ras"},
    {"rat", "application/rat-file"},
    {"rdf", "text/xml"},
    {"rss", "text/xml"},
    {"red", "application/x-red"},
    {"rgb", "application/x-rgb"},
    {"rjs", "application/vnd.rn-realmedia"},
    {"rjt", "application/vnd.rn-realmedia"},
    {"rlc", "application/vnd.rn-realmedia"},
    {"rle", "application/vnd.rn-realmedia"},
    {"rm", "application/vnd.rn-realmedia"},
    {"rmf", "application/vnd.rn-realmedia"},
    {"rmi", "application/vnd.rn-realmedia"},
    {"rmj", "application/vnd.rn-realmedia"},
    {"rmmp", "application/vnd.rn-realmedia"},
    {"rmm", "application/vnd.rn-realmedia"},
    {"rmp", "application/vnd.rn-realmedia"},
    {"rms", "application/vnd.rn-realmedia"},
    {"rmvb", "application/vnd.rn-realmedia"},
    {"rmx", "application/vnd.rn-realmedia"},
    {"rnx", "application/vnd.rn-realmedia"},
    {"rp", "application/vnd.rn-realmedia"},
    {"rpm", "application/vnd.rn-realmedia"},
    {"rsml", "application/vnd.rn-realmedia"},
    {"rt", "application/vnd.rn-realmedia"},
    {"rtf", "application/vnd.rn-realmedia"},
    {"rv", "application/vnd.rn-realmedia"},
    {"sam", "application/x-sam"},
    {"sat", "application/x-sat"},
    {"sdp", "application/sdp"},
    {"sdw", "application/x-sdw"},
    {"sit", "application/x-stuffit"},
    {"sea", "application/x-stuffit"},
    {"slb", "application/x-sld"},
    {"sld", "application/x-sld"},
    {"slk", "drawing/x-slk"},
    {"smi", "application/smil"},
    {"smil", "application/smil"},
    {"smk", "application/x-smk"},
    {"snd", "audio/basic"}
};

CTL::String CTL::Request::GetFileType(const CTL::String &str) {
    const CTL::String suffix = GetFileSuffix(str);
    auto it = mime_types.find(suffix);
    if (it != mime_types.end()) {
        return it->second;
    }
    return suffix; // 默认返回扩展名
}

CTL::String CTL::Request::GetParameter(const CTL::String& key,const bool Body) {
	CTL::String str = "";
    if (Body) {
    	if (RequestData.Params.exist(key)) {
    		str = RequestData.Params[key];
    	}
    	else {
    		return str;
    	}
    }
    else {
        if (RequestData.Method == "GET") {
        	if (RequestData.Params.exist(key)) {
        		str = RequestData.Params[key];
        	}
        	else {
        		return str;
        	}
        }
        else if (RequestData.Method == "POST") {
            CTL::String f = CTL::String(key + "=");
            auto len = RequestData.Body.toString().length();
            int pos = 0,A = f.length();
            for (int i = 0; i < len; ++i) {
                CTL::String B = RequestData.Body.toString().substr(i,A);
                if(f == B){
                    pos = i + A;
                    break;
                }
            }
            for (int i = pos; i < len; ++i) {
                if(RequestData.Body[i].get() != (uint8_t)'&'){
                    str += RequestData.Body[i];
                }
                else{
                    break;
                }
            }
        }
    }
	return URL_Decode(str);
}

CTL::String CTL::Request::GetFileInfo(const CTL::String& key) {
	return URL_Decode(RequestData.FileHeaders[key]);
}

CCVector<MultipartPart>& CTL::Request::GetMultipartParts() {
	return RequestData.MultipartParts;
}

MultipartPart* CTL::Request::GetMultipartPart(const CTL::String& name) {
	for (auto& part : RequestData.MultipartParts) {
		if (part.Name == name) {
			return &part;
		}
	}
	return nullptr;
}

CCVector<MultipartPart*> CTL::Request::GetMultipartParts(const CTL::String& name) {
	CCVector<MultipartPart*> result;
	for (auto& part : RequestData.MultipartParts) {
		if (part.Name == name) {
			result.push_back(&part);
		}
	}
	return result;
}

size_t CTL::Request::GetMultipartPartCount() const {
	return RequestData.MultipartParts.size();
}

CTL::JSONObject CTL::Request::GetJson() const {
	const CTL::String str = RequestData.Body.toString();
	CTL::String trimmedStr = str;
	if (!trimmedStr.empty() && trimmedStr.back() == ':') {
		trimmedStr.pop_back();
	}
	return CTL::JSONObject::parse(trimmedStr);
}

bool CTL::Request::GetMediaFiles(const CTL::String& Path) {
    auto Folder = CCFile::GetParentDir(Path);
    auto tmp = CTL::generateUnique16CharString() + ".tmp";
    auto FilePath = CCFile::NormalizePath(Folder + "/" + tmp);
    bool F = GetMediaFiles([this,FilePath](const CCVector<char>& Buffer, size_t Size) {
        try {
            FileWriting(FilePath, Buffer, Size);
            return true;
        }
        catch (CCException& e) {
            auto str = CTL::String({"Error GetMediaFiles -> ",e.what()});
            CC::Println(str);
            return false;
        }
    });
    if (F) {
        CCFile file(Path);
        if (file.isExists()) {
            auto DL = file.Delete();
        }
        F = CCFile::ReviseFileName(FilePath, Path);
        if (!F) {
            CCFile file_tmp(FilePath);
            if (file_tmp.isExists()) {
                auto DL = file_tmp.Delete();
            }
        }
    }
	return F;
}

bool Request::GetMediaFiles(const ReceiveFileCallback& callback) {
	if (!callback) {
		return false;
	}
	const String str = "\r\n" + RequestData.FileHeaders["Boundary"] + "--\r\n";
	auto S = RequestData.Headers["Content-Length"];
	String FileName = "xxxx";
	try {
		FileName = RequestData.FileHeaders["filename"];
	}
	catch (...){}
    if(S.empty()){
        S = RequestData.Headers["content-length"];
    }
    long long Maxlength = std::stoll(S); // 1659046
    size_t Length = 0;
    const size_t MLength = Maxlength;
    size_t BoundarySize = str.size(); // 46
    CCVector<char> data;
    int F = 0;
    if (!this->RequestData.Buffer.empty()) {
        data = this->RequestData.Buffer;
    	Length += data.size();
    	Maxlength -= data.size();
    }
    int Timeout = 0;
	data.reserve(File_Buffer_Max * 2);
	char buffer[File_Buffer_Max] = { 0 };
    while (true){
    	const auto l = this->channel->receive(buffer, File_Buffer_Max);
    	bool CF = false;
    	if(l > 0){
    		Timeout = 0;
    		Length += l;
    		try {
    			data.insert(data.end(), buffer, buffer + l);
    			Maxlength = Maxlength - l;
    			if (!FileName.empty()) {
    				if(Maxlength < File_Buffer_Max * 2){
    					String DB(data.data(), data.size());
    					const auto len = DB.find(str);
    					if(len != String::npos){
    						data.resize(len);
    						F = 1;
    					}
    					else {
    						continue;
    					}
    				}
    			}
    			else {
    				if (Maxlength <= 0) {
    					F = 1;
    				}
    			}
    		}
    		catch (CCException& e) {
    			System::Println("GetMediaFiles Error {}",e.what());
    			return false;
    		}
    		try {
    			CF = callback(data, data.size());
    		}
    		catch (CCException& e) {
    			System::Println("GetMediaFiles callback Error: {}",e.what());
    		}
    		if (!CF && F == 0) {
    			break;
    		}
    		if (F == 1) {
    			break;
    		}
    		data.clear();
    	}
    	else if (l == -2){
    		if (Timeout >= 1000) {
    			System::Println("GetMediaFiles TimeOut FileName: {}",FileName.c_str());
    			break;
    		}
    		Timeout++;
    		Thread::SleepMS(10);
    	}
    	else if (l == -5) {
    		System::Println("GetMediaFiles Error -5");
    		break;
    	}
    	else if (l == -1) {
    		System::Println("GetMediaFiles Error -1");
    		break;
    	}
    	else {
    		System::Println("GetMediaFiles Error {}",l);
    		break;
    	}
    }
    return F == 1;
}

bool Request::ProcessMultipartStream(const MultipartFileCallback &fileCallback,
	const MultipartFieldCallback &fieldCallback, size_t readBufSize)
{
	if (!channel || !fileCallback) {
		return false;
	}
	// 获取 boundary（FileHeaders["Boundary"] 存储的是 "--xxxxxx" 格式）
	String boundaryRaw = RequestData.FileHeaders["Boundary"];
	if (boundaryRaw.empty()) {
		return false;
	}
	// 去掉前缀 "--"
	String boundary = boundaryRaw;
	if (boundary.find("--") == 0) {
		boundary = boundary.substr(2);
	}
	if (boundary.empty()) {
		return false;
	}

	const String boundaryDelim = "\r\n--" + boundary;
	const String endDelim = boundaryDelim + "--";
	const size_t boundaryDelimLen = boundaryDelim.size();

	// 首个 part 的元信息从 FileHeaders 中获取
	struct CurrentPart {
		String name;
		String filename;
		String contentType;
		bool isFile = false;
	} current;
	current.name = RequestData.FileHeaders["name"];
	current.filename = RequestData.FileHeaders["filename"];
	current.contentType = RequestData.FileHeaders.exist("Content-Type")
		? RequestData.FileHeaders["Content-Type"] : "";
	current.isFile = !current.filename.empty();

	// 首次进入时在头部之后，可能已有少量 body 数据在 Buffer 中（从头部读取阶段泄露的）
	String accumulator;
	if (!RequestData.Buffer.empty()) {
		accumulator.assign(RequestData.Buffer.begin(), RequestData.Buffer.end());
	}
	RequestData.Buffer.clear();

	const size_t bufSize = readBufSize > 0 ? readBufSize : 4096;
	auto readBuf = std::make_unique<char[]>(bufSize);

	bool readingHeaders = false; // 当前是读 body 还是读下一 part 头部
	int timeout = 0;

	while (true) {
		// 需要更多数据时从 socket 读取
		const auto len = channel->receive(readBuf.get(), bufSize);
		if (len > 0) {
			timeout = 0;
			accumulator.append(readBuf.get(), len);
		}
		else if (len == -2) {
			if (timeout >= 1000) {
				return false;
			}
			timeout++;
			Thread::SleepMS(10);
			continue;
		}
		else {
			return false;
		}

		// 处理 accumulator 中所有可识别的段
		while (!accumulator.empty()) {
			if (readingHeaders) {
				// ===== 读取下一 part 的头部 =====
				const size_t hdrEnd = accumulator.find("\r\n\r\n");
				if (hdrEnd == String::npos) {
					// 头部不完整，需要更多数据
					break;
				}
				const String headerStr = accumulator.substr(0, hdrEnd);
				accumulator.erase(0, hdrEnd + 4); // 跳过 \r\n\r\n

				current.isFile = ParsePartHeaders(headerStr,
					current.name, current.filename, current.contentType);
				readingHeaders = false;
				// 继续循环，检查 accumulator 中是否已有该 part 的 body 数据
				continue;
			}

			// ===== 读取当前 part 的 body 数据 =====
			// 在 accumulator 中查找下一个 boundary 或结束标记
			size_t delimPos = accumulator.find(boundaryDelim);
			size_t endPos = accumulator.find(endDelim);

			// 两个都没找到 → 安全输出大部分数据，保留尾部以防 boundary 跨块
			if (delimPos == String::npos && endPos == String::npos) {
				const size_t safeLen = accumulator.size() > boundaryDelimLen
					? accumulator.size() - boundaryDelimLen : 0;
				if (safeLen > 0) {
					const String chunkStr = accumulator.substr(0, safeLen);
					CCVector<char> chunk(chunkStr.begin(), chunkStr.end());
					if (current.isFile) {
						if (!fileCallback(current.name, current.filename,
							current.contentType, chunk, chunk.size(), false)) {
							return false;
						}
					}
					// 表单字段累积但不立即回调（等 boundary 出现才知道完整值）
					accumulator.erase(0, safeLen);
				}
				break; // 需要更多数据
			}

			// 找到了 boundary
			bool isEnd = false;
			size_t splitPos;
			if (endPos != String::npos && (delimPos == String::npos || endPos <= delimPos)) {
				splitPos = endPos;
				isEnd = true;
			}
			else {
				splitPos = delimPos;
			}

			// 提取 boundary 之前的 body 数据
			String bodyData = accumulator.substr(0, splitPos);

			// 去除尾部 \r\n
			while (bodyData.size() >= 2 &&
				bodyData.substr(bodyData.size() - 2) == "\r\n") {
				bodyData.erase(bodyData.size() - 2);
			}

			// 回调当前 part 的数据
			if (current.isFile) {
				CCVector<char> chunk(bodyData.begin(), bodyData.end());
				if (!fileCallback(current.name, current.filename,
					current.contentType, chunk, chunk.size(), true)) {
					return false;
				}
			}
			else if (fieldCallback) {
				fieldCallback(current.name, bodyData);
			}

			// 跳过 boundary 行
			const size_t skipLen = isEnd ? endDelim.size() : boundaryDelim.size();
			accumulator.erase(0, splitPos + skipLen);

			if (isEnd) {
				return true; // 成功处理完所有 part
			}

			// 下一个 part 从头部开始
			readingHeaders = true;
		}
	}
	return false; // 异常退出（超时或连接断开）
}

bool Request::ReadMultipartBodyToMemory() {
	if (!channel) {
		return false;
	}

	// 获取 boundary
	String boundaryRaw = RequestData.FileHeaders["Boundary"];
	if (boundaryRaw.empty()) {
		return false;
	}
	String boundary = boundaryRaw;
	if (boundary.find("--") == 0) {
		boundary = boundary.substr(2);
	}
	if (boundary.empty()) {
		return false;
	}

	const String endMarker = "\r\n--" + boundary + "--\r\n";

	// 从 RawMultipartPrefix（完整原始头部）开始拼接
	String bodyData = RequestData.RawMultipartPrefix;

	// 继续从 socket 读取直到结束标记
	const size_t bufSize = 4096;
	auto readBuf = std::make_unique<char[]>(bufSize);
	int timeout = 0;

	while (true) {
		// 检查是否已包含结束标记
		if (bodyData.find(endMarker) != String::npos) {
			break;
		}

		const auto len = channel->receive(readBuf.get(), bufSize);
		if (len > 0) {
			timeout = 0;
			bodyData.append(readBuf.get(), len);
		}
		else if (len == -2) {
			if (timeout >= 1000) {
				System::Println("ReadMultipartBodyToMemory TimeOut");
				return false;
			}
			timeout++;
			Thread::SleepMS(10);
		}
		else {
			System::Println("ReadMultipartBodyToMemory Error {}", len);
			return false;
		}
	}

	// 解析完整 body 到 MultipartParts
	RequestData.MultipartParts = ParseMultipartFormData(bodyData, boundary);
	return !RequestData.MultipartParts.empty();
}

void CTL::Request::Init(HttpSocketChannel* channel, CORS* cors) {
	this->channel = channel;
	this->cors = cors;

}

CTL::String CTL::Request::GetFileSuffix(const CTL::String &filePath) {
	size_t dotPosition = filePath.rfind('.'); // 从右向左查找最后一个'.'的位置
	if (dotPosition == std::string::npos){
		return ""; // 如果没有找到'.'，则返回空字符串
	}
	return filePath.substr(dotPosition + 1); // 返回'.'之后的部分，即文件后缀名
}

unsigned int CTL::Request::GetFileSize(const CTL::String& path) {
	const CCFile file(path);
	if (!file.isExists()) {
		return 0;
	}
	return file.GetFileSize();
}

CTL::String CTL::Request::URL_Decode(const CTL::String& in) {
	CTL::String out;
	for (size_t i = 0; i < in.length(); ++i) {
		if (in[i] == '%') {
			if (i + 3 <= in.length()) {
				std::string byte_str = in.substr(i + 1, 2);
				char byte = static_cast<char>(std::strtol(byte_str.c_str(), nullptr, 16));
				out += byte;
				i += 2;
			}
		} else if (in[i] == '+') {
			out += ' ';
		} else {
			out += in[i];
		}
	}
	return out;
}

String Request::GetFileDateHeader(String& in, std::map<String, String>& map) {
	String str= in,A,B;
	auto len = str.find("\r\n");
	while (true){
		String Boundary = str.substr(0,len);
		if(Boundary.empty()){
			str.erase(0,len+2);
		}
		else {
			str.erase(0,len+2);
			map["Boundary"] = Boundary;
			break;
		}
		len = str.find("\r\n");
	}
	std::stringstream ss(str);
	ss >> A;
	ss >> B;
	len = A.length() + B.length();
	A.erase(A.length() - 1,A.length());
	B.erase(B.length() - 1,B.length());
	map[A] = B;
	str.erase(0,len + 2);
	GetFormData(str,A,B);
	map[A] = B;
	len = A.length() + B.length();
	str.erase(0,len + 5);
	len = str.find("\r\n\r\n");
	CTL::String str2 = str.substr(0,len + 4);
	str.erase(0,len + 4);
	const auto m = parseKeyValuePairs(str2);
	for (const auto& i : m) {
		map[i.first] = i.second;
	}
	return str;
}

CCVector<MultipartPart> Request::ParseMultipartFormData(const CTL::String &body, const CTL::String &boundary) {
	CCVector<MultipartPart> parts;
	if (body.empty() || boundary.empty()) {
		return parts;
	}

	const String boundaryMarker = "--" + boundary;
	const String endMarker = boundaryMarker + "--";
	const String delimiter = "\r\n" + boundaryMarker;

	// 找到 body 中第一个 boundary 之后的位置
	size_t pos = 0;
	size_t firstBoundary = body.find(boundaryMarker);
	if (firstBoundary == String::npos) {
		return parts;
	}
	// 跳过首行 boundary（可能后面有 \r\n）
	pos = body.find("\r\n", firstBoundary);
	if (pos == String::npos) {
		return parts;
	}
	pos += 2; // 跳过 \r\n

	while (pos < body.size()) {
		// 检查是否到达结束标记
		if (body.substr(pos, endMarker.size()) == endMarker) {
			break;
		}

		// 查找下一个 boundary 位置
		size_t nextDelim = body.find(delimiter, pos);
		size_t nextEnd = body.find(endMarker, pos);
		size_t nextBoundary = String::npos;
		bool isEnd = false;

		if (nextDelim != String::npos && nextEnd != String::npos) {
			if (nextDelim < nextEnd) {
				nextBoundary = nextDelim;
			} else {
				nextBoundary = nextEnd;
				isEnd = true;
			}
		} else if (nextDelim != String::npos) {
			nextBoundary = nextDelim;
		} else if (nextEnd != String::npos) {
			nextBoundary = nextEnd;
			isEnd = true;
		} else {
			break;
		}

		// 提取当前 part 的完整数据
		String partData = body.substr(pos, nextBoundary - pos);

		// 去掉尾部 \r\n
		while (partData.size() >= 2 &&
			partData.substr(partData.size() - 2) == "\r\n") {
			partData.erase(partData.size() - 2);
		}

		// 解析头部和主体：分割点为 \r\n\r\n
		const size_t headerEnd = partData.find("\r\n\r\n");
		if (headerEnd != String::npos) {
			MultipartPart part;

			const String headerStr = partData.substr(0, headerEnd);
			const String bodyStr = partData.substr(headerEnd + 4);

			// 逐行解析头部
			size_t lineStart = 0;
			while (lineStart < headerStr.size()) {
				size_t lineEnd = headerStr.find("\r\n", lineStart);
				if (lineEnd == String::npos) lineEnd = headerStr.size();
				const String line = headerStr.substr(lineStart, lineEnd - lineStart);

				// 解析 Content-Disposition
				if (line.find("Content-Disposition:") == 0 || line.find("content-disposition:") == 0) {
					// 提取 name
					size_t namePos = line.find("name=\"");
					if (namePos != String::npos) {
						namePos += 6;
						size_t nameEnd = line.find("\"", namePos);
						if (nameEnd != String::npos) {
							part.Name = line.substr(namePos, nameEnd - namePos);
						}
					}
					// 提取 filename
					size_t fnPos = line.find("filename=\"");
					if (fnPos != String::npos) {
						fnPos += 10;
						size_t fnEnd = line.find("\"", fnPos);
						if (fnEnd != String::npos) {
							part.FileName = line.substr(fnPos, fnEnd - fnPos);
							part.IsFile = true;
						}
					}
				}
				// 解析 Content-Type
				else if (line.find("Content-Type:") == 0 || line.find("content-type:") == 0) {
					size_t colonPos = line.find(':');
					if (colonPos != String::npos) {
						part.ContentType = line.substr(colonPos + 1);
						// trim spaces
						while (!part.ContentType.empty() && part.ContentType[0] == ' ') {
							part.ContentType.erase(0, 1);
						}
					}
				}

				// 存储所有头部信息
				size_t colonPos = line.find(':');
				if (colonPos != String::npos) {
					String key = line.substr(0, colonPos);
					String value = line.substr(colonPos + 1);
					while (!value.empty() && value[0] == ' ') {
						value.erase(0, 1);
					}
					part.Headers[key] = value;
				}

				lineStart = lineEnd;
				if (lineEnd < headerStr.size()) lineStart += 2;
			}

			// 设置 body
			part.Body = CCVector<char>(bodyStr.begin(), bodyStr.end());
			parts.push_back(part);
		}

		// 移动到下一个 part 的起始位置
		if (isEnd) {
			break;
		}
		pos = body.find("\r\n", nextBoundary);
		if (pos == String::npos) {
			break;
		}
		pos += 2;
	}

	return parts;
}

void CTL::Request::GetFormData(CTL::String& input, CTL::String& key, CTL::String& value) {
	// 寻找等号的位置
	size_t equalPos = input.find('=');
	if (equalPos != std::string::npos) {
		// 提取键（等号前的部分）
		key = input.substr(0, equalPos);
		key.erase(0, key.find_first_not_of(' ')); // 去除前导空格
		key.erase(key.find_last_not_of(' ') + 1); // 去除尾随空格

		// 寻找第一个双引号的位置
		size_t startPos = input.find('"', equalPos);
		if (startPos != std::string::npos) {
			// 寻找最后一个双引号的位置
			size_t endPos = input.find('"', startPos + 1);
			if (endPos != std::string::npos) {
				// 提取值（两个双引号之间的部分）
				value = input.substr(startPos + 1, endPos - startPos - 1);
			}
		}
	}
}

CCMap<CTL::String, CTL::String> CTL::Request::parseKeyValuePairs(const CTL::String& input) {
	CCMap<CTL::String, CTL::String> keyValuePairs{};
	size_t start = 0;
	size_t end = 0;
	while ((end = input.find('\r', start)) != std::string::npos) {
		std::string line = input.substr(start, end - start);
		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos) {
			// 处理键值对
			size_t eqPos = line.find('=');
			if (eqPos != std::string::npos) {
				std::string key = line.substr(0, eqPos);
				const size_t quoteStart = line.find('"', eqPos);
				const size_t quoteEnd = line.find('"', quoteStart + 1);
				if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
					String value = line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
					keyValuePairs[key] = value;
				}
			}
		}
		else {
			// 处理以冒号分隔的键值对
			std::string key = line.substr(0, colonPos);
			std::string value = line.substr(colonPos + 2); // 跳过冒号和空格
			keyValuePairs[key] = value;
		}
		start = end + 2; // 跳过 "\r\n"
	}
	return keyValuePairs;
}

void Request::SetMaxConcurrentFileUploads(int Num) {
    MAX_UPLOAD_FILE_CONCURRENT = Num;
}

void Request::SetFileUploadSize(int Size) {
    IsFileUploadSize = false;
    // File_Buffer_Max = Size;
}

void Request::SetKeepAlive(const bool Flag) {
	this->keep_alive = Flag;
}

bool Request::isKeepAlive() const {
	return this->keep_alive;
}

void Request::FileWriting(const CTL::String &path, const CCVector<char> &buffer, size_t size) {
    CCFileOutStream file(path, std::ios::out | std::ios::binary | std::ios::app);
    if (file.is_open()) {
        file.write(buffer.data(),size);
        file.close();
    }
}

String Request::getFileName(const String& contentDisposition){
	const auto start = contentDisposition.find("''");
	const auto end = contentDisposition.find("\r\n");
	const auto str = contentDisposition.substr(start + 2,(end - start) - 2);
	return str;
}

bool Request::ParsePartHeaders(const CTL::String &headerStr, CTL::String &outName, CTL::String &outFilename,
	CTL::String &outContentType)
{
	outName.clear();
	outFilename.clear();
	outContentType.clear();
	bool isFile = false;

	size_t lineStart = 0;
	while (lineStart < headerStr.size()) {
		size_t lineEnd = headerStr.find("\r\n", lineStart);
		if (lineEnd == String::npos) lineEnd = headerStr.size();
		const String line = headerStr.substr(lineStart, lineEnd - lineStart);

		// Content-Disposition
		if (line.find("Content-Disposition:") == 0 ||
			line.find("content-disposition:") == 0)
		{
			size_t namePos = line.find("name=\"");
			if (namePos != String::npos) {
				namePos += 6;
				const size_t nameEnd = line.find("\"", namePos);
				if (nameEnd != String::npos) {
					outName = line.substr(namePos, nameEnd - namePos);
				}
			}
			size_t fnPos = line.find("filename=\"");
			if (fnPos != String::npos) {
				fnPos += 10;
				const size_t fnEnd = line.find("\"", fnPos);
				if (fnEnd != String::npos) {
					outFilename = line.substr(fnPos, fnEnd - fnPos);
					isFile = true;
				}
			}
		}
		// Content-Type
		else if (line.find("Content-Type:") == 0 ||
			line.find("content-type:") == 0)
		{
			size_t colonPos = line.find(':');
			if (colonPos != String::npos) {
				outContentType = line.substr(colonPos + 1);
				while (!outContentType.empty() && outContentType[0] == ' ') {
					outContentType.erase(0, 1);
				}
			}
		}

		lineStart = lineEnd;
		if (lineEnd < headerStr.size()) lineStart += 2;
	}
	return isFile;
}

// ==================== Cookie 相关实现 ====================

void Request::ParseCookies() {
	const CTL::String cookieKey1("Cookie");
	const CTL::String cookieKey2("cookie");
	CTL::String cookieHeader;
	try {
		if (RequestData.Headers.find(cookieKey1) != RequestData.Headers.end()) {
			cookieHeader = RequestData.Headers.at(cookieKey1);
		} else if (RequestData.Headers.find(cookieKey2) != RequestData.Headers.end()) {
			cookieHeader = RequestData.Headers.at(cookieKey2);
		}
	} catch (...) {}

	if (!cookieHeader.empty()) {
		RequestData.Cookies = CookieParser::ParseRequestCookies(cookieHeader);
	}
}

CTL::String Request::GetCookie(const CTL::String &name) {
	if (RequestData.Cookies.empty()) {
		ParseCookies();
	}
	const auto it = RequestData.Cookies.find(name);
	if (it != RequestData.Cookies.end()) {
		return it->second;
	}
	return {};
}

CCMap<CTL::String, CTL::String>& Request::GetCookies() {
	if (RequestData.Cookies.empty()) {
		ParseCookies();
	}
	return RequestData.Cookies;
}

const CCMap<CTL::String, CTL::String>& Request::GetCookies() const {
	return RequestData.Cookies;
}

bool Request::HasCookie(const CTL::String &name) const {
	return RequestData.Cookies.find(name) != RequestData.Cookies.end();
}

// ==================== Auth 相关实现 ====================

CTL::String Request::GetAuthorizationHeader() const {
	const CTL::String authKey1("Authorization");
	const CTL::String authKey2("authorization");
	try {
		if (RequestData.Headers.find(authKey1) != RequestData.Headers.end()) {
			return RequestData.Headers.at(authKey1);
		}
		if (RequestData.Headers.find(authKey2) != RequestData.Headers.end()) {
			return RequestData.Headers.at(authKey2);
		}
	} catch (...) {}
	return {};
}

CTL::String Request::GetBearerToken() const {
	const CTL::String auth = GetAuthorizationHeader();
	if (auth.find("Bearer ") == 0 || auth.find("bearer ") == 0) {
		return auth.substr(7); // "Bearer " 是 7 个字符
	}
	return {};
}

bool Request::GetBasicAuthCredentials(CTL::String& outUsername, CTL::String& outPassword) const {
	outUsername.clear();
	outPassword.clear();

	const CTL::String auth = GetAuthorizationHeader();
	if (auth.find("Basic ") != 0 && auth.find("basic ") != 0) {
		return false;
	}

	const CTL::String encoded = auth.substr(6); // "Basic " 是 6 个字符

	// 使用 OpenSSL 的 Base64 解码
	// 先计算解码后的大小
	const int decodedLen = ((encoded.size() + 3) / 4) * 3;
	CCVector<unsigned char> decoded(decodedLen + 1);

	int actualLen = 0;
	// 使用 BIO 进行 base64 解码
	BIO* b64 = BIO_new(BIO_f_base64());
	BIO* bmem = BIO_new_mem_buf(encoded.c_str(), static_cast<int>(encoded.size()));
	bmem = BIO_push(b64, bmem);
	BIO_set_flags(bmem, BIO_FLAGS_BASE64_NO_NL);
	actualLen = BIO_read(bmem, decoded.data(), decodedLen);
	BIO_free_all(bmem);

	if (actualLen <= 0) {
		return false;
	}

	const CTL::String credentials(reinterpret_cast<char*>(decoded.data()), actualLen);
	const size_t colonPos = credentials.find(':');
	if (colonPos == CTL::String::npos) {
		return false;
	}

	outUsername = credentials.substr(0, colonPos);
	outPassword = credentials.substr(colonPos + 1);
	return !outUsername.empty();
}

CTL::String Request::GetClientIP() const {
	if (!channel) return {};
	const auto client = channel->getClient();
	if (!client) return {};
	try {
		return client->getClientHost().IPAddress;
	} catch (...) {}
	return {};
}

CTL::String Request::GetUserAgent() const {
	const CTL::String ua1("User-Agent");
	const CTL::String ua2("user-agent");
	try {
		if (RequestData.Headers.find(ua1) != RequestData.Headers.end()) {
			return RequestData.Headers.at(ua1);
		}
		if (RequestData.Headers.find(ua2) != RequestData.Headers.end()) {
			return RequestData.Headers.at(ua2);
		}
	} catch (...) {}
	return {};
}

ByteArray Request::GetBody() const {
	return RequestData.Body;
}


