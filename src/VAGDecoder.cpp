#include "VAGDecoder.h"
#include <cstring>
#include <algorithm>
#include <cstdio>

// Coefficients for SPU2 ADPCM
static const double VagLut[5][2] = {
    { 0.0, 0.0 },
    {  60.0/64.0,   0.0 },
    { 115.0/64.0, -52.0/64.0 },
    {  98.0/64.0, -55.0/64.0 },
    { 122.0/64.0, -60.0/64.0 }
};

inline uint32_t Swap32(uint32_t v) {
    return ((v>>24)&0xff) | ((v<<8)&0xff00) | ((v>>8)&0xff00) | ((v<<24)&0xff000000);
}

std::vector<size_t> VAGDecoder::ScanForHeaders(const std::vector<uint8_t>& buffer) {
    std::vector<size_t> offsets;
    if (buffer.size() < 48) return offsets;

    // "VAGp" is typically 16-byte aligned in archives, but we scan 4-byte boundaries to be safe
    // Scan up to end minus header size
    for (size_t i = 0; i <= buffer.size() - 48; i += 4) {
        if (buffer[i] == 'V' && buffer[i+1] == 'A' && buffer[i+2] == 'G' && buffer[i+3] == 'p') {
            offsets.push_back(i);
        }
    }
    return offsets;
}

bool VAGDecoder::Decode(const std::vector<uint8_t>& data, std::vector<uint8_t>& outWav) {
    if (data.size() < 48) return false;
    
    // Verify Magic
    if (memcmp(data.data(), "VAGp", 4) != 0) return false;

    VAGHeader h;
    memcpy(&h, data.data(), sizeof(h));
    
    uint32_t sampleRate = Swap32(h.sampleRate);
    uint32_t dataSize = Swap32(h.dataSize);

    // Sanity checks
    if (sampleRate == 0) sampleRate = 44100;
    
    // Bounds check the payload
    size_t actualPayload = data.size() - 48;
    if (dataSize > actualPayload || dataSize == 0) {
        dataSize = (uint32_t)actualPayload;
    }

    return DecodeInternal(data.data() + 48, dataSize, outWav, sampleRate);
}

bool VAGDecoder::DecodeRaw(const std::vector<uint8_t>& raw, std::vector<uint8_t>& outWav, int sampleRate) {
    return DecodeInternal(raw.data(), raw.size(), outWav, sampleRate);
}

bool VAGDecoder::DecodeInternal(const uint8_t* pData, size_t size, std::vector<uint8_t>& outWav, int sampleRate) {
    if (size == 0) return false;

    std::vector<int16_t> pcm;
    pcm.reserve(size * 4); // Compressed is 4-bit per sample, expanded is 16-bit (4x size bytes roughly)

    double h1 = 0.0, h2 = 0.0;
    
    // Loop through blocks of 16 bytes
    // Byte 0: Shift/Predict, Byte 1: Flags, Bytes 2-15: Data
    for (size_t i = 0; i + 16 <= size; i += 16) {
        uint8_t predict = (pData[i] >> 4) & 0xF;
        uint8_t shift   = pData[i] & 0xF;
        uint8_t flags   = pData[i+1];

        // 7 = Loop End/Stop
        if (flags == 7) {
             // We generally keep processing the last block, but could break after
        }

        const uint8_t* block = &pData[i+2];
        for (int s = 0; s < 28; s++) {
            // Unpack nibbles (low nibble first on PS2 usually)
            int byteIndex = s / 2;
            int nibble = (s % 2 == 0) ? (block[byteIndex] & 0xF) : ((block[byteIndex] >> 4) & 0xF);
            
            // Expand 4-bit to 16-bit (signed)
            int sample = nibble << 12;
            if (sample & 0x8000) sample |= 0xFFFF0000;
            
            double fSample = (sample >> shift) + h1 * VagLut[predict % 5][0] + h2 * VagLut[predict % 5][1];
            h2 = h1;
            h1 = fSample;

            // Clamp
            int val = (int)fSample;
            if (val > 32767) val = 32767;
            if (val < -32768) val = -32768;
            pcm.push_back((int16_t)val);
        }
        
        // Safety break if we find a "Stop" flag combined with no data left, usually
    }

    if (pcm.empty()) return false;

    // Write WAV header
    size_t pcmBytes = pcm.size() * sizeof(int16_t);
    uint32_t totalLen = 36 + (uint32_t)pcmBytes;
    uint32_t fmtRate = sampleRate;
    uint32_t byteRate = sampleRate * 2; // Mono 16-bit
    uint16_t blockAlign = 2;
    uint16_t bits = 16;
    
    outWav.resize(44 + pcmBytes);
    uint8_t* w = outWav.data();
    
    // RIFF
    memcpy(w, "RIFF", 4); 
    memcpy(w+4, &totalLen, 4); 
    memcpy(w+8, "WAVEfmt ", 8);
    
    // Chunk size 16, PCM (1), Mono (1)
    uint32_t chunk16 = 16; uint16_t pcm1 = 1; uint16_t ch1 = 1;
    memcpy(w+16, &chunk16, 4); memcpy(w+20, &pcm1, 2); memcpy(w+22, &ch1, 2);
    memcpy(w+24, &fmtRate, 4); memcpy(w+28, &byteRate, 4); memcpy(w+32, &blockAlign, 2); memcpy(w+34, &bits, 2);
    
    // Data Chunk
    memcpy(w+36, "data", 4); memcpy(w+40, &pcmBytes, 4);
    
    // Samples
    memcpy(w+44, pcm.data(), pcmBytes);
    
    return true;
}