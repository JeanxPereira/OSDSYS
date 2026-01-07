#include "Platform.h"
#include "MenuScene.h"
#include "../Core.h"
#include "../Renderer.h"
#include "../MathTypes.h"
#include "../Assets.h"

// ============================================================================
// MenuScene - Main menu (State 1)
// Handler: sub_24F3E0
// 
// The main OSDSYS menu with:
// - Central glowing orb
// - Menu items (Browser, System Configuration, Version)
// - Animated background
// - Selection highlights
// ============================================================================

struct MenuItem {
    std::string name;
    Vec3 position;
    Vec3 targetPosition;
    float scale;
    float targetScale;
    bool selected;
    Color color;
};

struct BackgroundOrb {
    Vec3 position;
    float radius;
    float glowIntensity;
    float pulsePhase;
    Color baseColor;
};

struct FloatingParticle {
    Vec3 position;
    Vec3 velocity;
    float alpha;
    float size;
    float lifetime;
    float age;
};

// Static scene data
static std::vector<MenuItem> menuItems;
static int selectedIndex = 0;
static BackgroundOrb orb;
static std::vector<FloatingParticle> particles;
static float sceneAlpha = 0.0f;
static ICOBModel orbMesh;
static bool orbMeshLoaded = false;

// Menu item definitions
static const char* MENU_ITEMS[] = {
    "Browser",
    "System Configuration", 
    "Version Information"
};
static const int MENU_ITEM_COUNT = 3;

void MenuScene::OnEnter() {
    printf("=== [MenuScene] OnEnter (State 1, Handler: sub_24F3E0) ===\n");
    time = 0.0f;
    selectedIndex = 0;
    sceneAlpha = 0.0f;

    // Initialize menu items
    menuItems.clear();
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        MenuItem item;
        item.name = MENU_ITEMS[i];
        item.targetPosition = Vec3(-150.0f, 60.0f - i * 50.0f, 0.0f);
        item.position = Vec3(-200.0f, 60.0f - i * 50.0f, 0.0f); // Start offset
        item.scale = 1.0f;
        item.targetScale = 1.0f;
        item.selected = (i == 0);
        item.color = Color(0.7f, 0.7f, 0.8f, 1.0f);
        menuItems.push_back(item);
    }

    // Initialize central orb
    orb.position = Vec3(100.0f, 0.0f, -50.0f);
    orb.radius = 40.0f;
    orb.glowIntensity = 1.0f;
    orb.pulsePhase = 0.0f;
    orb.baseColor = Color(0.2f, 0.4f, 0.9f, 1.0f);

    // Initialize floating particles
    particles.clear();
    const int PARTICLE_COUNT = 40;
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        FloatingParticle p;
        p.position = Vec3(
            ((float)rand() / RAND_MAX - 0.5f) * 500.0f,
            ((float)rand() / RAND_MAX - 0.5f) * 350.0f,
            -200.0f + ((float)rand() / RAND_MAX) * 100.0f
        );
        p.velocity = Vec3(
            ((float)rand() / RAND_MAX - 0.5f) * 10.0f,
            ((float)rand() / RAND_MAX - 0.5f) * 10.0f + 5.0f, // Slight upward drift
            ((float)rand() / RAND_MAX - 0.5f) * 5.0f
        );
        p.alpha = ((float)rand() / RAND_MAX) * 0.3f + 0.1f;
        p.size = ((float)rand() / RAND_MAX) * 3.0f + 1.0f;
        p.lifetime = ((float)rand() / RAND_MAX) * 10.0f + 5.0f;
        p.age = ((float)rand() / RAND_MAX) * p.lifetime; // Random start phase
        particles.push_back(p);
    }

    // Try to load orb mesh (ICOBYSYS or similar)
    AssetLoader assetLoader;
    ICOBModel model;
    if (assetLoader.LoadICOB("ICOBYSYS", model)) {
        orbMesh = model;
        orbMeshLoaded = true;
        printf("[MenuScene] Orb mesh loaded: %zu vertices\n", orbMesh.vertices.size());
    } else {
        orbMeshLoaded = false;
    }

    printf("[MenuScene] Initialized: %zu menu items, %zu particles\n", 
           menuItems.size(), particles.size());
}

void MenuScene::OnExit() {
    printf("=== [MenuScene] OnExit ===\n");
}

void MenuScene::HandleInput(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_UP:
            case SDLK_w:
                if (selectedIndex > 0) {
                    menuItems[selectedIndex].selected = false;
                    selectedIndex--;
                    menuItems[selectedIndex].selected = true;
                    printf("[MenuScene] Selected: %s\n", menuItems[selectedIndex].name.c_str());
                }
                break;

            case SDLK_DOWN:
            case SDLK_s:
                if (selectedIndex < (int)menuItems.size() - 1) {
                    menuItems[selectedIndex].selected = false;
                    selectedIndex++;
                    menuItems[selectedIndex].selected = true;
                    printf("[MenuScene] Selected: %s\n", menuItems[selectedIndex].name.c_str());
                }
                break;

            case SDLK_RETURN:
            case SDLK_SPACE:
            case SDLK_x: // X button on PS2
                printf("[MenuScene] Activated: %s\n", menuItems[selectedIndex].name.c_str());
                
                // Handle menu actions
                if (selectedIndex == 0) {
                    // Browser
                    requestedNextState = (int)State::Browser;
                }
                // TODO: System Configuration and Version Info
                break;
        }
    }
}

void MenuScene::Update(double dt) {
    time += (float)dt;
    float t = (float)time;

    // Scene fade in
    const float FADE_IN_TIME = 0.5f;
    if (t < FADE_IN_TIME) {
        sceneAlpha = Easing::EaseOutQuad(t / FADE_IN_TIME);
    } else {
        sceneAlpha = 1.0f;
    }

    // Update menu items
    for (size_t i = 0; i < menuItems.size(); i++) {
        auto& item = menuItems[i];
        
        // Target scale based on selection
        item.targetScale = item.selected ? 1.3f : 1.0f;
        item.scale = Math::Lerp(item.scale, item.targetScale, (float)dt * 10.0f);
        
        // Smooth position movement
        item.position.x = Math::Lerp(item.position.x, item.targetPosition.x, (float)dt * 5.0f);
        item.position.y = Math::Lerp(item.position.y, item.targetPosition.y, (float)dt * 5.0f);
        
        // Color based on selection
        if (item.selected) {
            item.color.r = Math::Lerp(item.color.r, 1.0f, (float)dt * 8.0f);
            item.color.g = Math::Lerp(item.color.g, 1.0f, (float)dt * 8.0f);
            item.color.b = Math::Lerp(item.color.b, 1.0f, (float)dt * 8.0f);
        } else {
            item.color.r = Math::Lerp(item.color.r, 0.5f, (float)dt * 5.0f);
            item.color.g = Math::Lerp(item.color.g, 0.5f, (float)dt * 5.0f);
            item.color.b = Math::Lerp(item.color.b, 0.6f, (float)dt * 5.0f);
        }
    }

    // Update orb
    orb.pulsePhase += (float)dt * Math::TWO_PI * 0.3f;
    orb.glowIntensity = 0.7f + 0.3f * sinf(orb.pulsePhase);
    
    // Orb subtly moves based on selection
    float targetY = 60.0f - selectedIndex * 50.0f;
    orb.position.y = Math::Lerp(orb.position.y, targetY * 0.3f, (float)dt * 2.0f);

    // Update floating particles
    for (auto& p : particles) {
        p.position = p.position + p.velocity * (float)dt;
        p.age += (float)dt;
        
        // Respawn if too old or out of bounds
        if (p.age > p.lifetime || 
            fabs(p.position.x) > 300.0f || 
            fabs(p.position.y) > 200.0f) {
            p.position = Vec3(
                ((float)rand() / RAND_MAX - 0.5f) * 500.0f,
                -180.0f, // Start from bottom
                -200.0f + ((float)rand() / RAND_MAX) * 100.0f
            );
            p.age = 0.0f;
        }
        
        // Fade based on age
        float lifeRatio = p.age / p.lifetime;
        if (lifeRatio < 0.2f) {
            p.alpha = Easing::EaseOutQuad(lifeRatio / 0.2f) * 0.4f;
        } else if (lifeRatio > 0.8f) {
            p.alpha = (1.0f - (lifeRatio - 0.8f) / 0.2f) * 0.4f;
        } else {
            p.alpha = 0.4f;
        }
    }

    // Debug logging
    static int lastLog = -1;
    int currentLog = (int)(t / 2.0f);
    if (currentLog != lastLog) {
        lastLog = currentLog;
        printf("[MenuScene] t=%.1fs, selected=%d (%s)\n", 
               t, selectedIndex, menuItems[selectedIndex].name.c_str());
    }
}

void MenuScene::Render(Renderer& renderer) {
    // Set fog
    renderer.SetFog(0.02f, Vec3(0.05f, 0.05f, 0.1f));

    // Draw floating particles
    for (const auto& p : particles) {
        if (p.alpha > 0.01f) {
            Color particleColor(0.4f, 0.5f, 0.8f, p.alpha * sceneAlpha);
            renderer.DrawSphere(p.position, p.size, particleColor, 4);
        }
    }

    // Draw central orb
    Color orbColor = orb.baseColor;
    orbColor.a = orb.glowIntensity * sceneAlpha;
    
    if (orbMeshLoaded) {
        Vec3 orbRot(time * 0.2f, time * 0.3f, 0.0f);
        renderer.DrawMesh(orbMesh, orb.position, Vec3(orb.radius * 0.5f), orbColor, orbRot);
    } else {
        renderer.DrawSphere(orb.position, orb.radius, orbColor, 16);
    }
    
    // Orb glow
    renderer.SetBlendMode(true);
    Color glowColor(0.3f, 0.5f, 0.9f, orb.glowIntensity * 0.3f * sceneAlpha);
    renderer.DrawSphere(orb.position, orb.radius * 1.5f, glowColor, 12);
    renderer.SetBlendMode(false);

    // Draw menu items (placeholder rectangles until text rendering is ready)
    for (size_t i = 0; i < menuItems.size(); i++) {
        const auto& item = menuItems[i];
        
        // Calculate screen position (approximate)
        float screenX = 50.0f + item.position.x * 0.3f;
        float screenY = 200.0f - item.position.y * 1.5f;
        float width = 200.0f * item.scale;
        float height = 24.0f * item.scale;
        
        // Selection highlight background
        if (item.selected) {
            Color highlightColor(0.2f, 0.3f, 0.6f, 0.5f * sceneAlpha);
            renderer.DrawRect(screenX - 10.0f, screenY - 4.0f, width + 20.0f, height + 8.0f, highlightColor);
        }
        
        // Text placeholder
        Color textColor = item.color;
        textColor.a *= sceneAlpha;
        renderer.DrawText(item.name.c_str(), screenX, screenY, textColor, item.scale);
        
        // Selection indicator (arrow or cursor)
        if (item.selected) {
            float cursorPulse = 0.7f + 0.3f * sinf(time * 5.0f);
            Color cursorColor(1.0f, 1.0f, 1.0f, cursorPulse * sceneAlpha);
            renderer.DrawRect(screenX - 20.0f, screenY + 4.0f, 10.0f, 10.0f, cursorColor);
        }
    }

    // Draw navigation hints at bottom
    Color hintColor(0.5f, 0.5f, 0.6f, 0.7f * sceneAlpha);
    renderer.DrawText("UP/DOWN: Select   ENTER: Confirm", 180.0f, 420.0f, hintColor, 0.8f);
}
