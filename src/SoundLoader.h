#pragma once
#include <string>
#include <vector>
#include <map>
#include "Platform.h" // SDL Includes

#if defined(_WIN32)
    #include <SDL2/SDL_mixer.h>
#else
    #include <SDL2/SDL_mixer.h>
#endif

class SoundLoader {
public:
    SoundLoader();
    ~SoundLoader();

    bool Init();
    void Shutdown();
    bool LoadSystemSounds(const std::string& directory);
    void Play(const std::string& name, int channel = -1, int loops = 0);
    bool IsLoaded(const std::string& name) const;
    const std::vector<std::string>& GetSoundList() const { return soundNames; }

private:
    std::map<std::string, Mix_Chunk*> soundBank;
    std::vector<std::string> soundNames;
    bool initialized = false;

    // Load file from disk
    bool LoadFile(const std::string& name, const std::string& fullPath);

    // Helpers
    bool LoadFromMemory(const std::string& key, const std::vector<uint8_t>& data, bool forceVAG);
    
    // Internal method to decode and register raw ADPCM chunks
    bool RawToChunk(const std::string& key, const std::vector<uint8_t>& data);
    
    void RegisterChunk(const std::string& name, Mix_Chunk* chunk);
};