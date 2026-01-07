#include "Platform.h"
#include "DebugTextureScene.h"
#include "../Core.h"
#include <algorithm>
#include <cstdio> 

void DebugTextureScene::OnEnter() {
    printf("=== [DebugTextureScene] ===\n");
    loader.SetDirectory("assets/textures/");
    textureFiles = loader.GetAvailableTextures();
    selectedIndex = 0;
    loadedName = "";
    if (!textureFiles.empty()) LoadSelected();
}

void DebugTextureScene::OnExit() {
    // Texture deletion managed lazily or by Renderer logic
}

void DebugTextureScene::HandleInput(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_UP) {
            selectedIndex--; 
            if(selectedIndex<0) selectedIndex=0; 
            else LoadSelected();
        }
        else if (event.key.keysym.sym == SDLK_DOWN) {
            selectedIndex++;
            if(selectedIndex >= (int)textureFiles.size()) selectedIndex = (int)textureFiles.size()-1;
            else LoadSelected();
        }
        else if (event.key.keysym.sym == SDLK_RIGHT) zoom += 0.5f; 
        else if (event.key.keysym.sym == SDLK_LEFT) { zoom -= 0.5f; if(zoom<0.5f) zoom=0.5f; }
        else if (event.key.keysym.sym == SDLK_SPACE) showAlphaChecker = !showAlphaChecker;
        else if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_BACKSPACE) {
            requestedNextState = (int)State::Menu;
        }
    }
}

void DebugTextureScene::Update(double dt) {}

void DebugTextureScene::LoadSelected() {
    if (selectedIndex < 0 || selectedIndex >= (int)textureFiles.size()) return;
    std::string name = textureFiles[selectedIndex];
    if (name == loadedName) return; 
    // Just flag reset, lazy load happens in Render
    loadedName = "";
}

void DebugTextureScene::Render(Renderer& renderer) {
    renderer.DisableFog();
    
    // Background Global
    renderer.DrawRect(0, 0, 640, 448, Color(0.12f, 0.12f, 0.14f, 1.0f));

    // ==========================================
    // AREA DE PREVIEW (Lado Direito)
    // ==========================================
    float previewX = 220.0f;
    float previewY = 50.0f;
    float previewW = 400.0f;
    float previewH = 350.0f;
    float centerX = previewX + previewW / 2.0f;
    float centerY = previewY + previewH / 2.0f;

    // Fundo Container
    renderer.DrawRect(previewX - 2, previewY - 2, previewW + 4, previewH + 4, Color(0.2f, 0.2f, 0.22f, 1.0f));
    renderer.DrawRect(previewX, previewY, previewW, previewH, Color(0.0f, 0.0f, 0.0f, 1.0f));

    // LOAD TEXTURE (Lazy Logic)
    if (!textureFiles.empty()) {
        std::string target = textureFiles[selectedIndex];
        if (target != loadedName) {
            if (currentTexture.valid) renderer.DeleteTexture(currentTexture);
            
            // CORREÇÃO AQUI: Cast explicito float na Color
            renderer.DrawText("Loading...", centerX - 30.0f, centerY, Color(1.0f,1.0f,0.0f, 1.0f), 1.0f);
            
            if (loader.Load(target, texInfo)) {
                currentTexture = renderer.CreateTexture(texInfo.pixels.data(), texInfo.width, texInfo.height, 4);
                loadedName = target;
            } else {
                loadedName = target; // prevent loop
                printf("Fail Load: %s\n", target.c_str());
            }
        }
    }

    // DRAW TEXTURE
    if (currentTexture.valid) {
        float drawW = (float)currentTexture.width * zoom;
        float drawH = (float)currentTexture.height * zoom;
        float dx = centerX - drawW / 2.0f;
        float dy = centerY - drawH / 2.0f;

        if (showAlphaChecker) DrawCheckerboard(renderer, dx, dy, drawW, drawH);
        
        renderer.DrawSprite(currentTexture, dx, dy, drawW, drawH);
        
        // Info Header
        char buf[128];
        const char* fmt = (texInfo.format==TexFormat::Indexed4)?"4bpp":
                          (texInfo.format==TexFormat::Indexed8)?"8bpp":
                          (texInfo.format==TexFormat::RGBA16)?"RGBA16":"RGBA32";
        
        snprintf(buf, 128, "%s [%dx%d] %s", loadedName.c_str(), currentTexture.width, currentTexture.height, fmt);
        renderer.DrawText(buf, previewX, 20.0f, Color(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
        
        snprintf(buf, 128, "Zoom: %.1fx", zoom);
        renderer.DrawText(buf, previewX, 35.0f, Color(0.7f, 0.7f, 0.7f, 1.0f), 0.7f);
        
    } else {
        renderer.DrawText("Preview Unavailable", centerX-60.0f, centerY, Color(0.5f, 0.5f, 0.5f, 1.0f), 1.0f);
    }

    // ==========================================
    // SIDEBAR LIST
    // ==========================================
    renderer.DrawRect(0, 0, 200, 448, Color(0.08f, 0.08f, 0.1f, 1.0f));
    renderer.DrawRect(198, 0, 2, 448, Color(0.3f, 0.3f, 0.35f, 1.0f)); 
    
    float startY = 40.0f;
    int maxLines = 18;
    int listStart = std::max(0, selectedIndex - maxLines/2);
    if(listStart + maxLines > (int)textureFiles.size()) listStart = std::max(0, (int)textureFiles.size()-maxLines);

    renderer.DrawText("TEXTURES", 20.0f, 15.0f, Color(1.0f, 0.8f, 0.0f, 1.0f), 0.9f);

    for (int i = 0; i < maxLines; i++) {
        int idx = listStart + i;
        if (idx >= (int)textureFiles.size()) break;
        
        float ly = startY + i * 20.0f;
        bool active = (idx == selectedIndex);
        
        // CORREÇÃO: Argumentos do ternário devem ter mesmo tipo Color
        Color activeCol = Color(1.0f, 1.0f, 1.0f, 1.0f);
        Color dimCol = Color(0.6f, 0.6f, 0.6f, 1.0f);

        if (active) renderer.DrawRect(5, ly-2, 190, 20, Color(0.2f, 0.4f, 0.6f, 1.0f));
        renderer.DrawText(textureFiles[idx].c_str(), 15.0f, ly, active?activeCol:dimCol, 0.7f);
    }

    // Footer
    renderer.DrawRect(200, 420, 440, 28, Color(0.0f,0.0f,0.0f,0.5f));
    renderer.DrawText("Nav: Arrows | Space: Alpha | ESC: Menu", 220.0f, 426.0f, Color(0.8f, 0.8f, 0.8f, 1.0f), 0.7f);
}

void DebugTextureScene::DrawCheckerboard(Renderer& r, float x, float y, float w, float h) {
    if (w*h > 1200*1200) { r.DrawRect(x,y,w,h,Color(0.5f,0.5f,0.5f,1.0f)); return; }
    
    r.DrawRect(x, y, w, h, Color(0.4f, 0.4f, 0.4f, 1.0f)); 
    
    float size = 16.0f;
    int cols = (int)(w/size) + 1;
    int rows = (int)(h/size) + 1;
    Color light(0.6f, 0.6f, 0.6f, 1.0f);
    
    for(int ry=0; ry<rows; ry++) {
        for(int rx=0; rx<cols; rx++) {
            if ((rx+ry)%2 != 0) continue;
            float px = x + rx*size;
            float py = y + ry*size;
            float rw = (px+size > x+w) ? x+w - px : size;
            float rh = (py+size > y+h) ? y+h - py : size;
            if(rw>0 && rh>0) r.DrawRect(px,py,rw,rh, light);
        }
    }
}