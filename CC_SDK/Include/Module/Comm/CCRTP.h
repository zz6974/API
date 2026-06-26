#ifndef SRCCTL_CC_RTP_H
#define SRCCTL_CC_RTP_H

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <mutex>
#include "map"
#include "vector"
#include <utility>
#include "Socket/CCSocket.h"

namespace CTL{
    struct RtpHeader {
        uint8_t version_p_x_cc;    // 版本(2) | 填充(1) | 扩展(1) | CSRC计数(4)
        uint8_t m_payload_type;    // 标记(1) | 负载类型(7)
        uint16_t sequence_number;  // 序列号
        uint32_t timestamp;        // 时间戳
        uint32_t ssrc;             // 同步源标识符
    };

    // 分片重组上下文（增强版）
    struct FrameContext {
        uint16_t base_seq = 0;     // 初始序列号
        uint16_t expected_seq = 0; // 期望的下一个相对序列号
        std::vector<uint8_t> buffer; // 重组缓冲区
        uint32_t timestamp = 0;      // 时间戳
        bool is_complete = false;    // 是否完成重组
        size_t max_gap = 1000;       // 最大允许间隔（防乱序）
    };

    // 序列号处理工具函数
    inline uint16_t get_relative_seq(uint16_t base, uint16_t current) {
        int16_t diff = current - base;
        return (diff + 65536) % 65536; // 处理负数差值
    }

    // 分片处理函数
    inline void process_rtp_packet(const uint8_t* packet, int len,
        std::map<uint16_t, FrameContext>& frame_map,FrameContext* frame_ctx)
    {
        if (len < sizeof(RtpHeader)) return;

        // 解析RTP头部
        RtpHeader* header = (RtpHeader*)packet;
        uint16_t seq = ntohs(header->sequence_number);
        uint32_t ts = ntohl(header->timestamp);
        const uint8_t* payload = packet + sizeof(RtpHeader);
        int payload_len = len - sizeof(RtpHeader);

        // 处理填充字节
        if (header->version_p_x_cc & 0x01) {
            int padding = packet[len - 1] & 0x0F;
            payload_len -= padding;
        }
        // std::cout << "seq: " << seq << " tm: " << ts << " len: " << payload_len << std::endl;
        // 根据负载类型处理（假设自定义编码类型为96）
        // if (header->m_payload_type != 96) return;

        // 获取或创建帧上下文
        FrameContext& ctx = frame_map[ts]; // 使用时间戳作为帧标识
        if (ctx.base_seq == 0) {
            ctx.base_seq = seq; // 初始化基准序列号
            ctx.expected_seq = 0; // 相对序列号从0开始
        }

        // 计算相对序列号
        uint16_t rel_seq = get_relative_seq(ctx.base_seq, seq);

        // 检测回绕后的异常间隔
        if (rel_seq > ctx.expected_seq + ctx.max_gap) {
            // 可能是新的帧开始（序列号回绕后重置）
            if (rel_seq < ctx.expected_seq) {
                ctx = FrameContext(); // 重置上下文
                ctx.base_seq = seq;
                ctx.expected_seq = 0;
            } else {
                // 丢弃异常包
                return;
            }
        }

        // 拼接数据
        ctx.buffer.insert(ctx.buffer.end(), payload, payload + payload_len);
        ctx.expected_seq++;

        // 检查是否完成（假设最大帧大小50KB）
        if (ctx.buffer.size() >= 50 * 1024 || (header->m_payload_type & 0x80)) {
            ctx.is_complete = true;
            // 提取完整帧
            // std::cout << "Frame complete. Size: " << ctx.buffer.size() << " bytes" << std::endl;
            // 这里可调用解码器处理完整帧
            // decode_frame(ctx.buffer.data(), ctx.buffer.size());
            *frame_ctx = ctx;
            frame_map.erase(ts);
        }
    }
}

#endif