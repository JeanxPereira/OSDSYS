#pragma once
#include "Scene.h"

// ============================================================================
// SCELogoScene - Sony Computer Entertainment logo (Pre-boot)
// First screen shown on PS2 boot sequence
// ============================================================================
class SCELogoScene : public Scene {
public:
    void OnEnter() override;
    void OnExit() override;
    void Update(double dt) override;
    void Render(Renderer& renderer) override;
};
