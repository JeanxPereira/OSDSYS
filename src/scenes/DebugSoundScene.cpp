#include "Platform.h"
#include "DebugSoundScene.h"
#include "../Renderer.h"
#include "../Core.h"

// ============================================================================
// DebugSoundScene Implementation
// ============================================================================

void DebugSoundScene::OnEnter() {
    printf("[DebugSoundScene] Initializing SoundLoader...\n");
    soundLoader.Init();
    
    // Attempt to load sounds from multiple possible asset paths
    bool soundsLoaded = soundLoader.LoadSystemSounds("assets/audio/");
    if (!soundsLoaded) {
        soundsLoaded = soundLoader.LoadSystemSounds("assets/sounds/");
    }
    if (!soundsLoaded) {
        // Fallback for flat structure
        soundsLoaded = soundLoader.LoadSystemSounds("assets/");
    }

    if (soundsLoaded) {
        printf("[DebugSoundScene] Sounds loaded. Total: %zu\n", soundLoader.GetSoundList().size());
    } else {
        printf("[DebugSoundScene] WARNING: No sound files found (assets/audio/SND*.bin).\n");
    }

    selectedIndex = 0;
    playFlash = 0.0f;
}

void DebugSoundScene::OnExit() {
    soundLoader.Shutdown();
    printf("[DebugSoundScene] Shutdown SoundLoader.\n");
}

void DebugSoundScene::HandleInput(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        // Return to Menu
        if (event.key.keysym.sym == SDLK_ESCAPE || 
            event.key.keysym.sym == SDLK_BACKSPACE || 
            event.key.keysym.sym == SDLK_F2) {
            requestedNextState = (int)State::Menu;
            return;
        }

        const auto& soundList = soundLoader.GetSoundList();
        if (soundList.empty()) return;

        // Navigation (Up/Down)
        if (event.key.keysym.sym == SDLK_UP) {
            selectedIndex--;
            if (selectedIndex < 0) selectedIndex = 0;
        }
        else if (event.key.keysym.sym == SDLK_DOWN) {
            selectedIndex++;
            if (selectedIndex >= (int)soundList.size()) selectedIndex = (int)soundList.size() - 1;
        }
        // Play (Enter, Space, or X button)
        else if (event.key.keysym.sym == SDLK_RETURN || 
                 event.key.keysym.sym == SDLK_SPACE || 
                 event.key.keysym.sym == SDLK_x) {
            
            if (selectedIndex >= 0 && selectedIndex < (int)soundList.size()) {
                std::string sound = soundList[selectedIndex];
                soundLoader.Play(sound);
                
                // Visual feedback
                lastPlayed = sound;
                playFlash = 1.0f;
            }
        }
    }
}

void DebugSoundScene::Update(double dt) {
    // Fade out the green playback flash
    if (playFlash > 0.0f) {
        playFlash -= (float)dt * 5.0f;
        if (playFlash < 0.0f) playFlash = 0.0f;
    }
}

void DebugSoundScene::Render(Renderer& renderer) {
    renderer.DisableFog();
    
    // Technical dark background
    renderer.DrawRect(0, 0, 640, 448, Color(0.1f, 0.12f, 0.15f, 1.0f));

    // Header
    renderer.DrawText("Debug Sound Player", 30.0f, 25.0f, Color(0.4f, 0.9f, 1.0f, 1.0f), 1.0f);
    renderer.DrawRect(30.0f, 45.0f, 580.0f, 1.0f, Color(0.4f, 0.9f, 1.0f, 0.5f)); 
    
    const auto& soundList = soundLoader.GetSoundList();
    if (soundList.empty()) {
        renderer.DrawText("ERROR: No sounds loaded!", 30.0f, 80.0f, Color(1.0f, 0.3f, 0.3f, 1.0f), 1.0f);
        renderer.DrawText("Check 'assets/audio/' for .bin/.vag files.", 30.0f, 110.0f, Color(0.7f, 0.7f, 0.7f, 1.0f), 0.8f);
        return;
    }

    // Scroll Logic: Center the selection
    float lineHeight = 25.0f;
    float startY = 60.0f;
    int displayLines = 12; // Lines per page
    
    int startIdx = 0;
    if (selectedIndex >= displayLines / 2) {
        startIdx = selectedIndex - (displayLines / 2);
    }
    if (startIdx + displayLines > (int)soundList.size()) {
        startIdx = (int)soundList.size() - displayLines;
    }
    if (startIdx < 0) startIdx = 0;
    
    // Render list
    for (int i = 0; i < displayLines; i++) {
        int idx = startIdx + i;
        if (idx >= (int)soundList.size()) break;

        float y = startY + (i * lineHeight);
        bool isSelected = (idx == selectedIndex);
        
        Color textColor = isSelected ? Color(1.0f, 1.0f, 1.0f, 1.0f) : Color(0.6f, 0.6f, 0.7f, 1.0f);
        Color barColor  = Color(0.0f, 0.0f, 0.0f, 0.0f);

        if (isSelected) {
            barColor = Color(0.2f, 0.25f, 0.35f, 1.0f);
            
            // Green flash if this track is playing
            if (playFlash > 0.0f && soundList[idx] == lastPlayed) {
                float intensity = playFlash * 0.4f;
                barColor = Color(0.2f + intensity, 0.3f + intensity, 0.2f, 1.0f);
            }
        }
        
        // Draw selection bar and text
        if (isSelected) {
             renderer.DrawRect(25.0f, y, 300.0f, lineHeight, barColor);
             renderer.DrawText(">", 10.0f, y, Color(1.0f, 0.8f, 0.0f, 1.0f), 0.8f);
        }

        char buf[128];
        snprintf(buf, sizeof(buf), "%2d. %s", idx + 1, soundList[idx].c_str());
        renderer.DrawText(buf, 30.0f, y + 2.0f, textColor, 0.9f);
    }
    
    // Footer legend
    renderer.DrawRect(0.0f, 400.0f, 640.0f, 48.0f, Color(0.05f, 0.05f, 0.1f, 0.8f));
    renderer.DrawText("UP/DOWN: Navigate   X / SPACE: Play   BACKSPACE: Exit", 40.0f, 415.0f, Color(0.6f, 0.7f, 0.8f, 1.0f), 0.8f);
}