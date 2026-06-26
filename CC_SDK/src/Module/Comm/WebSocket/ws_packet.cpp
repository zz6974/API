/*
* The MIT License (MIT)
* Copyright(c) 2020 BeikeSong

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "sha1.h"
#include "base64.h"
#include "ws_packet.h"
#include "string_helper.h"

#define SP " "
#define EOL "\r\n"
#define DEFAULT_HTTP_VERSION "HTTP/1.1"

WebSocketPacket::WebSocketPacket()
{
	fin_ = 0;
	rsv1_ = 0;
	rsv2_ = 0;
	rsv3_ = 0;
	opcode_ = 0;
	mask_ = 0;
	length_type_ = 0;
	masking_key_[4] = {0};
	payload_length_ = 0;
}

int32_t WebSocketPacket::recv_handshake(ByteBuffer &input)
{
	if (input.length() > WS_MAX_HANDSHAKE_FRAME_SIZE)
	{
		return WS_MAX_HANDSHAKE_FRAME_SIZE;
	}

	std::string inputstr(input.bytes(), input.length());
	int32_t frame_size = fetch_hs_element(inputstr);
	if (frame_size <= 0)
	{
		//continue recving data;
		input.resetoft();
		return 0;
	}
	const bool A = get_param("Upgrade") != "websocket";
	const bool B = get_param("Connection").find("Upgrade") == std::string::npos;
	const bool C = get_param("Sec-WebSocket-Version") != "13";
	const bool D = get_param("Sec-WebSocket-Key").empty();
	if (A || B || C || D){
		input.resetoft();
		return WS_ERROR_INVALID_HANDSHAKE_PARAMS;
	}

	hs_length_ = frame_size;
	input.skip_x(hs_length_);
	return 0;
}

int32_t WebSocketPacket::pack_handshake_rsp(std::string &hs_rsp)
{
	std::string magic_key = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	std::string raw_key = get_param("Sec-WebSocket-Key") + magic_key;

	std::string sha1_key = SHA1::SHA1HashString(raw_key);
	char accept_key[128] = {0};
	Base64encode(accept_key, sha1_key.c_str(), sha1_key.length());

	std::ostringstream sstream;
	sstream << "HTTP/1.1 101 Switching Protocols" << EOL;
	sstream << "Connection: upgrade" << EOL;
	sstream << "Upgrade: websocket" << EOL;
	sstream << "Sec-WebSocket-Accept: " << accept_key << EOL;
	if (get_param("Sec-WebSocket-Protocol") != "")
	{
		sstream << "Sec-WebSocket-Protocol: chat" << EOL;
	}
	sstream << EOL;
	hs_rsp = sstream.str();
	return 0;
}

uint64_t WebSocketPacket::recv_dataframe(ByteBuffer &input)
{
	int header_size = fetch_frame_info(input);

	//std::cout << "WebSocketPacket: header size: " << header_size
	//		  << " payload_length_: " << payload_length_ << " input.length: " << input.length() << std::endl;

	if (payload_length_ + header_size > input.length())
	{
		// buffer size is not enough, so we continue recving data
		// std::cout << "WebSocketPacket: recv_dataframe: continue recving data." << std::endl;
		input.resetoft();
		return 0;
	}

	fetch_payload(input);

	// std::cout << "WebSocketPacket: received data with header size: " << header_size << " payload size:" << payload_length_
	// 		  << " input oft size:" << input.getoft() << std::endl;
	//return payload_length_;
	return input.getoft();
}

int32_t WebSocketPacket::fetch_frame_info(ByteBuffer &input)
{
	// FIN, opcode
	uint8_t onebyte = 0;
	input.read_bytes_x((char *)&onebyte, 1);
	fin_ = onebyte >> 7;
	opcode_ = onebyte & 0x7F;

	// payload length
	input.read_bytes_x((char *)&onebyte, 1);
	mask_ = onebyte >> 7 & 0x01;
	length_type_ = onebyte & 0x7F;

	if (length_type_ < 126)
	{
		payload_length_ = length_type_;
	}
	else if (length_type_ == 126)
	{
		/*
		uint16_t len = 0;
		input.read_bytes_x((char *)&len, 2);
		len = (len << 8) | (len >>8 & 0xFF);
		payload_length_ = len;
		*/
		uint16_t len = 0;
		uint8_t array[2] = {0};
		input.read_bytes_x(reinterpret_cast<char *>(array), 2);
		len = static_cast<uint16_t>(array[0] << 8) | static_cast<uint16_t>(array[1]);
		payload_length_ = len;
	}
	else if (length_type_ == 127)
	{
		// if you don't have ntohll
		uint8_t array[8] = {0};
		input.read_bytes_x(reinterpret_cast<char *>(array), 8);
		if (payload_length_ > 0xFFFFFFFF)
		{
			//std::cout >>
		}
	}
	else
	{
	}

	// masking key
	if (mask_ == 1)
	{
		input.read_bytes_x((char *)masking_key_, 4);
	}

	// return header size
	return input.getoft();
}

int32_t WebSocketPacket::fetch_payload(ByteBuffer &input)
{
	char real = 0;
	if (mask_ == 1)
	{
		for (uint64_t i = 0; i < payload_length_; i++)
		{
			input.read_bytes_x(&real, 1);
			real = real ^ masking_key_[i % 4];
			payload_.append(&real, 1);
		}
	}
	else
	{
		if(payload_length_ > 0){
            payload_.append(input.curat(), payload_length_);
            input.skip_x(payload_length_);
        }
	}
	return 0;
}

int32_t WebSocketPacket::pack_dataframe(ByteBuffer &output)
{
	uint8_t onebyte = 0;
	onebyte |= (fin_ << 7);
	onebyte |= (rsv1_ << 6);
	onebyte |= (rsv2_ << 5);
	onebyte |= (rsv3_ << 4);
	onebyte |= (opcode_ & 0x0F);
	output.append((char *)&onebyte, 1);

	onebyte = 0;
	//set mask flag
	onebyte = onebyte | (mask_ << 7);
	uint8_t length_type = get_length_type();

	if (length_type < 126)
	{
		onebyte |= payload_length_;
		output.append((char *)&onebyte, 1);
	}
	else if (length_type == 126)
	{
		onebyte |= length_type;
		output.append((char *)&onebyte, 1);

		// also can use htons
		onebyte = (payload_length_ >> 8) & 0xFF;
		output.append((char *)&onebyte, 1);
		onebyte = payload_length_ & 0xFF;
		output.append((char *)&onebyte, 1);
	}
	else if (length_type == 127)
	{
		onebyte |= length_type;
		output.append((char *)&onebyte, 1);

		// also can use htonll if you have it
		onebyte = (payload_length_ >> 56) & 0xFF;
		output.append((char *)&onebyte, 1);
		onebyte = (payload_length_ >> 48) & 0xFF;
		output.append((char *)&onebyte, 1);
		onebyte = (payload_length_ >> 40) & 0xFF;
		output.append((char *)&onebyte, 1);
		onebyte = (payload_length_ >> 32) & 0xFF;
		output.append((char *)&onebyte, 1);
		onebyte = (payload_length_ >> 24) & 0xFF;
		output.append((char *)&onebyte, 1);
		onebyte = (payload_length_ >> 16) & 0xFF;
		output.append((char *)&onebyte, 1);
		onebyte = (payload_length_ >> 8) & 0xFF;
		output.append((char *)&onebyte, 1);
		onebyte = payload_length_ & 0XFF;
		output.append((char *)&onebyte, 1);
	}
	else
	{
		return -1;
	}

	if (mask_ == 1)
	{
		char value = 0;
		// save masking key
		output.append((char *)masking_key_, 4);
		// std::cout << "WebSocketPacket: send data with header size: " << output.length()
		// 		  << " payload size:" << payload_length_ << std::endl;
		for (uint64_t i = 0; i < payload_length_; i++)
		{
			payload_.read_bytes_x(&value, 1);
			value = value ^ masking_key_[i % 4];
			output.append(&value, 1);
		}
	}
	else
	{
		// std::cout << "WebSocketPacket: send data with header size: " << output.length()
		// 		  << " payload size:" << payload_length_ << std::endl<<std::endl;
		output.append(payload_.bytes(), payload_.length());
	}

	return 0;
}

int32_t WebSocketPacket::fetch_hs_element(const std::string &msg)
{
	// two EOLs mean a completed http1.1 request line
	std::string::size_type endpos = msg.find(std::string(EOL) + EOL);
	if (endpos == std::string::npos)
	{
		return -1;
	}

	//can't find end of request line in current, and we continue receiving data
	std::vector<std::string> lines;
	if (strHelper::splitStr<std::vector<std::string> >
		(lines, msg.substr(0, endpos), EOL) < 2)
	{
		return -1;
	}

	std::vector<std::string>::iterator it = lines.begin();

	while ((it != lines.end()) && strHelper::trim(*it).empty())
	{
		++it;
	};

	std::vector<std::string> reqLineParams;
	if (strHelper::splitStr<std::vector<std::string> >
		(reqLineParams, *it, SP) < 3)
	{
		return -1;
	}

	mothod_ = strHelper::trim(reqLineParams[0]);
	uri_ = strHelper::trim(reqLineParams[1]);
	version_ = strHelper::trim(reqLineParams[2]);

	for (++it; it != lines.end(); ++it)
	{
		// header fields format:
		// field name: values
		std::string::size_type pos = it->find_first_of(":");
		if (pos == std::string::npos)
			continue; // invalid line

		std::string k = it->substr(0, pos);

		std::string v = it->substr(pos + 1);
		if (strHelper::trim(k).empty())
		{
			continue;
		}
		if (strHelper::trim(v).empty())
		{
			continue;
		}

		params_[k] = v;
		// std::cout << "handshake element k:" << k.c_str() << " v:" << v.c_str() << std::endl;
	}

	return endpos + 4;
}

void WebSocketPacket::set_payload(const char *buf, uint64_t size)
{
	payload_.append(buf, size);
	payload_length_ = payload_.length();
}

const uint8_t WebSocketPacket::get_length_type()
{
	if (payload_length_ < 126)
	{
		return (uint8_t)payload_length_;
	}
	else if (payload_length_ >= 126 && payload_length_ <= 0xFFFF)
	{
		return 126;
	}
	else
	{
		return 127;
	}
}

const uint8_t WebSocketPacket::get_header_size()
{
	int header_size = 0;
	if (get_length_type() < 126)
	{
		header_size = 2;
	}
	else if (get_length_type() == 126)
	{
		header_size = 4;
	}
	else if (get_length_type() == 127)
	{
		header_size = 2 + 8;
	}

	if (mask_ == 1)
	{
		header_size += 4;
	}
	return header_size;
}

bool WebSocketPacket::ValidateSecWebSocketAccept(const std::string &key, const std::string &accept) {
    std::string magic_key = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string raw_key = key + magic_key;
    std::string sha1_key = SHA1::SHA1HashString(raw_key);
    char accept_key[128] = {0};
    Base64encode(accept_key, sha1_key.c_str(), sha1_key.length());
    return accept == accept_key;
}

/*
*  ByteBuffer
*
*/
ByteBuffer::ByteBuffer()
{
	oft = 0;
}

ByteBuffer::~ByteBuffer()
{
}

bool ByteBuffer::require(int require)
{
	int len = length();

	return require <= len - oft;
}

char *ByteBuffer::curat()
{
	return (length() == 0) ? NULL : &data.at(oft);
}

int ByteBuffer::getoft()
{
	return oft;
}

bool ByteBuffer::skip_x(int size)
{
	if (require(size))
	{
		oft += size;
		return true;
	}
	else
	{
		return false;
	}
}

bool ByteBuffer::read_bytes_x(char *cb, int size)
{
	if (require(size))
	{
		memcpy(cb, &data.at(oft), size);
		oft += size;
		return true;
	}
	else
	{
		return false;
	}
}

void ByteBuffer::resetoft()
{
	oft = 0;
}

void ByteBuffer::clear() {
	data.clear();
	resetoft();
}

void ByteBuffer::assign(char* buf, const int size) {
	clear();
	data.assign(buf, buf + size);
}

bool ByteBuffer::operator==(const ByteBuffer &other) const {
	return data == other.data;
}

bool ByteBuffer::operator!=(const ByteBuffer &other) const {
	return data != other.data;
}

void ByteBuffer::operator+=(const ByteBuffer &other) {
	data.insert(data.end(), other.data.begin(), other.data.end());
}

void ByteBuffer::operator-=(const ByteBuffer &other) {
	data.erase(std::remove_if(data.begin(), data.end(), [&other](const CTL::Byte &byte) {
        return std::find(other.data.begin(), other.data.end(), 
            static_cast<char>(byte)) != other.data.end(); // 显式转换为char类型
    }), data.end());
}

int ByteBuffer::length()
{
	const int len = static_cast<int>(data.size());
	//srs_assert(len >= 0);
	return len;
}

char *ByteBuffer::bytes()
{
	return (length() == 0) ? nullptr : &data.at(0);
}

void ByteBuffer::erase(int size)
{
	if (size <= 0)
	{
		return;
	}

	if (size >= length())
	{
		data.clear();
		return;
	}

	data.erase(data.begin(), data.begin() + size);
}

void ByteBuffer::append(const char *bytes, int size)
{
	//srs_assert(size > 0);
	data.insert(data.end(), bytes, bytes + size);
}

void ByteBuffer::append(const ByteBuffer &other) {
	data.insert(data.end(), other.data.begin(), other.data.end());
}

void ByteBuffer::append(const CTL::ByteArray &other) {
	data.insert(data.end(), other.begin(), other.end());
}

void ByteBuffer::append(const CCVector<CTL::Byte> &other) {
	data.insert(data.end(), other.begin(), other.end());
}

void ByteBuffer::append(const CCList<CTL::Byte> &other) {
	data.insert(data.end(), other.begin(), other.end());
}

void ByteBuffer::append(std::initializer_list<CTL::Byte> initList) {
	data.insert(data.end(), initList.begin(), initList.end());
}

void ByteBuffer::append(const std::string &other) {
	data.insert(data.end(), other.begin(), other.end());
}

void ByteBuffer::operator=(const char *other) {
	data.assign(other, other + strlen(other));
}

ByteBuffer::ByteBuffer(const ByteBuffer &other) {
    data = other.data;
}

ByteBuffer::ByteBuffer(const CTL::ByteArray &other) {
    data.assign(other.begin(), other.end());
}

ByteBuffer::ByteBuffer(const CCVector<CTL::Byte> &other) {
	data.assign(other.begin(), other.end());
}

ByteBuffer::ByteBuffer(const CCList<CTL::Byte> &other) {
	data.assign(other.begin(), other.end());
}

ByteBuffer::ByteBuffer(std::initializer_list<CTL::Byte> initList) {
	data.assign(initList.begin(), initList.end());
}
