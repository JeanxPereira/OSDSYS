#pragma once
#include "Scene.h"

// ============================================================================
// BrowserScene - Memory card browser (State 3)
// Handler: sub_23FFA8
// ============================================================================
class BrowserScene : public Scene {
public:
    void OnEnter() override;
    void OnExit() override;
    void HandleInput(const SDL_Event& event) override;
    void Update(double dt) override;
    void Render(Renderer& renderer) override;

private:
    // Managed by static variables in .cpp
};
