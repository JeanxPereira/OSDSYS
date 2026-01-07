#include "FontLoader.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>

// ============================================================================
// CONSTRUCTOR & DESTRUCTOR
// ============================================================================

FontLoader::FontLoader() : mainBankIndex(-1) {}

FontLoader::~FontLoader() {}

// ============================================================================
// COMPATIBILIDADE LEGADO (Wrapper para o Renderer atual usar FNTASCII)
// ============================================================================

const std::vector<uint8_t>& FontLoader::GetTextureData() const {
    // Retorna a textura da fonte ASCII se carregada
    if (mainBankIndex >= 0 && mainBankIndex < (int)banks.size()) {
        return banks[mainBankIndex].textureData;
    }
    static std::vector<uint8_t> empty;
    return empty;
}

int FontLoader::GetAtlasWidth() const {
    return (mainBankIndex >= 0) ? banks[mainBankIndex].config.width : 0;
}

int FontLoader::GetAtlasHeight() const {
    return (mainBankIndex >= 0) ? banks[mainBankIndex].config.height : 0;
}

const FontGlyph& FontLoader::GetGlyph(int index) const {
    if (mainBankIndex >= 0 && index >= 0 && index < (int)banks[mainBankIndex].glyphs.size()) {
        return banks[mainBankIndex].glyphs[index];
    }
    static FontGlyph dummy = {0,0,0,0,0,0,0};
    return dummy;
}

float FontLoader::GetTextWidth(const char* text, float scale) const {
    if (mainBankIndex < 0) return 0.0f;
    float width = 0.0f;
    const FontBank& mainBank = banks[mainBankIndex];
    
    while (*text) {
        uint8_t c = (uint8_t)*text;
        // Trata apenas ASCII básico por enquanto na compatibilidade
        if (c < mainBank.glyphs.size()) {
            width += mainBank.glyphs[c].advance * scale;
        } else {
            width += 10.0f * scale; // Fallback
        }
        text++;
    }
    return width;
}

// Implementação do Getter que faltava na compilação anterior
const FontBank* FontLoader::GetBank(size_t index) const {
    if (index < banks.size()) {
        return &banks[index];
    }
    return nullptr;
}

// ============================================================================
// LÓGICA DE CARREGAMENTO (LOADALL)
// ============================================================================

bool FontLoader::LoadAll(const std::string& baseDir) {
    banks.clear();
    mainBankIndex = -1;

    // Configuração da Fila de Carregamento
    struct FileRequest {
        const char* name;
        FontType type;
        bool is8bpp;
    };

    FileRequest requests[] = {
        {"FNTASCII.bin", FontType::ASCII_LEGACY, true},  // 8bpp
        {"FNTEXOSD.bin", FontType::OSD_ICONS,    false}, // 4bpp
        {"FNTEX000.bin", FontType::KANJI_GRID,   true},  // 8bpp
        {"FNTEX001.bin", FontType::KANJI_GRID,   true},  // 8bpp
        {"FNTADD00.bin", FontType::OSD_ICONS,   false},  // 8bpp
        {"FONTM.fbj2",   FontType::VECTOR_DATA,  false}  // Raw
    };

    std::string cleanDir = baseDir;
    if (!cleanDir.empty() && cleanDir.back() != '/' && cleanDir.back() != '\\') {
        cleanDir += '/';
    }

    printf("[FontLoader] Scanning directory: %s\n", cleanDir.c_str());

    for (const auto& req : requests) {
        FontBank bank;
        // Configuração Inicial
        bank.config.name = req.name;
        bank.config.type = req.type;
        bank.config.is8bpp = req.is8bpp;
        bank.isValid = false;

        std::string fullPath = cleanDir + req.name;
        
        if (LoadFile(fullPath, bank)) {
            // Se carregou o binário, processa os glifos conforme o tipo
            switch (req.type) {
                case FontType::ASCII_LEGACY: 
                    SetupAsciiBank(bank);
                    if (mainBankIndex == -1) mainBankIndex = (int)banks.size(); 
                    break;
                case FontType::OSD_ICONS:    
                    SetupIconBank(bank);  
                    break;
                case FontType::KANJI_GRID:   
                    SetupKanjiBank(bank); 
                    break;
                case FontType::VECTOR_DATA:  
                    SetupVectorBank(bank); 
                    break;
            }
            bank.isValid = true;
            banks.push_back(bank);
            printf("[FontLoader] Successfully loaded: %s\n", req.name);
        } else {
            // Silent fail is fine for optional files, but print debug
            // printf("[FontLoader] Optional file not found: %s\n", req.name);
        }
    }

    return (mainBankIndex != -1);
}

bool FontLoader::LoadFile(const std::string& filepath, FontBank& bank) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size == 0) return false;

    std::vector<uint8_t> rawData(size);
    file.read(reinterpret_cast<char*>(rawData.data()), size);
    file.close();

    // === CORREÇÃO DE GEOMETRIA (STRIDE 512 PARA ÍCONES) ===
    bank.config.width = 256; 

    // OSD_ICONS costuma ter stride de 512 (metade vazia ou interlaced buffer)
    if (bank.config.type == FontType::OSD_ICONS) {
        bank.config.width = 512; // <--- FIX DAS LISTRAS PRETAS
    }

    // Calcula altura baseado no tamanho do arquivo e bpp
    if (bank.config.type == FontType::VECTOR_DATA) {
        bank.config.width = 64; bank.config.height = 64;
    } else if (bank.config.is8bpp) {
        bank.config.height = (int)(size / bank.config.width);
    } else {
        // 4bpp: Size bytes * 2 pixels_per_byte / width
        bank.config.height = (int)((size * 2) / bank.config.width);
    }

    if (bank.config.height <= 0) bank.config.height = 1;

    // Conversão
    if (bank.config.type == FontType::VECTOR_DATA) {
        // Placeholder xadrez
        bank.textureData.assign(bank.config.width * bank.config.height * 4, 0);
        for(int i=0; i<bank.config.width*bank.config.height; i++) {
            bool c = ((i%bank.config.width)/8 + (i/bank.config.width)/8) % 2;
            bank.textureData[i*4+3] = c ? 50 : 200; // Alpha visualization only
        }
    } else if (bank.config.is8bpp) {
        Convert8bppToRGBA(rawData, bank);
    } else {
        Convert4bppToRGBA(rawData, bank);
    }

    return true;
}

// ============================================================================
// CONVERSORES DE PIXEL
// ============================================================================

void FontLoader::Convert8bppToRGBA(const std::vector<uint8_t>& raw, FontBank& bank) {
    // (Código 8bpp anterior mantido)
    size_t pixelCount = bank.config.width * bank.config.height;
    bank.textureData.resize(pixelCount * 4);
    for (size_t i = 0; i < pixelCount && i < raw.size(); i++) {
        uint8_t val = raw[i];
        // Boost no alpha para legibilidade no OSD (valores baixos quase transparentes)
        uint8_t alpha = (val > 16) ? std::min(255, val * 2) : 0; 
        
        size_t dst = i * 4;
        bank.textureData[dst+0] = 255; bank.textureData[dst+1] = 255;
        bank.textureData[dst+2] = 255; bank.textureData[dst+3] = alpha;
    }
}

void FontLoader::Convert4bppToRGBA(const std::vector<uint8_t>& raw, FontBank& bank) {
    size_t pixelCount = bank.config.width * bank.config.height;
    bank.textureData.resize(pixelCount * 4);
    size_t processed = 0;
    
    for (size_t i = 0; i < raw.size() && processed < pixelCount; i++) {
        uint8_t byte = raw[i];
        
        // Ordem do PS2 4bpp: Pixel baixo (0-3) primeiro, Pixel alto (4-7) depois.
        uint8_t p1 = byte & 0x0F; 
        uint8_t p2 = (byte >> 4) & 0x0F;

        auto SetPix = [&](uint8_t val) {
            if (processed >= pixelCount) return;
            
            // Ícones do PS2 são brancos com alpha
            // Val 0 = Transparente
            // Val 1-15 = Opacidade / Anti-Aliasing de Borda
            
            // Fórmula visual "pop": Aumentar contraste
            uint8_t alpha = 0;
            if (val > 0) {
                // Mapeia 1..15 para ~40..255
                // Isso deixa as bordas suaves (linear filter faz o resto) e miolo sólido
                alpha = (uint8_t)std::min(255, 30 + val * 16);
            }

            size_t dst = processed * 4;
            bank.textureData[dst+0] = 255; 
            bank.textureData[dst+1] = 255;
            bank.textureData[dst+2] = 255;
            bank.textureData[dst+3] = alpha;
            processed++;
        };

        SetPix(p1);
        SetPix(p2);
    }
}

// ============================================================================
// CONFIGURADORES (SETUPS)
// ============================================================================

void FontLoader::SetupAsciiBank(FontBank& bank) {
    bank.config.cellWidth = 16;
    bank.config.cellHeight = 16;
    bank.config.strideY = 20;
    bank.config.offsetY = 4;
    bank.config.charsPerRow = 8;
    bank.config.asciiOffset = 32;
    ProcessAsciiLogic(bank);
}


void FontLoader::SetupKanjiBank(FontBank& bank) {
    // Kanji são grades simples 16x16, geralmente sem stride estranho
    bank.config.cellWidth = 16;
    bank.config.cellHeight = 16;
    // O resto é zero por padrão
    ProcessLinearLogic(bank);
}

void FontLoader::SetupIconBank(FontBank& bank) {
    // Configura Grid 16x16 padrão. 
    // Nota: A largura agora é 512, então 'charsPerRow' aumentaria se calculado auto.
    // O renderer calcula UVs com base na textura width, então está ok.
    
    // Ícones do PS2 ocupam blocos lógicos. A visualização mostrará o atlas expandido.
    bank.config.cellWidth = 16;
    bank.config.cellHeight = 16;
    ProcessLinearLogic(bank);
}

void FontLoader::SetupVectorBank(FontBank& bank) {
    bank.defaultGlyph = {0,0,0,0,0,0,0};
    // Nada a processar em bitmaps
}

// ============================================================================
// LÓGICA DE PROCESSAMENTO DE GLIFOS
// ============================================================================

void FontLoader::ProcessAsciiLogic(FontBank& bank) {
    // O sistema "Sagrado" de 256 glifos
    bank.glyphs.resize(256);
    bank.defaultGlyph = {0,0,0,0, bank.config.cellWidth, bank.config.cellHeight, bank.config.cellWidth/2};

    int texW = bank.config.width;
    int texH = bank.config.height;

    for (int i = 0; i < 256; i++) {
        // Cálculo lógico OSDSYS
        int atlasIndex = i - bank.config.asciiOffset; // i - 32

        if (atlasIndex < 0) {
            bank.glyphs[i] = bank.defaultGlyph;
            if (bank.config.asciiOffset > 0 && i == 32) bank.glyphs[i].advance = 6; // Espaço
            continue;
        }

        int col = atlasIndex % bank.config.charsPerRow;
        int row = atlasIndex / bank.config.charsPerRow;

        // O famoso stride Y do FNTASCII
        int yStart = (row * bank.config.strideY) + bank.config.offsetY;
        int yEnd   = yStart + bank.config.cellHeight;

        if (yEnd > texH) {
            bank.glyphs[i] = bank.defaultGlyph;
            continue;
        }

        // Setup Glyph
        bank.glyphs[i].width = bank.config.cellWidth;
        bank.glyphs[i].height = bank.config.cellHeight;
        
        bank.glyphs[i].u0 = (col * bank.config.cellWidth) / (float)texW;
        bank.glyphs[i].u1 = ((col + 1) * bank.config.cellWidth) / (float)texW;
        
        // Coordenadas V com stride
        bank.glyphs[i].v0 = yStart / (float)texH;
        bank.glyphs[i].v1 = yEnd / (float)texH;

        // Cálculo de Advance (Largura Real) Scaneando a textura (no canal Alpha)
        // Isso evita "espaço extra" após letras finas como 'l' ou 'i'
        int startPxX = col * bank.config.cellWidth;
        int maxPixX = 0;
        bool foundPixel = false;

        for (int py = 0; py < bank.config.cellHeight; py++) {
            if ((yStart + py) >= texH) break;
            
            for (int px = 0; px < bank.config.cellWidth; px++) {
                // RGBA -> idx * 4 + 3 (Alpha Channel)
                int bufferIdx = ((yStart + py) * texW + (startPxX + px)) * 4 + 3;
                
                if (bufferIdx < (int)bank.textureData.size()) {
                    uint8_t alpha = bank.textureData[bufferIdx];
                    if (alpha > 20) { // Threshold de pixel visível
                        if (px > maxPixX) maxPixX = px;
                        foundPixel = true;
                    }
                }
            }
        }

        int advance = foundPixel ? (maxPixX + 2) : (bank.config.cellWidth / 2);
        if (advance > bank.config.cellWidth) advance = bank.config.cellWidth;
        bank.glyphs[i].advance = advance;
    }
}

void FontLoader::ProcessLinearLogic(FontBank& bank) {
    // Grade matemática simples (usado para Kanjis e Icones)
    int cols = bank.config.width / bank.config.cellWidth;
    int rows = bank.config.height / bank.config.cellHeight;
    int total = cols * rows;

    bank.glyphs.resize(total);
    bank.defaultGlyph = {0,0,0,0, bank.config.cellWidth, bank.config.cellHeight, bank.config.cellWidth};

    for (int i = 0; i < total; i++) {
        int c = i % cols;
        int r = i / cols;

        int pxX = c * bank.config.cellWidth;
        int pxY = r * bank.config.cellHeight;

        bank.glyphs[i].width  = bank.config.cellWidth;
        bank.glyphs[i].height = bank.config.cellHeight;
        
        bank.glyphs[i].u0 = pxX / (float)bank.config.width;
        bank.glyphs[i].u1 = (pxX + bank.config.cellWidth) / (float)bank.config.width;
        bank.glyphs[i].v0 = pxY / (float)bank.config.height;
        bank.glyphs[i].v1 = (pxY + bank.config.cellHeight) / (float)bank.config.height;
        
        // Por padrão, mono-espaçado. FNTEXOSD pode ter icones maiores ocupando 2 células,
        // mas aqui tratamos como blocos individuais.
        bank.glyphs[i].advance = bank.config.cellWidth;
    }
}