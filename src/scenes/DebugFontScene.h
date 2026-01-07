#pragma once
#include "Scene.h"
#include <vector>
#include "../Renderer.h"

class DebugFontScene : public Scene {
public:
    void OnEnter() override;
    void OnExit() override;
    void HandleInput(const SDL_Event& event) override;
    void Update(double dt) override;
    void Render(Renderer& renderer) override;

private:
    float targetBankIndex = 0.0f;
    float currentBankIndex = 0.0f; 
    int maxBanks = 0;
    int pageIndex = 0;
    
    // Flag para inicialização Lazy
    bool texturesLoaded = false;

    std::vector<Texture> debugTextures; 
    void DrawBank(Renderer& renderer, int bankIndex, float yOffset, float alpha);
};