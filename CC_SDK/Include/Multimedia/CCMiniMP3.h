#ifndef CC_MINI_MP3_H
#define CC_MINI_MP3_H

#include <cstdint>
#include <vector>
#include "cstddef"
#ifdef _WIN32
#include <windows.h>
#endif

namespace CTL {
    class Frame {
    public:
        size_t length = {};
        std::vector<uint8_t> frame{};
    };
    class PCM_Int16 {
    public:
        size_t length = {};
        unsigned long frames = {};
        std::vector<int16_t> frame{};
    };
    class MiniMP3Frames {
    public:
        size_t length = {};
        std::vector<Frame> frames{};
        std::vector<PCM_Int16> PCM{};
    };
    class MiniMP3 {
    public:
        int file_size = -1;
        int frame_byte_size = -1;
        int frame_count = -1;
        int samples_per_frame = -1;
        int channels = -1;
        int sampling_rate = -1;
        double frame_duration = -1;
        long long total_duration = -1;
        long long bit_rate = -1;
        bool is_vbr = false;
        bool has_cover = false;
    private:
        void* mp3d = nullptr;
        std::vector<uint8_t> mp3Data;
        int offsetof_t = 0;
    public:
        MiniMP3() = default;
        explicit MiniMP3(const char* filename);
        ~MiniMP3();
        void Init(const char* filename);
        MiniMP3Frames GetFrames(bool DecodingPCM = false);
        [[nodiscard]] PCM_Int16 GetPCM(size_t frame_index) const;
    };
}


#endif
