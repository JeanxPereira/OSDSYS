#include "Platform.h"
#include <GL/glew.h>
#include "DebugFontScene.h"
#include "../Renderer.h"
#include "../Core.h"
#include "../FontLoader.h"
#include <cstdio>
#include <cmath>
#include <algorithm> // para std::min, std::max

// Função Helper para desenhar outline fino
void DrawHollowRect(Renderer& r, float x, float y, float w, float h, const Color& c) {
    r.DrawRect(x, y, w, 1.0f, c);         // Top
    r.DrawRect(x, y + h - 1.0f, w, 1.0f, c); // Bottom
    r.DrawRect(x, y, 1.0f, h, c);         // Left
    r.DrawRect(x + w - 1.0f, y, 1.0f, h, c); // Right
}

void DebugFontScene::OnEnter() {
    printf("=== [DebugFontScene] Multi-Font Viewer ===\n");
    texturesLoaded = false;
    targetBankIndex = 0.0f;
    currentBankIndex = 0.0f;
    maxBanks = 0;
    pageIndex = 0; // Reinicia a página
}

void DebugFontScene::OnExit() {
    for (auto& t : debugTextures) {
        if (t.valid && t.id != 0) {
            glDeleteTextures(1, &t.id);
        }
    }
    debugTextures.clear();
}

void DebugFontScene::HandleInput(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_BACKSPACE) {
            requestedNextState = (int)State::Menu;
        }

        // --- NAVEGAÇÃO DE BANCOS (CIMA / BAIXO) ---
        if (event.key.keysym.sym == SDLK_UP) {
            targetBankIndex -= 1.0f;
            if (targetBankIndex < 0.0f) targetBankIndex = 0.0f;
            pageIndex = 0; // Reseta paginação ao mudar arquivo
        }
        if (event.key.keysym.sym == SDLK_DOWN) {
            targetBankIndex += 1.0f;
            if (maxBanks > 0 && targetBankIndex >= maxBanks) targetBankIndex = (float)maxBanks - 1;
            pageIndex = 0;
        }

        // --- NAVEGAÇÃO DE PÁGINAS DE GLYPHS (ESQUERDA / DIREITA) ---
        if (event.key.keysym.sym == SDLK_RIGHT) {
            pageIndex++;
        }
        if (event.key.keysym.sym == SDLK_LEFT) {
            pageIndex--;
            if(pageIndex < 0) pageIndex = 0;
        }
    }
}

void DebugFontScene::Update(double dt) {
    // Interpolação suave do Scroll Vertical
    float diff = targetBankIndex - currentBankIndex;
    if (std::abs(diff) < 0.001f) {
        currentBankIndex = targetBankIndex;
    } else {
        currentBankIndex += diff * 10.0f * (float)dt;
    }
}

void DebugFontScene::Render(Renderer& renderer) {
    // Lazy Texture Loading (Carrega apenas ao renderizar a primeira vez)
    if (!texturesLoaded) {
        static FontLoader localLoader;
        // Caminho relativo ao executável
        if (localLoader.LoadAll("assets/fonts/")) {
            maxBanks = localLoader.GetBankCount();
            debugTextures.clear();
            for (size_t i = 0; i < maxBanks; i++) {
                const auto* bank = localLoader.GetBank(i);
                Texture tex = renderer.CreateTexture(
                    bank->textureData.data(),
                    bank->config.width,
                    bank->config.height,
                    4
                );
                debugTextures.push_back(tex);
            }
        }
        texturesLoaded = true;
    }

    renderer.DisableFog();
    
    // Background Dark Gray
    renderer.DrawRect(0.0f, 0.0f, 640.0f, 448.0f, Color(0.12f, 0.12f, 0.14f, 1.0f));

    if (maxBanks == 0) {
        renderer.DrawText("Nenhum arquivo de fonte encontrado!", 200.0f, 200.0f, Color(1.0f, 0.0f, 0.0f, 1.0f), 1.0f);
        return;
    }

    // Lógica do Scroll Slide Vertical
    int indexA = (int)floor(currentBankIndex);
    int indexB = indexA + 1;
    float fract = currentBankIndex - (float)indexA;
    float screenHeight = 448.0f;

    float yOffsetA = -fract * screenHeight;
    float yOffsetB = yOffsetA + screenHeight;

    if (indexA >= 0 && indexA < maxBanks) {
        DrawBank(renderer, indexA, yOffsetA, 1.0f - (fract * 0.5f));
    }

    if (indexB >= 0 && indexB < maxBanks) {
        DrawBank(renderer, indexB, yOffsetB, fract);
    }

    // UI Overlay Topo
    renderer.DrawRect(0.0f, 0.0f, 640.0f, 40.0f, Color(0.0f, 0.0f, 0.0f, 0.8f));
    renderer.DrawText("UP/DOWN: Switch Font | LEFT/RIGHT: Switch Page", 20.0f, 10.0f, Color(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);

    char buffer[64];
    snprintf(buffer, 64, "Bank %.2f / %d", currentBankIndex + 1.0f, maxBanks);
    renderer.DrawText(buffer, 450.0f, 30.0f, Color(1.0f, 1.0f, 0.0f, 1.0f), 1.0f);
}

void DebugFontScene::DrawBank(Renderer& renderer, int bankIndex, float yOffset, float alpha) {
    if (bankIndex < 0 || bankIndex >= (int)debugTextures.size()) return;

    // Recarrega apenas ponteiro estático (já carregado)
    static FontLoader localLoader;
    if (localLoader.GetBankCount() == 0) localLoader.LoadAll("assets/fonts/");
    
    const FontBank* bank = localLoader.GetBank(bankIndex);
    const Texture& tex = debugTextures[bankIndex];

    float startX = 20.0f;
    float startY = 60.0f + yOffset;

    // --- Header Info ---
    const char* typeStr = "Unknown";
    if (bank->config.type == FontType::ASCII_LEGACY) typeStr = "ASCII (Legacy)";
    if (bank->config.type == FontType::KANJI_GRID)   typeStr = "Kanji Grid";
    if (bank->config.type == FontType::OSD_ICONS)    typeStr = "OSD Icons (4bpp)";
    if (bank->config.type == FontType::VECTOR_DATA)  typeStr = "FONTM Vector";

    char title[128];
    snprintf(title, 128, "%s - [%s] (%dx%d)", 
             bank->config.name.c_str(), 
             typeStr,
             bank->config.width, bank->config.height);

    renderer.DrawText(title, startX, startY, Color(0.4f, 0.9f, 1.0f, alpha), 1.2f);

    if (bank->config.type == FontType::VECTOR_DATA) return;

    // --- Preview de Textura (Lado Direito) ---
    float maxW = 240.0f; 
    float maxH = 240.0f;
    
    float aspect = 1.0f;
    if (bank->config.height > 0) 
        aspect = (float)bank->config.width / (float)bank->config.height;
    
    float drawW = maxW;
    float drawH = maxW / aspect;

    if (drawH > maxH) {
        drawH = maxH;
        drawW = maxH * aspect;
    }

    float previewX = 380.0f;
    float previewY = startY + 40.0f;

    // Fundo + Textura + Info Dimensão
    renderer.DrawRect(previewX - 2, previewY - 2, drawW + 4, drawH + 4, Color(0.3f, 0.3f, 0.3f, 0.5f * alpha));
    renderer.DrawSprite(tex, previewX, previewY, drawW, drawH, Color(1.0f, 1.0f, 1.0f, alpha));

    char sizeStr[32];
    snprintf(sizeStr, 32, "View: %.0fx%.0f", drawW, drawH);
    renderer.DrawText(sizeStr, previewX, previewY + drawH + 5.0f, Color(0.7f, 0.7f, 0.7f, alpha), 0.7f);

    // --- Grid Visual (Lado Esquerdo) ---
    float gridY = startY + 60.0f; // Baixei um pouco por causa do texto de página
    float cellSize = 22.0f;
    float gap = 2.0f;
    
    // Controle de Página
    int cols = 10;
    int rows = 14;
    int glyphsPerPage = cols * rows; 
    int totalGlyphs = (int)bank->glyphs.size();
    
    // Garante que pageIndex não ultrapasse limite
    int maxPages = (totalGlyphs + glyphsPerPage - 1) / glyphsPerPage;
    if (maxPages == 0) maxPages = 1;
    // (A variável de instância pageIndex é ajustada no HandleInput, mas aqui limitamos visualmente se estourar)
    int safePage = std::min(pageIndex, maxPages - 1); 
    
    int startGlyph = safePage * glyphsPerPage;
    
    // Texto informativo de Página
    char pageInfo[64];
    snprintf(pageInfo, 64, "Page %d/%d (IDs %d-%d)", 
             safePage+1, maxPages, startGlyph, std::min(startGlyph+glyphsPerPage-1, totalGlyphs-1));
    renderer.DrawText(pageInfo, startX, startY + 30.0f, Color(1.0f, 1.0f, 0.0f, alpha), 0.8f);

    // Loop do Grid
    for (int i = 0; i < glyphsPerPage; i++) {
        int glyphIdx = startGlyph + i;
        if (glyphIdx >= totalGlyphs) break;
        
        int c = i % cols;
        int r = i / cols;
        
        // Coordenadas da Célula na Tela
        float gx = startX + c * (cellSize + gap);
        float gy = gridY + r * (cellSize + gap);
        
        const auto& glyph = bank->glyphs[glyphIdx];
        bool hasData = (glyph.width > 0 && glyph.u1 > glyph.u0);

        if (hasData) {
             // Caixa verde escuro = Tem conteúdo
             renderer.DrawRect(gx, gy, cellSize, cellSize, Color(0.0f, 0.2f, 0.0f, 0.4f * alpha)); 
             DrawHollowRect(renderer, gx, gy, cellSize, cellSize, Color(0.2f, 0.6f, 0.2f, 0.6f * alpha)); // Outline

             // ID numérico pequeno
             char idStr[8];
             snprintf(idStr, 8, "%d", glyphIdx);
             renderer.DrawText(idStr, gx + 2.0f, gy + 4.0f, Color(1.0f, 1.0f, 1.0f, alpha), 0.5f);
        } else {
             // Caixa cinza/vermelho = Vazio / Dummy
             DrawHollowRect(renderer, gx, gy, cellSize, cellSize, Color(0.3f, 0.0f, 0.0f, 0.3f * alpha));
        }
    }
}