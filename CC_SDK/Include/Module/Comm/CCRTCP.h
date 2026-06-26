#ifndef SRCCTL_CC_RTCP_H
#define SRCCTL_CC_RTCP_H

#include <cstdint>
#include <vector>
#include <cstring>
#include <iostream>
#ifdef _WIN32
#include "winsock2.h"
#else
#include <arpa/inet.h>  // for ntohs, ntohl
#endif

namespace CTL {
#pragma pack(push, 1) // 禁用结构体对齐

    // RTCP 包类型定义
    enum RtcpPacketType {
        RTCP_SR = 200,    // Sender Report
        RTCP_RR = 201,    // Receiver Report
        RTCP_SDES = 202,  // Source Description
        RTCP_BYE = 203,   // Goodbye
        RTCP_APP = 204    // Application-defined
    };

    // SDES 项类型定义
    enum SdesItemType {
        SDES_END = 0,
        SDES_CNAME = 1,
        SDES_NAME = 2,
        SDES_EMAIL = 3,
        SDES_PHONE = 4,
        SDES_LOC = 5,
        SDES_TOOL = 6,
        SDES_NOTE = 7,
        SDES_PRIV = 8
    };

    // RTCP 通用头部 (4字节)
    struct RtcpCommonHeader {
        uint8_t version : 2;  // 版本号 (固定为2)
        uint8_t padding : 1;  // 填充标志
        uint8_t count : 5;    // 接收报告计数或源计数
        uint8_t pt;           // 包类型
        uint16_t length;      // 长度 (以32位字为单位，减1)
    };

    // 发送者信息 (SR包专用)
    struct RtcpSenderInfo {
        uint32_t ntp_msw;     // NTP时间戳高位
        uint32_t ntp_lsw;     // NTP时间戳低位
        uint32_t rtp_ts;      // RTP时间戳
        uint32_t pkt_count;   // 发送的包总数
        uint32_t octet_count; // 发送的字节总数
    };

    // 接收报告块 (SR/RR共用)
    struct RtcpReportBlock {
        uint32_t ssrc;        // 数据源SSRC
        uint8_t fraction_lost; // 丢包率 (1/256)
        int32_t cum_pkt_loss;  // 累计丢包数
        uint32_t ext_high_seq; // 扩展最高序列号
        uint32_t jitter;       // 抖动值
        uint32_t lsr;          // 上次SR时间戳
        uint32_t dlsr;         // 上次SR到现在的延迟
    };

    // SDES项 (源描述)
    struct RtcpSdesItem {
        uint8_t type;          // 项类型
        uint8_t len;           // 数据长度
        char data[256];        // 文本内容
    };

    // SDES块 (包含一个SSRC的所有描述项)
    struct RtcpSdesChunk {
        uint32_t ssrc;         // 关联的SSRC
        std::vector<RtcpSdesItem> items; // 描述项列表
    };

#pragma pack(pop)

    class RtcpPacket {
    public:
        RtcpPacket() = default;
        ~RtcpPacket() = default;

        // 解析RTCP复合包
        bool parse(const uint8_t* data, size_t len);

        // 获取包类型
        RtcpPacketType getPacketType() const { return m_packetType; }

        // 获取发送者SSRC (SR包)
        uint32_t getSenderSsrc() const { return m_senderSsrc; }

        // 获取接收报告块数量
        size_t getReportBlockCount() const { return m_reportBlocks.size(); }

        // 获取接收报告块
        const std::vector<RtcpReportBlock>& getReportBlocks() const { return m_reportBlocks; }

        // 获取发送者信息 (SR包)
        const RtcpSenderInfo* getSenderInfo() const { return m_senderInfo.get(); }

        // 获取SDES块
        const std::vector<RtcpSdesChunk>& getSdesChunks() const { return m_sdesChunks; }

        // 打印包信息
        void printInfo() const;

    private:
        RtcpPacketType m_packetType = RTCP_SR;
        uint32_t m_senderSsrc = 0;
        std::unique_ptr<RtcpSenderInfo> m_senderInfo;
        std::vector<RtcpReportBlock> m_reportBlocks;
        std::vector<RtcpSdesChunk> m_sdesChunks;

        // 解析各个类型的RTCP包
        bool parseSrPacket(const uint8_t* data, size_t len);
        bool parseRrPacket(const uint8_t* data, size_t len);
        bool parseSdesPacket(const uint8_t* data, size_t len);
        bool parseByePacket(const uint8_t* data, size_t len);
    };

    // 实现解析函数
    inline bool RtcpPacket::parse(const uint8_t* data, size_t len) {
       if (!data || len < sizeof(RtcpCommonHeader)) {
            std::cerr << "RTCP parse error: Invalid input data or insufficient length ("
                      << len << " < " << sizeof(RtcpCommonHeader) << ")" << std::endl;
            return false;
        }

        const auto* header = reinterpret_cast<const RtcpCommonHeader*>(data);

        // 详细版本检查
        if (header->version != 2) {
            std::cerr << "RTCP parse error: Invalid version "
                      << static_cast<int>(header->version)
                      << " (expected 2)" << std::endl;
            return false;
        }

        // 包类型检查
        RtcpPacketType packetType = static_cast<RtcpPacketType>(header->pt);
        if (packetType < RTCP_SR || packetType > RTCP_APP) {
            std::cerr << "RTCP parse error: Unsupported packet type "
                      << static_cast<int>(header->pt) << std::endl;
            return false;
        }

        // 获取实际包长度(字节)
        size_t declaredLen = (ntohs(header->length) + 1) * 4;
        if (declaredLen > len) {
            std::cerr << "RTCP parse error: Declared length (" << declaredLen
                      << ") exceeds actual data length (" << len << ")" << std::endl;
            return false;
        }

        if (declaredLen < sizeof(RtcpCommonHeader)) {
            std::cerr << "RTCP parse error: Declared length too small ("
                      << declaredLen << ")" << std::endl;
            return false;
        }

        m_packetType = packetType;

        // 根据包类型解析
        bool result = false;
        switch (m_packetType) {
            case RTCP_SR:
                result = parseSrPacket(data, declaredLen);
                break;
            case RTCP_RR:
                result = parseRrPacket(data, declaredLen);
                break;
            case RTCP_SDES:
                result = parseSdesPacket(data, declaredLen);
                break;
            case RTCP_BYE:
                result = parseByePacket(data, declaredLen);
                break;
            default:
                std::cerr << "RTCP parse error: Unhandled packet type "
                          << static_cast<int>(header->pt) << std::endl;
                return false;
        }

        if (!result) {
            std::cerr << "RTCP parse error: Failed to parse "
                      << static_cast<int>(m_packetType) << " packet" << std::endl;
        }

        return result;
    }

    inline bool RtcpPacket::parseSrPacket(const uint8_t* data, size_t len) {
        if (len < sizeof(RtcpCommonHeader) + sizeof(uint32_t) + sizeof(RtcpSenderInfo)) {
            return false;
        }

        const uint8_t* ptr = data + sizeof(RtcpCommonHeader);

        // 读取发送者SSRC
        m_senderSsrc = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
        ptr += sizeof(uint32_t);

        // 读取发送者信息
        m_senderInfo = std::make_unique<RtcpSenderInfo>();
        m_senderInfo->ntp_msw = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
        ptr += sizeof(uint32_t);
        m_senderInfo->ntp_lsw = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
        ptr += sizeof(uint32_t);
        m_senderInfo->rtp_ts = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
        ptr += sizeof(uint32_t);
        m_senderInfo->pkt_count = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
        ptr += sizeof(uint32_t);
        m_senderInfo->octet_count = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
        ptr += sizeof(uint32_t);

        // 读取接收报告块
        uint8_t reportCount = reinterpret_cast<const RtcpCommonHeader*>(data)->count;
        for (int i = 0; i < reportCount && ptr + sizeof(RtcpReportBlock) <= data + len; ++i) {
            RtcpReportBlock block{};
            block.ssrc = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
            ptr += sizeof(uint32_t);
            block.fraction_lost = *ptr++;
            block.cum_pkt_loss = ntohl(*reinterpret_cast<const int32_t*>(ptr - 1)) & 0x00FFFFFF;
            ptr += 3;
            block.ext_high_seq = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
            ptr += sizeof(uint32_t);
            block.jitter = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
            ptr += sizeof(uint32_t);
            block.lsr = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
            ptr += sizeof(uint32_t);
            block.dlsr = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
            ptr += sizeof(uint32_t);

            m_reportBlocks.push_back(block);
        }

        return true;
    }

    inline bool RtcpPacket::parseRrPacket(const uint8_t* data, size_t len) {
        if (len < sizeof(RtcpCommonHeader) + sizeof(uint32_t)) {
            return false;
        }

        const uint8_t* ptr = data + sizeof(RtcpCommonHeader);

        // 读取接收者SSRC
        m_senderSsrc = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
        ptr += sizeof(uint32_t);

        // 读取接收报告块
        uint8_t reportCount = reinterpret_cast<const RtcpCommonHeader*>(data)->count;
        for (int i = 0; i < reportCount && ptr + sizeof(RtcpReportBlock) <= data + len; ++i) {
            RtcpReportBlock block{};
            block.ssrc = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
            ptr += sizeof(uint32_t);
            block.fraction_lost = *ptr++;
            block.cum_pkt_loss = ntohl(*reinterpret_cast<const int32_t*>(ptr - 1)) & 0x00FFFFFF;
            ptr += 3;
            block.ext_high_seq = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
            ptr += sizeof(uint32_t);
            block.jitter = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
            ptr += sizeof(uint32_t);
            block.lsr = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
            ptr += sizeof(uint32_t);
            block.dlsr = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
            ptr += sizeof(uint32_t);

            m_reportBlocks.push_back(block);
        }

        return true;
    }

    inline bool RtcpPacket::parseSdesPacket(const uint8_t* data, size_t len) {
        const uint8_t* ptr = data + sizeof(RtcpCommonHeader);
        uint8_t sourceCount = reinterpret_cast<const RtcpCommonHeader*>(data)->count;

        for (int i = 0; i < sourceCount && ptr < data + len; ++i) {
            RtcpSdesChunk chunk{};

            // 读取SSRC
            if (ptr + sizeof(uint32_t) > data + len) break;
            chunk.ssrc = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
            ptr += sizeof(uint32_t);

            // 读取SDES项
            while (ptr < data + len) {
                uint8_t itemType = *ptr++;
                if (itemType == SDES_END) {
                    // 跳过填充字节直到4字节边界
                    while ((ptr - data) % 4 != 0 && ptr < data + len) {
                        ptr++;
                    }
                    break;
                }

                if (ptr >= data + len) break;
                uint8_t itemLen = *ptr++;

                if (ptr + itemLen > data + len) break;

                RtcpSdesItem item{};
                item.type = itemType;
                item.len = itemLen;
                memcpy(item.data, ptr, itemLen);
                item.data[itemLen] = '\0';

                chunk.items.push_back(item);
                ptr += itemLen;
            }

            m_sdesChunks.push_back(chunk);
        }

        return true;
    }

    inline bool RtcpPacket::parseByePacket(const uint8_t* data, size_t len) {
        const uint8_t* ptr = data + sizeof(RtcpCommonHeader);
        uint8_t sourceCount = reinterpret_cast<const RtcpCommonHeader*>(data)->count;

        // 读取SSRC列表
        for (int i = 0; i < sourceCount && ptr + sizeof(uint32_t) <= data + len; ++i) {
            uint32_t ssrc = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
            // 这里可以存储SSRC列表
            ptr += sizeof(uint32_t);
        }

        // 如果有原因短语
        if (ptr < data + len) {
            uint8_t reasonLen = *ptr++;
            if (ptr + reasonLen <= data + len) {
                std::string reason(reinterpret_cast<const char*>(ptr), reasonLen);
                // 这里可以存储离开原因
            }
        }

        return true;
    }

    inline void RtcpPacket::printInfo() const {
        std::cout << "RTCP Packet Type: ";
        switch (m_packetType) {
            case RTCP_SR: std::cout << "Sender Report"; break;
            case RTCP_RR: std::cout << "Receiver Report"; break;
            case RTCP_SDES: std::cout << "Source Description"; break;
            case RTCP_BYE: std::cout << "Goodbye"; break;
            default: std::cout << "Unknown"; break;
        }
        std::cout << std::endl;

        std::cout << "Sender SSRC: " << m_senderSsrc << std::endl;

        if (m_senderInfo) {
            std::cout << "Sender Info:" << std::endl;
            std::cout << "  NTP Timestamp: " << m_senderInfo->ntp_msw << "." << m_senderInfo->ntp_lsw << std::endl;
            std::cout << "  RTP Timestamp: " << m_senderInfo->rtp_ts << std::endl;
            std::cout << "  Packet Count: " << m_senderInfo->pkt_count << std::endl;
            std::cout << "  Octet Count: " << m_senderInfo->octet_count << std::endl;
        }

        std::cout << "Report Blocks: " << m_reportBlocks.size() << std::endl;
        for (size_t i = 0; i < m_reportBlocks.size(); ++i) {
            const auto& block = m_reportBlocks[i];
            std::cout << "  Block " << i << ":" << std::endl;
            std::cout << "    SSRC: " << block.ssrc << std::endl;
            std::cout << "    Fraction Lost: " << static_cast<int>(block.fraction_lost) << "/256" << std::endl;
            std::cout << "    Cumulative Packet Loss: " << block.cum_pkt_loss << std::endl;
            std::cout << "    Extended Highest Sequence: " << block.ext_high_seq << std::endl;
            std::cout << "    Jitter: " << block.jitter << std::endl;
        }

        std::cout << "SDES Chunks: " << m_sdesChunks.size() << std::endl;
        for (size_t i = 0; i < m_sdesChunks.size(); ++i) {
            const auto& chunk = m_sdesChunks[i];
            std::cout << "  Chunk " << i << " (SSRC: " << chunk.ssrc << "):" << std::endl;
            for (const auto& item : chunk.items) {
                std::cout << "    Type: " << static_cast<int>(item.type)
                         << ", Length: " << static_cast<int>(item.len)
                         << ", Data: " << item.data << std::endl;
            }
        }
    }

} // namespace CTL

#endif // SRCCTL_CC_RTCP_H
