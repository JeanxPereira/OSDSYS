#include "Platform.h"
#include "BrowserScene.h"
#include "../Core.h"
#include "../Renderer.h"
#include "../MathTypes.h"
#include "../Assets.h"

// ============================================================================
// BrowserScene - Memory card browser (State 3)
// Handler: sub_23FFA8
// 
// The save data browser with:
// - 3D animated save icons (ICOB models)
// - Scrollable list
// - Selection animations
// - Memory card info display
// ============================================================================

struct SaveIcon {
    std::string name;
    std::string iconName; // ICOB asset name (e.g., "ICOBPS2M", "ICOBPS1M")
    Vec3 position;
    Vec3 targetPosition;
    Vec3 rotation;
    float scale;
    float targetScale;
    bool selected;
    int assetId;
    
    ICOBModel mesh;
    bool meshLoaded;
};

struct MemoryCardInfo {
    std::string name;
    int usedSlots;
    int totalSlots;
    bool connected;
};

// Static scene data
static std::vector<SaveIcon> saveIcons;
static int selectedIndex = 0;
static float scrollOffset = 0.0f;
static float targetScrollOffset = 0.0f;
static float sceneAlpha = 0.0f;
static MemoryCardInfo mc1, mc2;

// Available ICOB icons to use for save files
static const char* AVAILABLE_ICONS[] = {
    "ICOBPS2M",  // PS2 Memory Card
    "ICOBPS2D",  // PS2 Disc
    "ICOBPS1M",  // PS1 Memory Card
    "ICOBPS1D",  // PS1 Disc
    "ICOBDISC",  // Generic Disc
    "ICOBCDDA",  // CD Audio
    "ICOBDVDD",  // DVD
    "ICOBFNOR",  // Normal folder
    "ICOBFBRK",  // Broken folder
    "ICOBFSCE",  // SCE folder
    "ICOBYSYS",  // System
    "ICOBPKST",  // Pocket Station
    "ICOBQUES"   // Question mark
};
static const int ICON_COUNT = 13;

void BrowserScene::OnEnter() {
    printf("=== [BrowserScene] OnEnter (State 3, Handler: sub_23FFA8) ===\n");
    time = 0.0f;
    selectedIndex = 0;
    scrollOffset = 0.0f;
    targetScrollOffset = 0.0f;
    sceneAlpha = 0.0f;

    // Initialize memory card info (simulated)
    mc1.name = "Memory Card (SLOT 1)";
    mc1.usedSlots = 7;
    mc1.totalSlots = 15;
    mc1.connected = true;
    
    mc2.name = "Memory Card (SLOT 2)";
    mc2.usedSlots = 0;
    mc2.totalSlots = 15;
    mc2.connected = false;

    // Create save icons with actual ICOB models
    saveIcons.clear();
    
    const char* saveNames[] = {
        "Gran Turismo 4",
        "Final Fantasy X",
        "God of War",
        "Shadow of the Colossus",
        "Kingdom Hearts",
        "Resident Evil 4",
        "Metal Gear Solid 3",
        "Devil May Cry 3"
    };
    
    const char* saveIconNames[] = {
        "ICOBPS2D",
        "ICOBPS2D",
        "ICOBPS2D",
        "ICOBPS2D",
        "ICOBPS2D",
        "ICOBPS2D",
        "ICOBPS2D",
        "ICOBPS2D"
    };
    
    AssetLoader assetLoader;
    
    for (int i = 0; i < 8; i++) {
        SaveIcon icon;
        icon.name = saveNames[i];
        icon.iconName = saveIconNames[i];
        
        // Initial positions (stacked list)
        icon.targetPosition = Vec3(-100.0f, 150.0f - i * 80.0f, 0.0f);
        icon.position = icon.targetPosition;
        icon.position.x -= 50.0f; // Start offset for animation
        
        icon.rotation = Vec3(0, 0, 0);
        icon.scale = 1.0f;
        icon.targetScale = 1.0f;
        icon.selected = (i == 0);
        icon.assetId = i;
        
        // Try to load ICOB mesh
        ICOBModel model;
        if (assetLoader.LoadICOB(icon.iconName, model)) {
            icon.mesh = model;
            icon.meshLoaded = true;
            printf("[BrowserScene] Loaded icon '%s' for '%s'\n", icon.iconName.c_str(), icon.name.c_str());
        } else {
            icon.meshLoaded = false;
            printf("[BrowserScene] Failed to load icon '%s' for '%s'\n", icon.iconName.c_str(), icon.name.c_str());
        }
        
        saveIcons.push_back(icon);
    }

    printf("[BrowserScene] Initialized: %zu save icons\n", saveIcons.size());
}

void BrowserScene::OnExit() {
    printf("=== [BrowserScene] OnExit ===\n");
}

void BrowserScene::HandleInput(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_UP:
            case SDLK_w:
                if (selectedIndex > 0) {
                    saveIcons[selectedIndex].selected = false;
                    selectedIndex--;
                    saveIcons[selectedIndex].selected = true;
                    printf("[BrowserScene] Selected: %s\n", saveIcons[selectedIndex].name.c_str());
                }
                break;

            case SDLK_DOWN:
            case SDLK_s:
                if (selectedIndex < (int)saveIcons.size() - 1) {
                    saveIcons[selectedIndex].selected = false;
                    selectedIndex++;
                    saveIcons[selectedIndex].selected = true;
                    printf("[BrowserScene] Selected: %s\n", saveIcons[selectedIndex].name.c_str());
                }
                break;

            case SDLK_RETURN:
            case SDLK_SPACE:
            case SDLK_x:
                printf("[BrowserScene] Opening options for: %s\n", saveIcons[selectedIndex].name.c_str());
                // TODO: Open save options menu
                break;

            case SDLK_BACKSPACE:
            case SDLK_z: // O button on PS2
                printf("[BrowserScene] Going back to menu\n");
                requestedNextState = (int)State::Menu;
                break;
        }
    }
}

void BrowserScene::Update(double dt) {
    time += (float)dt;
    float t = (float)time;

    // Scene fade in
    const float FADE_IN_TIME = 0.3f;
    if (t < FADE_IN_TIME) {
        sceneAlpha = Easing::EaseOutQuad(t / FADE_IN_TIME);
    } else {
        sceneAlpha = 1.0f;
    }

    // Update scroll offset to keep selected item visible
    const float VISIBLE_AREA_TOP = 150.0f;
    const float VISIBLE_AREA_BOTTOM = -150.0f;
    const float ITEM_SPACING = 80.0f;
    
    float selectedY = VISIBLE_AREA_TOP - selectedIndex * ITEM_SPACING;
    if (selectedY - scrollOffset > VISIBLE_AREA_TOP - 30.0f) {
        targetScrollOffset = selectedY - (VISIBLE_AREA_TOP - 30.0f);
    } else if (selectedY - scrollOffset < VISIBLE_AREA_BOTTOM + 30.0f) {
        targetScrollOffset = selectedY - (VISIBLE_AREA_BOTTOM + 30.0f);
    }
    
    scrollOffset = Math::Lerp(scrollOffset, targetScrollOffset, (float)dt * 8.0f);

    // Update save icons
    for (size_t i = 0; i < saveIcons.size(); i++) {
        auto& icon = saveIcons[i];
        
        // Target position with scroll
        icon.targetPosition.y = VISIBLE_AREA_TOP - i * ITEM_SPACING + scrollOffset;
        
        // Smooth position animation
        icon.position.x = Math::Lerp(icon.position.x, icon.targetPosition.x, (float)dt * 8.0f);
        icon.position.y = Math::Lerp(icon.position.y, icon.targetPosition.y, (float)dt * 10.0f);
        
        // Scale based on selection
        icon.targetScale = icon.selected ? 1.4f : 1.0f;
        icon.scale = Math::Lerp(icon.scale, icon.targetScale, (float)dt * 10.0f);
        
        // Rotation animation (selected items spin)
        if (icon.selected) {
            icon.rotation.y += (float)dt * Math::PI * 0.6f; // Slow spin
        } else {
            // Slowly return to default rotation
            icon.rotation.y = Math::Lerp(icon.rotation.y, 0.0f, (float)dt * 3.0f);
        }
        
        // Slight hover effect
        if (icon.selected) {
            icon.position.z = sinf(t * 2.0f) * 5.0f;
        } else {
            icon.position.z = Math::Lerp(icon.position.z, 0.0f, (float)dt * 5.0f);
        }
    }

    // Debug logging
    static int lastLog = -1;
    int currentLog = (int)(t / 2.0f);
    if (currentLog != lastLog) {
        lastLog = currentLog;
        printf("[BrowserScene] t=%.1fs, selected=%d, scroll=%.1f\n", 
               t, selectedIndex, scrollOffset);
    }
}

void BrowserScene::Render(Renderer& renderer) {
    // Set fog
    renderer.SetFog(0.015f, Vec3(0.06f, 0.06f, 0.12f));

    // Draw header
    Color headerColor(0.9f, 0.9f, 1.0f, sceneAlpha);
    renderer.DrawText("Memory Card Browser", 200.0f, 30.0f, headerColor, 1.2f);
    
    // Draw memory card info
    Color mcColor = mc1.connected 
        ? Color(0.5f, 0.8f, 0.5f, sceneAlpha) 
        : Color(0.6f, 0.4f, 0.4f, sceneAlpha);
    char mcInfo[64];
    snprintf(mcInfo, sizeof(mcInfo), "%s: %d/%d", mc1.name.c_str(), mc1.usedSlots, mc1.totalSlots);
    renderer.DrawText(mcInfo, 30.0f, 60.0f, mcColor, 0.8f);

    // Draw save icons
    for (size_t i = 0; i < saveIcons.size(); i++) {
        const auto& icon = saveIcons[i];
        
        // Skip if off-screen
        if (icon.position.y < -220.0f || icon.position.y > 200.0f) {
            continue;
        }

        // Calculate render position
        // Map 3D position to 2D screen approximately
        Vec3 iconPos3D(
            icon.position.x,
            icon.position.y,
            icon.position.z - 100.0f // Push back in Z
        );
        
        // Draw the 3D icon
        if (icon.meshLoaded) {
            Color iconColor = icon.selected 
                ? Color(1.0f, 1.0f, 1.0f, sceneAlpha)
                : Color(0.7f, 0.7f, 0.8f, 0.8f * sceneAlpha);
            
            Vec3 iconScale(8.0f * icon.scale, 8.0f * icon.scale, 8.0f * icon.scale);
            renderer.DrawMesh(icon.mesh, iconPos3D, iconScale, iconColor, icon.rotation);
        } else {
            // Fallback: draw a colored cube
            Color fallbackColor = icon.selected 
                ? Color(0.4f, 0.6f, 0.9f, sceneAlpha)
                : Color(0.3f, 0.4f, 0.6f, 0.8f * sceneAlpha);
            
            Vec3 cubeScale(15.0f * icon.scale, 15.0f * icon.scale, 15.0f * icon.scale);
            renderer.DrawCube(iconPos3D, cubeScale, fallbackColor, icon.rotation);
        }
        
        // Draw save name (2D text overlay)
        float screenX = 200.0f;
        float screenY = 224.0f - icon.position.y * 0.8f;
        
        // Selection highlight
        if (icon.selected) {
            Color highlightColor(0.15f, 0.2f, 0.4f, 0.6f * sceneAlpha);
            renderer.DrawRect(screenX - 10.0f, screenY - 2.0f, 280.0f, 22.0f, highlightColor);
        }
        
        Color textColor = icon.selected 
            ? Color(1.0f, 1.0f, 1.0f, sceneAlpha)
            : Color(0.6f, 0.6f, 0.7f, 0.8f * sceneAlpha);
        renderer.DrawText(icon.name.c_str(), screenX, screenY, textColor, icon.selected ? 1.1f : 1.0f);
    }

    // Draw scrollbar if needed
    if (saveIcons.size() > 4) {
        float scrollbarX = 610.0f;
        float scrollbarY = 100.0f;
        float scrollbarHeight = 280.0f;
        
        // Background
        Color scrollbarBg(0.2f, 0.2f, 0.3f, 0.5f * sceneAlpha);
        renderer.DrawRect(scrollbarX, scrollbarY, 8.0f, scrollbarHeight, scrollbarBg);
        
        // Thumb
        float thumbHeight = scrollbarHeight / saveIcons.size();
        float thumbY = scrollbarY + (selectedIndex / (float)(saveIcons.size() - 1)) * (scrollbarHeight - thumbHeight);
        Color thumbColor(0.5f, 0.6f, 0.9f, 0.8f * sceneAlpha);
        renderer.DrawRect(scrollbarX, thumbY, 8.0f, thumbHeight, thumbColor);
    }

    // Draw navigation hints at bottom
    Color hintColor(0.5f, 0.5f, 0.6f, 0.7f * sceneAlpha);
    renderer.DrawText("UP/DOWN: Select   ENTER: Options   BACKSPACE: Back", 120.0f, 420.0f, hintColor, 0.8f);
}
