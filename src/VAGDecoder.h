#pragma once
#include <vector>
#include <cstdint>
#include <cstddef> // for size_t

class VAGDecoder {
public:
    struct VAGHeader {
        char magic[4];          // "VAGp"
        uint32_t version;       // Big Endian
        uint32_t reserved1;
        uint32_t dataSize;      // Big Endian
        uint32_t sampleRate;    // Big Endian
        uint8_t reserved2[12];
        char name[16];
    };

    // Decode a standard VAG file with header
    static bool Decode(const std::vector<uint8_t>& vagData, std::vector<uint8_t>& outWavData);
    
    // Decode RAW SPU2 ADPCM stream (headerless)
    static bool DecodeRaw(const std::vector<uint8_t>& rawData, std::vector<uint8_t>& outWavData, int sampleRate = 44100);

    // Scan a buffer for offsets of valid "VAGp" headers
    static std::vector<size_t> ScanForHeaders(const std::vector<uint8_t>& buffer);

private:
    static bool DecodeInternal(const uint8_t* data, size_t size, std::vector<uint8_t>& outWav, int sampleRate);
};