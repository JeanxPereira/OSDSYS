#pragma once
#include "Scene.h"
#include <string>
#include <vector>
#include "../TextureLoader.h"
#include "../Renderer.h"

class DebugTextureScene : public Scene {
public:
    void OnEnter() override;
    void OnExit() override;
    void HandleInput(const SDL_Event& event) override;
    void Update(double dt) override;
    void Render(Renderer& renderer) override;

private:
    TextureLoader loader;
    std::vector<std::string> textureFiles;
    int selectedIndex = 0;
    
    // Textura selecionada
    Texture currentTexture;
    std::string loadedName;
    TexData texInfo;
    
    // View state
    float zoom = 1.0f;
    bool showAlphaChecker = true;
    
    void LoadSelected();
    void DrawCheckerboard(Renderer& r, float x, float y, float w, float h);
};