#pragma once
#include <cstdint>
#include <vector>
#include <string>

enum PS2_PSM {
    GS_PSM_32,
    GS_PSM_16,
    GS_PSM_16S,
    GS_PSM_8,
    GS_PSM_8H,
    GS_PSM_4,
    GS_PSM_4HL,
    GS_PSM_4HH
};

enum class TexLayout {
    Linear,
    Swizzled
};

struct TexData {
    int width = 0;
    int height = 0;
    PS2_PSM psm = GS_PSM_32;
    TexLayout layout = TexLayout::Linear;
    std::vector<uint8_t> pixels;
};

class TextureLoader {
public:
    bool LoadFromFile(const std::string& path, TexData& out);

private:
    uint32_t GetGSAddress(uint32_t x, uint32_t y, uint32_t bufferWidth, PS2_PSM psm);

    bool Read32(const uint8_t* src, TexData& out, uint32_t bufferWidth);
    bool Read16(const uint8_t* src, TexData& out, uint32_t bufferWidth);
    bool Read8(const uint8_t* src, TexData& out, uint32_t bufferWidth);
    bool Read4(const uint8_t* src, TexData& out, uint32_t bufferWidth);
};
