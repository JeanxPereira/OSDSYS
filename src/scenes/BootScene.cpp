#include "Platform.h"
#include "BootScene.h"
#include "../Core.h"
#include "../Renderer.h"
#include "../MathTypes.h"
#include "../Assets.h"

// ============================================================================
// BootScene - Boot animation (State 0)
// Handler: sub_202AB0
// 
// The iconic PS2 boot sequence with:
// - Animated light trails converging to center
// - Background particle cubes
// - PS2 logo rotation
// - Fade transitions
// ============================================================================

// Trail particle (simulating the light streaks during boot)
struct BootTrail {
    Vec3 position;
    Vec3 velocity;
    float alpha;
    float lifetime;
    Color color;
    float width;
};

// Background cube (ambient floating elements)
struct BootCube {
    Vec3 position;
    Vec3 rotation;
    Vec3 rotationSpeed;
    float scale;
    float alpha;
};

// Static scene data
static std::vector<BootTrail> trails;
static std::vector<BootCube> cubes;
static ICOBModel ps2LogoMesh;
static bool ps2LogoLoaded = false;
static Vec3 logoRotation = Vec3(0, 0, 0);
static float logoAlpha = 0.0f;
static float sceneAlpha = 0.0f;

// Animation timing constants (in seconds)
static const float PHASE_TRAILS_START = 0.0f;
static const float PHASE_TRAILS_END = 2.0f;
static const float PHASE_LOGO_START = 1.5f;
static const float PHASE_LOGO_PEAK = 3.5f;
static const float PHASE_FADE_OUT = 4.0f;
static const float PHASE_TRANSITION = 5.0f;

void BootScene::OnEnter() {
    printf("=== [BootScene] OnEnter (State 0, Handler: sub_202AB0) ===\n");
    time = 0.0f;
    sceneAlpha = 0.0f;
    logoAlpha = 0.0f;
    logoRotation = Vec3(0, 0, 0);
    
    // Create animated trails (light streaks converging to center)
    trails.clear();
    const int TRAIL_COUNT = 30;
    for (int i = 0; i < TRAIL_COUNT; i++) {
        BootTrail trail;
        
        // Random starting position in a circle around center
        float angle = (float)i / TRAIL_COUNT * Math::TWO_PI;
        float radius = 250.0f + (i % 5) * 30.0f;
        
        trail.position = Vec3(
            cosf(angle) * radius,
            sinf(angle) * radius * 0.5f + (i % 3 - 1) * 20.0f,
            -50.0f + (i % 7) * 10.0f
        );
        
        // Velocity towards center with slight variation
        Vec3 toCenter = Vec3(0, 0, 0) - trail.position;
        float dist = toCenter.Length();
        trail.velocity = toCenter.Normalize() * (150.0f + (i % 10) * 10.0f);
        
        trail.alpha = 0.0f;
        trail.lifetime = dist / trail.velocity.Length();
        
        // Color gradient: blue → cyan → purple
        float hue = (float)i / TRAIL_COUNT;
        if (hue < 0.5f) {
            trail.color = Color(0.2f + hue * 0.3f, 0.4f + hue * 0.6f, 0.9f, 1.0f);
        } else {
            trail.color = Color(0.3f + (hue - 0.5f) * 0.5f, 0.7f - (hue - 0.5f) * 0.4f, 0.9f, 1.0f);
        }
        
        trail.width = 2.0f + (i % 3);
        
        trails.push_back(trail);
    }

    // Create background cubes (ambient floating elements)
    cubes.clear();
    const int CUBE_COUNT = 15;
    for (int i = 0; i < CUBE_COUNT; i++) {
        BootCube cube;
        
        // Distribute cubes in 3D space
        cube.position = Vec3(
            -150.0f + (i % 5) * 60.0f + (i % 3) * 10.0f,
            -80.0f + (i / 5) * 60.0f,
            -300.0f + (i % 4) * 50.0f
        );
        
        cube.rotation = Vec3(
            (i * 0.3f),
            (i * 0.5f),
            (i * 0.2f)
        );
        
        cube.rotationSpeed = Vec3(
            0.3f + (i % 3) * 0.15f,
            0.2f + (i % 4) * 0.1f,
            0.15f + (i % 2) * 0.1f
        );
        
        cube.scale = 8.0f + (i % 4) * 4.0f;
        cube.alpha = 0.0f;
        
        cubes.push_back(cube);
    }

    // Load PS2 logo mesh (ICOBPS2M)
    AssetLoader assetLoader;
    ICOBModel model;
    
    if (assetLoader.LoadICOB("ICOBPS2M", model)) {
        ps2LogoMesh = model;
        ps2LogoLoaded = true;
        printf("[BootScene] PS2 Logo loaded: %zu vertices, %zu indices\n", 
               ps2LogoMesh.vertices.size(), ps2LogoMesh.indices.size());
    } else {
        ps2LogoLoaded = false;
        printf("[BootScene] PS2 Logo not available (using fallback cubes)\n");
    }

    printf("[BootScene] Initialized: %zu trails, %zu cubes\n", trails.size(), cubes.size());
}

void BootScene::OnExit() {
    printf("=== [BootScene] OnExit ===\n");
    trails.clear();
    cubes.clear();
}

void BootScene::Update(double dt) {
    time += (float)dt;
    float t = (float)time;

    // Scene fade in
    if (t < 0.5f) {
        sceneAlpha = Easing::EaseOutQuad(t / 0.5f);
    } else if (t > PHASE_FADE_OUT) {
        sceneAlpha = 1.0f - Easing::EaseInQuad((t - PHASE_FADE_OUT) / (PHASE_TRANSITION - PHASE_FADE_OUT));
    } else {
        sceneAlpha = 1.0f;
    }

    // Auto-transition to Menu after animation completes
    if (t >= PHASE_TRANSITION && requestedNextState == -1) {
        printf("[BootScene] Animation complete, transitioning to Menu...\n");
        requestedNextState = (int)State::Menu;
        return;
    }

    // Update trails
    for (auto& trail : trails) {
        // Move towards center
        trail.position = trail.position + trail.velocity * (float)dt;
        
        // Update alpha based on phase
        if (t < PHASE_TRAILS_START + 0.5f) {
            trail.alpha = Easing::EaseOutQuad((t - PHASE_TRAILS_START) / 0.5f);
        } else if (t < PHASE_TRAILS_END) {
            trail.alpha = 1.0f;
        } else if (t < PHASE_TRAILS_END + 0.5f) {
            trail.alpha = 1.0f - Easing::EaseInQuad((t - PHASE_TRAILS_END) / 0.5f);
        } else {
            trail.alpha = 0.0f;
        }
        
        // Reset trail if it reaches center
        Vec3 toCenter = Vec3(0, 0, 0) - trail.position;
        if (toCenter.Length() < 20.0f) {
            // Respawn at outer edge
            float angle = ((float)rand() / RAND_MAX) * Math::TWO_PI;
            float radius = 250.0f + ((float)rand() / RAND_MAX) * 50.0f;
            trail.position = Vec3(
                cosf(angle) * radius,
                sinf(angle) * radius * 0.5f,
                -50.0f + ((float)rand() / RAND_MAX) * 70.0f
            );
            toCenter = Vec3(0, 0, 0) - trail.position;
            trail.velocity = toCenter.Normalize() * (150.0f + ((float)rand() / RAND_MAX) * 50.0f);
        }
    }

    // Update background cubes
    for (auto& cube : cubes) {
        cube.rotation = cube.rotation + cube.rotationSpeed * (float)dt;
        
        // Fade cubes
        if (t < PHASE_TRAILS_START + 1.0f) {
            cube.alpha = Easing::EaseInCubic((t - PHASE_TRAILS_START) / 1.0f) * 0.25f;
        } else if (t < PHASE_FADE_OUT) {
            cube.alpha = 0.25f;
        } else {
            cube.alpha = 0.25f * (1.0f - Easing::EaseInQuad((t - PHASE_FADE_OUT) / 1.0f));
        }
    }

    // Update logo
    if (t >= PHASE_LOGO_START) {
        // Logo fade in
        if (t < PHASE_LOGO_PEAK) {
            float logoT = (t - PHASE_LOGO_START) / (PHASE_LOGO_PEAK - PHASE_LOGO_START);
            logoAlpha = Easing::EaseOutCubic(logoT);
        } else if (t < PHASE_FADE_OUT) {
            logoAlpha = 1.0f;
        } else {
            float fadeT = (t - PHASE_FADE_OUT) / (PHASE_TRANSITION - PHASE_FADE_OUT);
            logoAlpha = 1.0f - Easing::EaseInQuad(fadeT);
        }
        
        // Slow continuous rotation (PS2 style)
        logoRotation.y += 0.4f * (float)dt;
        logoRotation.x = sinf(t * 0.3f) * 0.1f; // Slight wobble
    }

    // Debug logging (once per second)
    static int lastSecond = -1;
    int currentSecond = (int)t;
    if (currentSecond != lastSecond) {
        lastSecond = currentSecond;
        printf("[BootScene] t=%.1fs, sceneAlpha=%.2f, logoAlpha=%.2f\n", 
               t, sceneAlpha, logoAlpha);
    }
}

void BootScene::Render(Renderer& renderer) {
    // Set fog for depth effect
    renderer.SetFog(0.015f, Vec3(0.02f, 0.02f, 0.08f));

    // Draw background cubes
    for (const auto& cube : cubes) {
        if (cube.alpha > 0.01f) {
            Color cubeColor(0.1f, 0.15f, 0.3f, cube.alpha * sceneAlpha);
            renderer.DrawCube(cube.position, Vec3(cube.scale), cubeColor, cube.rotation);
        }
    }

    // Draw trails as lines
    for (const auto& trail : trails) {
        if (trail.alpha > 0.01f) {
            Color trailColor = trail.color;
            trailColor.a = trail.alpha * sceneAlpha;
            
            // Draw trail as a line from current position towards center
            Vec3 trailEnd = trail.position + trail.velocity.Normalize() * (-30.0f);
            renderer.DrawLine(trail.position, trailEnd, trailColor, trail.width);
            
            // Draw a small bright point at the head
            Color headColor(1.0f, 1.0f, 1.0f, trail.alpha * sceneAlpha * 0.8f);
            renderer.DrawSphere(trail.position, 2.0f, headColor, 6);
        }
    }

    // Draw PS2 logo
    if (ps2LogoLoaded && logoAlpha > 0.01f) {
        // Silver/white logo color with glow effect
        Color logoColor(0.85f, 0.88f, 0.95f, logoAlpha * sceneAlpha);
        Vec3 logoPos(0.0f, 0.0f, 0.0f);
        Vec3 logoScale(12.0f, 12.0f, 12.0f);
        
        renderer.DrawMesh(ps2LogoMesh, logoPos, logoScale, logoColor, logoRotation);
        
        // Optional: Draw subtle glow behind logo
        if (logoAlpha > 0.5f) {
            Color glowColor(0.3f, 0.4f, 0.8f, (logoAlpha - 0.5f) * 0.3f * sceneAlpha);
            renderer.SetBlendMode(true); // Additive
            renderer.DrawSphere(logoPos, 50.0f, glowColor, 12);
            renderer.SetBlendMode(false); // Back to normal
        }
    } else if (!ps2LogoLoaded) {
        // Fallback: show a placeholder cube
        if (logoAlpha > 0.01f) {
            Color fallbackColor(0.4f, 0.5f, 0.8f, logoAlpha * sceneAlpha);
            renderer.DrawCube(Vec3(0, 0, 0), Vec3(30.0f), fallbackColor, logoRotation);
        }
    }

    // Draw center convergence point (bright flash when trails arrive)
    float t = (float)time;
    if (t > 1.5f && t < 2.5f) {
        float flashT = (t - 1.5f) / 1.0f;
        float flashAlpha = sinf(flashT * Math::PI) * 0.5f;
        Color flashColor(0.8f, 0.9f, 1.0f, flashAlpha * sceneAlpha);
        renderer.SetBlendMode(true);
        renderer.DrawSphere(Vec3(0, 0, 0), 15.0f + flashT * 10.0f, flashColor, 8);
        renderer.SetBlendMode(false);
    }
}
