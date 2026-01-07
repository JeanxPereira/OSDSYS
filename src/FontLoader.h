#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Tipos de Fonte
enum class FontType {
    ASCII_LEGACY, // FNTASCII (ASCII Padrão)
    KANJI_GRID,   // FNTEX... (Kanjis)
    OSD_ICONS,    // FNTEXOSD (Icones do sistema)
    VECTOR_DATA,  // FONTM (Vetorial)
    GENERIC
};

// Configuração do "Banco" de Textura
struct AtlasConfig {
    FontType type;
    std::string name;
    int width;
    int height;
    int cellWidth;
    int cellHeight;
    int strideY;      // Apenas para ASCII_LEGACY
    int offsetY;      // Apenas para ASCII_LEGACY
    int charsPerRow;  // Apenas para ASCII_LEGACY
    int asciiOffset;  // Apenas para ASCII_LEGACY
    bool is8bpp;
};

// Um Glifo individual
struct FontGlyph {
    float u0, v0, u1, v1;
    int width, height, advance;
};

// Um Arquivo carregado
struct FontBank {
    AtlasConfig config;
    std::vector<uint8_t> textureData; // RGBA8888
    std::vector<FontGlyph> glyphs;
    FontGlyph defaultGlyph;
    bool isValid;
};

class FontLoader {
public:
    static constexpr int GLYPH_HEIGHT = 16; 

    FontLoader();
    ~FontLoader();

    // Carregamento Unificado
    bool LoadAll(const std::string& baseDir);
    
    // Acesso aos Bancos (Multi-fonte)
    size_t GetBankCount() const { return banks.size(); }
    const FontBank* GetBank(size_t index) const;

    // --- COMPATIBILIDADE COM CÓDIGO LEGADO (Renderer atual) ---
    // Estes métodos redirecionam para o banco "FNTASCII" principal
    const std::vector<uint8_t>& GetTextureData() const; 
    int GetAtlasWidth() const; 
    int GetAtlasHeight() const; 
    const FontGlyph& GetGlyph(int index) const;
    float GetTextWidth(const char* text, float scale) const;

private:
    std::vector<FontBank> banks; 
    int mainBankIndex;

    // Leitura de Arquivo
    bool LoadFile(const std::string& filepath, FontBank& outBank);
    
    // Decodificadores de Pixel
    void Convert8bppToRGBA(const std::vector<uint8_t>& raw, FontBank& bank);
    void Convert4bppToRGBA(const std::vector<uint8_t>& raw, FontBank& bank);

    // Configuradores Específicos (Chamam os Processadores Lógicos)
    void SetupAsciiBank(FontBank& bank);
    void SetupKanjiBank(FontBank& bank);
    void SetupIconBank(FontBank& bank);
    void SetupVectorBank(FontBank& bank);

    // === PROCESSADORES LÓGICOS (FALTAVAM AQUI!) ===
    // Calcula as coordenadas UV de todos os glifos baseada na config
    void ProcessAsciiLogic(FontBank& bank);
    void ProcessLinearLogic(FontBank& bank);
};