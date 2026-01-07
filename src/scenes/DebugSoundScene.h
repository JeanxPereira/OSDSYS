#pragma once
#include "Scene.h"
#include "../SoundLoader.h"
#include <string>
#include <vector>

// DebugSoundScene - Testador de Áudio
// Use as setas para selecionar e ENTER para tocar
class DebugSoundScene : public Scene {
public:
    void OnEnter() override;
    void OnExit() override;
    void HandleInput(const SDL_Event& event) override;
    void Update(double dt) override;
    void Render(Renderer& renderer) override;

private:
    SoundLoader soundLoader;
    int selectedIndex = 0;
    
    // Feedback visual quando toca
    float playFlash = 0.0f; 
    std::string lastPlayed;
};