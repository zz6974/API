#ifndef CC_SDP_BUILDER_H
#define CC_SDP_BUILDER_H

#include <string>
#include <vector>
#include <map>
#include <sstream>

namespace CTL {
    class SDPBuilder {
        std::string session_id;
        std::string session_version;
        std::string session_name;
        std::string origin_username;
        std::string origin_session_id;
        std::string origin_nettype;
        std::string origin_addrtype;
        std::string origin_address;
        std::string connection_nettype;
        std::string connection_addrtype;
        std::string connection_address;
        std::string timing_start;
        std::string timing_stop;
        struct MediaDescription {
            std::string media_type;      // video, audio, etc.
            int port;                   // transport port
            std::string protocol;       // RTP/AVP, etc.
            int fmt;                    // format identifier
            std::string rtpmap;         // rtpmap information
            std::string fmtp;           // format specific parameters
            std::string control_url;    // control URL
            std::vector<std::string> attributes; // additional attributes
        };
        std::vector<MediaDescription> media_descriptions;
    public:
        SDPBuilder() : session_id("0"), session_version("0"), session_name("RTSP Session"),
                       origin_username("-"), origin_session_id("0"), origin_nettype("IN"),
                       origin_addrtype("IP4"), origin_address("127.0.0.1"),
                       connection_nettype("IN"), connection_addrtype("IP4"),
                       connection_address("0.0.0.0"), timing_start("0"), timing_stop("0") {}

        // 设置会话ID
        SDPBuilder& setSessionId(const std::string& id) {
            session_id = id;
            return *this;
        }

        // 设置会话版本
        SDPBuilder& setSessionVersion(const std::string& version) {
            session_version = version;
            return *this;
        }

        // 设置会话名称
        SDPBuilder& setSessionName(const std::string& name) {
            session_name = name;
            return *this;
        }

        // 设置源信息
        SDPBuilder& setOriginInfo(const std::string& username, const std::string& session_id_val,
                                 const std::string& nettype = "IN", const std::string& addrtype = "IP4",
                                 const std::string& address = "127.0.0.1") {
            origin_username = username;
            origin_session_id = session_id_val;
            origin_nettype = nettype;
            origin_addrtype = addrtype;
            origin_address = address;
            return *this;
        }

        // 设置连接信息
        SDPBuilder& setConnectionInfo(const std::string& nettype = "IN", const std::string& addrtype = "IP4",
                                     const std::string& address = "0.0.0.0") {
            connection_nettype = nettype;
            connection_addrtype = addrtype;
            connection_address = address;
            return *this;
        }

        // 添加视频媒体描述
        SDPBuilder& addVideoMedia(int port, const std::string& codec, int payload_type,
                                 const std::string& clock_rate = "90000",
                                 const std::string& fmtp_params = "") {
            MediaDescription media;
            media.media_type = "video";
            media.port = port;
            media.protocol = "RTP/AVP";
            media.fmt = payload_type;

            if (codec == "H264") {
                media.rtpmap = std::to_string(payload_type) + " " + codec + "/" + clock_rate;
                if (!fmtp_params.empty()) {
                    media.fmtp = "a=fmtp:" + std::to_string(payload_type) + " " + fmtp_params;
                }
            } else if (codec == "H265") {
                media.rtpmap = std::to_string(payload_type) + " " + codec + "/" + clock_rate;
                if (!fmtp_params.empty()) {
                    media.fmtp = "a=fmtp:" + std::to_string(payload_type) + " " + fmtp_params;
                }
            } else if (codec == "VP8" || codec == "VP9") {
                media.rtpmap = std::to_string(payload_type) + " " + codec + "/" + clock_rate;
            } else {
                media.rtpmap = std::to_string(payload_type) + " " + codec + "/" + clock_rate;
            }

            media.control_url = "stream";
            media_descriptions.push_back(media);
            return *this;
        }

        // 添加音频媒体描述
        SDPBuilder& addAudioMedia(int port, const std::string& codec, int payload_type,
                                 const std::string& clock_rate = "8000", int channels = 1,
                                 const std::string& fmtp_params = "") {
            MediaDescription media;
            media.media_type = "audio";
            media.port = port;
            media.protocol = "RTP/AVP";
            media.fmt = payload_type;

            if (channels > 1) {
                media.rtpmap = std::to_string(payload_type) + " " + codec + "/" + clock_rate + "/" + std::to_string(channels);
            } else {
                media.rtpmap = std::to_string(payload_type) + " " + codec + "/" + clock_rate;
            }

            if (!fmtp_params.empty()) {
                media.fmtp = "a=fmtp:" + std::to_string(payload_type) + " " + fmtp_params;
            }

            media.control_url = "stream";
            media_descriptions.push_back(media);
            return *this;
        }

        SDPBuilder& addMedia(const CTL::String& sdp) {
                        std::istringstream stream(sdp);
            std::string line;
            MediaDescription current_media;
            bool has_current_media = false;

            while (std::getline(stream, line)) {
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                if (line.empty()) continue;

                size_t colon_pos = line.find('=');
                if (colon_pos == std::string::npos) continue;

                std::string type = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);

                if (type == "m") {
                    if (has_current_media) {
                        media_descriptions.push_back(current_media);
                    }

                    std::istringstream media_stream(value);
                    std::string protocol;
                    int port, fmt;
                    media_stream >> current_media.media_type >> port >> protocol >> fmt;

                    current_media.port = port;
                    current_media.protocol = protocol;
                    current_media.fmt = fmt;
                    current_media.rtpmap.clear();
                    current_media.fmtp.clear();
                    current_media.control_url = "stream";
                    current_media.attributes.clear();
                    has_current_media = true;
                }
                else if (type == "a" && has_current_media) {
                    if (value.substr(0, 7) == "rtpmap:") {
                        current_media.rtpmap = value.substr(7);
                    }
                    else if (value.substr(0, 5) == "fmtp:") {
                        current_media.fmtp = "a=fmtp:" + value.substr(5);
                    }
                    else if (value.substr(0, 8) == "control:") {
                        current_media.control_url = value.substr(8);
                    }
                    else {
                        current_media.attributes.push_back("a=" + value);
                    }
                }
            }

            if (has_current_media) {
                media_descriptions.push_back(current_media);
            }

            return *this;
        }

        // 添加自定义属性
        SDPBuilder& addAttribute(const std::string& attr) {
            if (!media_descriptions.empty()) {
                media_descriptions.back().attributes.push_back("a=" + attr);
            }
            return *this;
        }

        // 生成SDP字符串
        std::string build() const {
            std::ostringstream sdp;

            // 版本行
            sdp << "v=0\r\n";

            // 源行
            sdp << "o=" << origin_username << " " << origin_session_id << " " << session_version
                << " " << origin_nettype << " " << origin_addrtype << " " << origin_address << "\r\n";

            // 会话名称
            sdp << "s=" << session_name << "\r\n";

            // 连接信息
            sdp << "c=" << connection_nettype << " " << connection_addrtype << " " << connection_address << "\r\n";

            // 时间信息
            sdp << "t=" << timing_start << " " << timing_stop << "\r\n";

            // 媒体描述
            for (const auto& media : media_descriptions) {
                sdp << "m=" << media.media_type << " " << media.port << " " << media.protocol << " " << media.fmt << "\r\n";
                sdp << "a=rtpmap:" << media.rtpmap << "\r\n";
                if (!media.fmtp.empty()) {
                    sdp << media.fmtp << "\r\n";
                }
                sdp << "a=control:" << media.control_url << "\r\n";

                // 添加自定义属性
                for (const auto& attr : media.attributes) {
                    sdp << attr << "\r\n";
                }
            }

            return sdp.str();
        }

        static SDPBuilder parse(const std::string& sdp) {
            SDPBuilder builder;
            std::istringstream stream(sdp);
            std::string line;
            while (std::getline(stream, line)) {
                // 移除行末的换行符和回车符
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                if (line.empty()) continue;
                // 解析行类型 (字母部分)
                size_t colon_pos = line.find('=');
                if (colon_pos == std::string::npos) continue;
                std::string type = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                if (type == "v") {
                    // 版本行 - 通常为 "v=0"
                }
                else if (type == "o") {
                    // 源行: o=username sessionid version nettype addrtype address
                    std::istringstream origin_stream(value);
                    std::string username, session_id, version, nettype, addrtype, address;
                    origin_stream >> username >> session_id >> version >> nettype >> addrtype >> address;
                    builder.setOriginInfo(username, session_id, nettype, addrtype, address);
                }
                else if (type == "s") {
                    // 会话名称
                    builder.setSessionName(value);
                }
                else if (type == "c") {
                    // 连接信息: c=nettype addrtype address
                    std::istringstream conn_stream(value);
                    std::string nettype, addrtype, address;
                    conn_stream >> nettype >> addrtype >> address;
                    builder.setConnectionInfo(nettype, addrtype, address);
                }
                else if (type == "t") {
                    // 时间信息: t=start stop
                    size_t space_pos = value.find(' ');
                    if (space_pos != std::string::npos) {
                        builder.timing_start = value.substr(0, space_pos);
                        builder.timing_stop = value.substr(space_pos + 1);
                    }
                }
                else if (type == "m") {
                    // 媒体行: m=media port protocol fmt
                    std::istringstream media_stream(value);
                    std::string media_type, protocol;
                    int port, fmt;
                    media_stream >> media_type >> port >> protocol >> fmt;

                    // 简单处理，假设是视频或音频
                    if (media_type == "video") {
                        builder.addVideoMedia(port, "H264", fmt);
                    } else if (media_type == "audio") {
                        builder.addAudioMedia(port, "PCMU", fmt);
                    }
                }
                else if (type == "a") {
                    // 属性行
                    if (value.substr(0, 7) == "rtpmap:") {
                        // rtpmap 属性
                        size_t space_pos = value.find(' ', 7);
                        if (space_pos != std::string::npos) {
                            std::string rtpmap_info = value.substr(7);
                            // 这里可以进一步解析rtpmap信息，但需要根据当前media进行设置
                        }
                    } else if (value.substr(0, 5) == "fmtp:") {
                        // fmtp 属性
                        size_t space_pos = value.find(' ', 5);
                        if (space_pos != std::string::npos) {
                            std::string fmtp_info = value.substr(5);
                            // 这里可以进一步解析fmtp信息
                        }
                    } else if (value.substr(0, 8) == "control:") {
                        // control 属性
                        std::string control_url = value.substr(8);
                        // 需要更新当前媒体描述的control_url
                    }
                }
            }
            return builder;
        }

    };
}

#endif
