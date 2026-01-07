#pragma once
#include "Scene.h"

// ============================================================================
// BootScene - PlayStation 2 logo boot animation (State 0)
// Handler: sub_202AB0
// ============================================================================
class BootScene : public Scene {
public:
    void OnEnter() override;
    void OnExit() override;
    void Update(double dt) override;
    void Render(Renderer& renderer) override;

private:
    // Managed by static variables in .cpp
    // (kept minimal here for clean interface)
};
