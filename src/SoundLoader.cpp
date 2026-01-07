#include "SoundLoader.h"
#include "VAGDecoder.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm> // for transform to upper

SoundLoader::SoundLoader() : initialized(false) {}
SoundLoader::~SoundLoader() { Shutdown(); }

bool SoundLoader::Init() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        printf("[SoundLoader] ERROR SDL_InitSubSystem: %s\n", SDL_GetError());
        return false;
    }
    // Attempt standard CD audio quality
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("[SoundLoader] Mix_OpenAudio Failed: %s\n", Mix_GetError());
        return false;
    }
    Mix_AllocateChannels(32);
    initialized = true;
    return true;
}

void SoundLoader::Shutdown() {
    for (auto& pair : soundBank) {
        if (pair.second) Mix_FreeChunk(pair.second);
    }
    soundBank.clear();
    soundNames.clear();
    if (initialized) {
        Mix_CloseAudio();
        initialized = false;
    }
}

// --------------------------------------------------------------------------------------
// Load Strategy: 
// 1. Scan directory for common names (SND*).
// 2. Scan file content for special OSDSYS split patterns (07 77 77...) or VAGp headers.
// --------------------------------------------------------------------------------------
bool SoundLoader::LoadSystemSounds(const std::string& dir) {
    if (!initialized) Init();
    
    // Normalize path
    std::string directory = dir;
    if (!directory.empty() && directory.back() != '/' && directory.back() != '\\') directory += '/';

    printf("[SoundLoader] Scanning for audio in: %s\n", directory.c_str());

    const char* targets[] = {
        "SNDBOOTB", "SNDBOOTH", "SNDBOOTS", "SNDCLOKS", "SNDLOGOS",
        "SNDOSDDB", "SNDOSDDH", "SNDRCLKS", "SNDTM30S", "SNDTM60S",
        "SNDTNNLS", "SNDWARNS"
    };

    int loadedCount = 0;
    
    for (const char* t : targets) {
        std::string base = directory + t;
        // Try extension variants
        if (LoadFile(t, base)) loadedCount++;
        else if (LoadFile(t, base + ".bin")) loadedCount++;
        else if (LoadFile(t, base + ".BIN")) loadedCount++;
        else if (LoadFile(t, base + ".wav")) loadedCount++;
    }

    printf("[SoundLoader] System Load Complete. Files found: %d\n", loadedCount);
    return loadedCount > 0;
}

bool SoundLoader::LoadFile(const std::string& name, const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (size < 32) return false; 

    std::vector<uint8_t> buffer(size);
    if (!file.read((char*)buffer.data(), size)) return false;
    file.close();

    // Check 1: VAGp Headers (Standard)
    if (VAGDecoder::ScanForHeaders(buffer).size() > 0) {
        printf("[SoundLoader] %s appears to be a standard VAGp container.\n", name.c_str());
        std::vector<size_t> vagOffsets = VAGDecoder::ScanForHeaders(buffer);
        for (size_t i = 0; i < vagOffsets.size(); i++) {
            size_t start = vagOffsets[i];
            size_t end = (i + 1 < vagOffsets.size()) ? vagOffsets[i+1] : buffer.size();
            std::vector<uint8_t> subBuf(buffer.begin() + start, buffer.begin() + end);
            
            // Register SNDBOOTS_0, SNDBOOTS_1, etc.
            std::string subName = name + "_" + std::to_string(i);
            if (LoadFromMemory(subName, subBuf, true)) {
                // If it's the first one, also register as base name
                if (i == 0) LoadFromMemory(name, subBuf, true);
            }
        }
        return true;
    }

    // Check 2: Raw Archive Splitting (The 07 77 77... signature)
    // OSDSYS sound blobs are often raw streams concatenated with padding blocks
    // Pattern seems to be: [Data...] then padding ending with blocks starting 0x07 or filled with 0x77
    bool isSplit = false;
    size_t streamStart = 0;
    int index = 0;

    for (size_t i = 0; i < buffer.size() - 16; i += 16) {
        // Detect STOP BLOCK pattern seen in dumps: "07 77 77..."
        // Or 16 bytes of "00...00" followed by start of new data?
        
        bool isDelimiter = false;
        // Looking for explicit stop flag `07 77 ...` at start of a block
        if (buffer[i] == 0x07 && buffer[i+1] == 0x77 && buffer[i+2] == 0x77) {
            isDelimiter = true;
        } 
        // Some dumps use 00 07 77...
        else if (buffer[i] == 0x00 && buffer[i+1] == 0x07 && buffer[i+2] == 0x77) {
            isDelimiter = true;
        }

        if (isDelimiter) {
            size_t streamEnd = i + 16; // Include the delimiter block as "tail" for decoder silence
            if (streamEnd > buffer.size()) streamEnd = buffer.size();

            size_t len = streamEnd - streamStart;
            // Only save if significant size
            if (len > 128) {
                // We have a chunk from streamStart to streamEnd
                std::vector<uint8_t> rawSlice(buffer.begin() + streamStart, buffer.begin() + streamEnd);
                std::string subName = name + "_" + std::to_string(index);
                
                // Attempt raw decode (assume 44100Hz)
                if (RawToChunk(subName, rawSlice)) {
                    // Alias first one
                    if (index == 0) RawToChunk(name, rawSlice);
                    index++;
                    isSplit = true;
                }
            }

            size_t nextData = streamEnd;
            while (nextData + 16 < buffer.size()) {
                if (buffer[nextData] == 0 && buffer[nextData+1] == 0) {
                    nextData += 16;
                } else if (buffer[nextData] == 0x77 && buffer[nextData+1] == 0x77) {
                    nextData += 16;
                } else {
                    break;
                }
            }
            streamStart = nextData;
            i = nextData - 16;
        }
    }

    if (!isSplit && buffer.size() > 128) {
        if (RawToChunk(name, buffer)) {
            // Also alias _0
            RawToChunk(name + "_0", buffer); 
            return true;
        }
    } else if (isSplit) {
        printf("[SoundLoader] Successfully split '%s' into %d streams using markers.\n", name.c_str(), index);
        return true;
    }

    return false;
}

// Decodes raw bytes to chunk and registers
bool SoundLoader::RawToChunk(const std::string& key, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> wav;
    // Default PS2 sample rate for these blobs
    if (VAGDecoder::DecodeRaw(data, wav, 44100)) {
        Mix_Chunk* c = Mix_LoadWAV_RW(SDL_RWFromConstMem(wav.data(), (int)wav.size()), 1);
        if (c) {
            RegisterChunk(key, c);
            return true;
        }
    }
    return false;
}

bool SoundLoader::LoadFromMemory(const std::string& key, const std::vector<uint8_t>& data, bool forceVAG) {
    // VAGp wrapper
    std::vector<uint8_t> wav;
    if (forceVAG && VAGDecoder::Decode(data, wav)) {
        Mix_Chunk* c = Mix_LoadWAV_RW(SDL_RWFromConstMem(wav.data(), (int)wav.size()), 1);
        if (c) {
            RegisterChunk(key, c);
            return true;
        }
    }
    return false;
}

void SoundLoader::RegisterChunk(const std::string& name, Mix_Chunk* chunk) {
    if (soundBank.find(name) != soundBank.end()) {
        Mix_FreeChunk(soundBank[name]);
    }
    soundBank[name] = chunk;

    // Check duplicates for list
    bool found = false;
    for (const auto& s : soundNames) if(s == name) found = true;
    if (!found) soundNames.push_back(name);
}

void SoundLoader::Play(const std::string& name, int channel, int loops) {
    if (!initialized) return;
    auto it = soundBank.find(name);
    
    // Play requested
    if (it != soundBank.end()) {
        Mix_PlayChannel(channel, it->second, loops);
        return;
    } 
    
    // Fallback: If "NAME" requested but we only have "NAME_0", play index 0
    std::string fallback = name + "_0";
    auto it2 = soundBank.find(fallback);
    if (it2 != soundBank.end()) {
        Mix_PlayChannel(channel, it2->second, loops);
    }
}

bool SoundLoader::IsLoaded(const std::string& name) const {
    return soundBank.find(name) != soundBank.end();
}