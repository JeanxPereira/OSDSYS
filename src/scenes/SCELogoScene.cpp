#include "Platform.h"
#include "SCELogoScene.h"
#include "../Core.h"
#include "../Renderer.h"
#include "../MathTypes.h"

// ============================================================================
// SCELogoScene - Sony Computer Entertainment Logo
// 
// The first screen shown during PS2 boot:
// - Dark background fades in
// - "Sony Computer Entertainment" text appears
// - Fades to black before boot animation
// ============================================================================

static float sceneAlpha = 0.0f;
static float textAlpha = 0.0f;

// Animation timing (in seconds)
static const float FADE_IN_START = 0.0f;
static const float FADE_IN_END = 0.5f;
static const float TEXT_HOLD_START = 0.5f;
static const float TEXT_HOLD_END = 2.5f;
static const float FADE_OUT_START = 2.5f;
static const float FADE_OUT_END = 3.0f;
static const float TRANSITION_TIME = 3.2f;

void SCELogoScene::OnEnter() {
    printf("=== [SCELogoScene] OnEnter (Pre-boot) ===\n");
    time = 0.0f;
    sceneAlpha = 0.0f;
    textAlpha = 0.0f;
}

void SCELogoScene::OnExit() {
    printf("=== [SCELogoScene] OnExit ===\n");
}

void SCELogoScene::Update(double dt) {
    time += (float)dt;
    float t = time;

    // Scene fade in
    if (t < FADE_IN_END) {
        sceneAlpha = Easing::EaseOutQuad(t / FADE_IN_END);
    } else if (t > FADE_OUT_START) {
        sceneAlpha = 1.0f - Easing::EaseInQuad((t - FADE_OUT_START) / (FADE_OUT_END - FADE_OUT_START));
    } else {
        sceneAlpha = 1.0f;
    }

    // Text alpha
    if (t < TEXT_HOLD_START + 0.3f) {
        textAlpha = Easing::EaseOutQuad((t - FADE_IN_START) / 0.5f);
    } else if (t > FADE_OUT_START - 0.2f) {
        textAlpha = 1.0f - Easing::EaseInQuad((t - (FADE_OUT_START - 0.2f)) / 0.5f);
    } else {
        textAlpha = 1.0f;
    }

    // Clamp values
    if (sceneAlpha < 0.0f) sceneAlpha = 0.0f;
    if (sceneAlpha > 1.0f) sceneAlpha = 1.0f;
    if (textAlpha < 0.0f) textAlpha = 0.0f;
    if (textAlpha > 1.0f) textAlpha = 1.0f;

    // Transition to boot scene
    if (t >= TRANSITION_TIME && requestedNextState == -1) {
        printf("[SCELogoScene] Transitioning to Boot...\n");
        requestedNextState = (int)State::Boot;
    }
}

void SCELogoScene::Render(Renderer& renderer) {
    // Black background
    renderer.DisableFog();
    
    // Draw SCE logo text (centered)
    // PS2 resolution: 640x448
    const char* line1 = "Sony Computer Entertainment";
    
    // Calculate text width for centering
    float scale = 1.0f;
    float textWidth = renderer.GetTextWidth(line1, scale);
    float x = (640.0f - textWidth) / 2.0f;
    float y = 200.0f;
    
    // Golden/amber color like original PS2
    Color textColor(0.9f, 0.75f, 0.3f, textAlpha * sceneAlpha);
    renderer.DrawText(line1, x, y, textColor, scale);
    
    // Optional: Draw a subtle horizontal line under the text
    if (textAlpha > 0.5f) {
        float lineAlpha = (textAlpha - 0.5f) * 2.0f * sceneAlpha;
        Color lineColor(0.8f, 0.65f, 0.2f, lineAlpha * 0.5f);
        float lineWidth = 200.0f;
        float lineX = (640.0f - lineWidth) / 2.0f;
        renderer.DrawRect(lineX, y + 30.0f, lineWidth, 1.0f, lineColor);
    }
    
    // Copyright text at bottom
    const char* copyright = "(C) 2000 Sony Computer Entertainment Inc.";
    float copyScale = 0.6f;
    float copyWidth = renderer.GetTextWidth(copyright, copyScale);
    Color copyColor(0.5f, 0.5f, 0.5f, textAlpha * sceneAlpha * 0.7f);
    renderer.DrawText(copyright, (640.0f - copyWidth) / 2.0f, 400.0f, copyColor, copyScale);
}
