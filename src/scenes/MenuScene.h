#pragma once
#include "Scene.h"

// ============================================================================
// MenuScene - Main menu (State 1)
// Handler: sub_24F3E0
// ============================================================================
class MenuScene : public Scene {
public:
    void OnEnter() override;
    void OnExit() override;
    void HandleInput(const SDL_Event& event) override;
    void Update(double dt) override;
    void Render(Renderer& renderer) override;

private:
    // Managed by static variables in .cpp
};
