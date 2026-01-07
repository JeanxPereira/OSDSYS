#include "Platform.h"
#include "TextureLoader.h"
#include <fstream>
#include <cmath>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <iostream>

namespace fs = std::filesystem;

// Tabelas de Mapeamento do GS (Hardware)
static const uint8_t block32[] = {
    0, 1, 4, 5, 16, 17, 20, 21, 2, 3, 6, 7, 18, 19, 22, 23,
    8, 9, 12, 13, 24, 25, 28, 29, 10, 11, 14, 15, 26, 27, 30, 31
};

static const uint8_t block16[] = {
    0, 2, 8, 10, 1, 3, 9, 11, 4, 6, 12, 14, 5, 7, 13, 15,
    16, 18, 24, 26, 17, 19, 25, 27, 20, 22, 28, 30, 21, 23, 29, 31
};

static const uint8_t block8[] = {
    0, 1, 4, 5, 16, 17, 20, 21, 2, 3, 6, 7, 18, 19, 22, 23,
    8, 9, 12, 13, 24, 25, 28, 29, 10, 11, 14, 15, 26, 27, 30, 31
};

static const uint8_t block4[] = {
    0, 2, 8, 10, 1, 3, 9, 11, 4, 6, 12, 14, 5, 7, 13, 15,
    16, 18, 24, 26, 17, 19, 25, 27, 20, 22, 28, 30, 21, 23, 29, 31
};

// -----------------------------------------------------------------------

TextureLoader::TextureLoader() {
    directory = "assets/textures/";
}

TextureLoader::~TextureLoader() {}

void TextureLoader::SetDirectory(const std::string& dir) {
    directory = dir;
    if (!directory.empty() && directory.back() != '/' && directory.back() != '\\') directory += '/';
}

std::vector<std::string> TextureLoader::GetAvailableTextures() const {
    std::vector<std::string> files;
    if (!fs::exists(directory)) return files;
    try {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".bin" || ext == ".tm2" || ext == ".raw") {
                    files.push_back(entry.path().stem().string());
                }
            }
        }
    } catch (...) {}
    std::sort(files.begin(), files.end());
    return files;
}

bool TextureLoader::Load(const std::string& name, TexData& outData) {
    std::string filename = name + ".bin";
    std::string fullPath = directory + filename;
    if (!fs::exists(fullPath)) fullPath = directory + name;
    return LoadFromPath(fullPath, outData);
}

// --------------------------------------------------------------------------------------
// HARDWARE SIMULATION - Calculate Memory Address from X,Y (Unswizzling)
// --------------------------------------------------------------------------------------
uint32_t TextureLoader::GetGSAddress(uint32_t x, uint32_t y, uint32_t width, PS2_PSM psm)
{
    switch (psm)
    {
        case GS_PSM_32:
        {
            // Page: 64x32, Block: 8x8, 32 blocks por page
            const uint32_t pagesPerRow = (width + 63) >> 6;     // /64 (ceil)
            const uint32_t pageX = x >> 6;
            const uint32_t pageY = y >> 5;                      // /32
            const uint32_t page  = pageX + pageY * pagesPerRow;

            const uint32_t ox = x & 0x3F;                       // x % 64
            const uint32_t oy = y & 0x1F;                       // y % 32

            const uint32_t blockX = ox >> 3;                    // /8
            const uint32_t blockY = oy >> 3;                    // /8
            const uint32_t block  = block32[blockY * 8 + blockX];

            const uint32_t col = ox & 0x7;                      // %8
            const uint32_t row = oy & 0x7;                      // %8

            const uint32_t pixelIndex =
                page * (64u * 32u) +
                block * (8u * 8u) +
                row * 8u + col;

            return pixelIndex * 4u;                             // bytes
        }

        case GS_PSM_16:
        case GS_PSM_16S:
        {
            // Page: 64x64, Block: 16x8, 32 blocks por page
            const uint32_t pagesPerRow = (width + 63) >> 6;     // /64 (ceil)
            const uint32_t pageX = x >> 6;
            const uint32_t pageY = y >> 6;                      // /64
            const uint32_t page  = pageX + pageY * pagesPerRow;

            const uint32_t ox = x & 0x3F;                       // x % 64
            const uint32_t oy = y & 0x3F;                       // y % 64

            const uint32_t blockX = ox >> 4;                    // /16  (0..3)
            const uint32_t blockY = oy >> 3;                    // /8   (0..7)
            const uint32_t block  = block16[blockY * 4 + blockX];

            const uint32_t col = ox & 0xF;                      // %16
            const uint32_t row = oy & 0x7;                      // %8

            const uint32_t pixelIndex =
                page * (64u * 64u) +
                block * (16u * 8u) +
                row * 16u + col;

            return pixelIndex * 2u;                             // bytes
        }

        case GS_PSM_8:
        case GS_PSM_8H:
        {
            // Page: 128x64, Block: 16x16, 32 blocks por page
            const uint32_t pagesPerRow = (width + 127) >> 7;    // /128 (ceil)
            const uint32_t pageX = x >> 7;
            const uint32_t pageY = y >> 6;                      // /64
            const uint32_t page  = pageX + pageY * pagesPerRow;

            const uint32_t ox = x & 0x7F;                       // x % 128
            const uint32_t oy = y & 0x3F;                       // y % 64

            const uint32_t blockX = ox >> 4;                    // /16 (0..7)
            const uint32_t blockY = oy >> 4;                    // /16 (0..3)
            const uint32_t block  = block8[blockY * 8 + blockX];

            const uint32_t col = ox & 0xF;                      // %16
            const uint32_t row = oy & 0xF;                      // %16

            const uint32_t pixelIndex =
                page * (128u * 64u) +
                block * (16u * 16u) +
                row * 16u + col;

            return pixelIndex;                                  // bytes (1 byte por pixel)
        }

        case GS_PSM_4:
        case GS_PSM_4HL:
        case GS_PSM_4HH:
        {
            // Page: 128x128, Block: 32x16, 32 blocks por page
            // 4bpp: 2 pixels por byte
            const uint32_t pagesPerRow = (width + 127) >> 7;    // /128 (ceil)
            const uint32_t pageX = x >> 7;
            const uint32_t pageY = y >> 7;                      // /128
            const uint32_t page  = pageX + pageY * pagesPerRow;

            const uint32_t ox = x & 0x7F;                       // x % 128
            const uint32_t oy = y & 0x7F;                       // y % 128

            const uint32_t blockX = ox >> 5;                    // /32 (0..3)
            const uint32_t blockY = oy >> 4;                    // /16 (0..7)
            const uint32_t block  = block4[blockY * 4 + blockX];

            const uint32_t col = ox & 0x1F;                     // %32
            const uint32_t row = oy & 0x0F;                     // %16

            const uint32_t pixelIndexInPage =
                block * (32u * 16u) +
                row * 32u + col;                                // em pixels (0..16383)

            const uint32_t pageByteBase = page * (128u * 128u / 2u); // 8192 bytes por page
            return pageByteBase + (pixelIndexInPage >> 1);      // bytes
        }

        default:
            // fallback conservador
            return (y * width + x) * 4u;
    }
}

// --------------------------------------------------------------------------------------
// Format Detection
// --------------------------------------------------------------------------------------

PS2_PSM TextureLoader::DetectPSM(size_t size, int& w, int& h, int& offset) {
    offset = 0;
    // Padrões Exatos
    if (size == 16384) { w=128; h=128; return GS_PSM_8; } // 8bpp
    if (size == 8192)  { w=64;  h=128; return GS_PSM_8; }
    if (size == 4096)  { w=64;  h=64;  return GS_PSM_8; }
    if (size == 2048)  { w=64;  h=64;  return GS_PSM_4; } // 4bpp
    
    // Headers 24 Bytes (Tim2 / OSD RAW)
    // 8bpp
    if (size == 16408) { w=128; h=128; offset=24; return GS_PSM_8; }
    if (size == 8216)  { w=64;  h=128; offset=24; return GS_PSM_8; }
    if (size == 32792) { w=256; h=128; offset=24; return GS_PSM_8; } // TEXBCDPB
    
    // [IMPORTANTE] TEXCKLGN (16-bit 256x256)
    // Tamanho do arquivo ~135KB - 138KB
    // Dados Pixels = 256*256*2 = 131072 bytes.
    if (size > 131072 && size < 145000) {
        w = 256; h = 256;
        // O excesso está geralmente no Header (começo)
        offset = (int)(size - 131072);
        return GS_PSM_16; 
    }
    
    // [IMPORTANTE] TEXCKABE (24-bit ou 32-bit Packed 64x64)
    // 64*64*3 = 12288
    // 64*64*4 = 16384 (Seria GS_PSM_32)
    // Se for 12KB, é Packed 24.
    if (size == 12288 || size == 12288+24) {
        w = 64; h = 64; offset = (size > 12288) ? 24 : 0;
        return GS_PSM_24; 
    }
    
    // Default safe square
    int sq = (int)sqrt(size);
    if (sq*sq == size) { w=sq; h=sq; return GS_PSM_8; }

    return GS_PSM_32; // Default falha
}

bool TextureLoader::LoadFromPath(const std::string& path, TexData& out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if(!f.is_open()) return false;
    size_t size = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> data(size);
    f.read((char*)data.data(), size);
    
    int w,h,off;
    out.originalPsm = DetectPSM(size, w, h, off);
    
    out.width = w;
    out.height = h;
    out.valid = true;
    
    // Verifica boundaries
    if (off + (w*h)/(out.originalPsm==GS_PSM_4 ? 2 : (out.originalPsm==GS_PSM_16?0.5:1)) > size) {
        // Se calculado mal, tenta sem offset
        off = 0;
    }
    
    const uint8_t* ptr = data.data() + off;

    // Redireciona para leitor correto
    switch(out.originalPsm) {
        case GS_PSM_16: out.format = TexFormat::RGBA16; return Read16(ptr, out);
        case GS_PSM_24: out.format = TexFormat::RGBA32; return Read32(ptr, out); // Treat 24 as 32 packer
        case GS_PSM_32: out.format = TexFormat::RGBA32; return Read32(ptr, out);
        case GS_PSM_8:  out.format = TexFormat::Indexed8; return Read8(ptr, out);
        case GS_PSM_4:  out.format = TexFormat::Indexed4; return Read4(ptr, out);
        default: return false;
    }
}

// --------------------------------------------------------------------------
// READERS (COM UNSWIZZLE)
// --------------------------------------------------------------------------

bool TextureLoader::Read16(const uint8_t* src, TexData& out) {
    out.pixels.resize(out.width * out.height * 4);
    
    // Verifica se realmente precisamos swizzler
    // Se o offset calculado foi > 0, geralmente o dump é linear?
    // Dumpers VRAM geralmente dumpam em ordem swizzled. Vamos tentar Unswizzle.
    
    // PS: A leitura de memória de TEXCKLGN anterior mostrava "padrões de blocos", o que
    // confirma que está Swizzled na memória de arquivo.
    
    for (int y = 0; y < out.height; y++) {
        for (int x = 0; x < out.width; x++) {
            // Get physical address in swizzled buffer
            uint32_t addr = GetGSAddress(x, y, out.width, GS_PSM_16);
            
            // Ler 2 bytes (short)
            uint16_t raw = *(uint16_t*)(src + addr);
            
            // A1 B5 G5 R5 (Standard PS2)
            uint8_t r = (raw & 0x1F) << 3;
            uint8_t g = (raw >> 5) & 0x1F; g <<= 3;
            uint8_t b = (raw >> 10) & 0x1F; b <<= 3;
            uint8_t a = (raw >> 15) ? 255 : 0; 
            
            // Fix Color Range
            r |= (r >> 5); g |= (g >> 5); b |= (b >> 5);
            
            if(a==0 && (r||g||b)) a=255; // Visible colors fix
            
            // Saida Linear OpenGL (y*w+x)
            int dstIdx = (y * out.width + x) * 4;
            out.pixels[dstIdx+0] = r;
            out.pixels[dstIdx+1] = g;
            out.pixels[dstIdx+2] = b;
            out.pixels[dstIdx+3] = a;
        }
    }
    return true;
}

bool TextureLoader::Read32(const uint8_t* src, TexData& out) {
    // RGB24 -> RGBA32
    // Se PSM24, 3 bytes. Se PSM32, 4 bytes.
    int bpp = (out.originalPsm == GS_PSM_24) ? 3 : 4;
    
    out.pixels.resize(out.width * out.height * 4);

    for (int y = 0; y < out.height; y++) {
        for (int x = 0; x < out.width; x++) {
            
            uint32_t addr;
            if (out.originalPsm == GS_PSM_24) {
               // RGB24 usually isn't swizzled complexly, often just Packed 
               // ou Linear se veio de dump convertido. Vamos tentar linear simples para RGB24.
               addr = (y * out.width + x) * 3;
            } else {
               addr = GetGSAddress(x, y, out.width, GS_PSM_32);
            }
            
            int dstIdx = (y * out.width + x) * 4;
            
            out.pixels[dstIdx+0] = src[addr+0];
            out.pixels[dstIdx+1] = src[addr+1];
            out.pixels[dstIdx+2] = src[addr+2];
            out.pixels[dstIdx+3] = (bpp==4) ? std::min((int)src[addr+3]*2, 255) : 255;
        }
    }
    return true;
}

// --------------------------------------------------------
// CLUT UTILS (Desembaralhar paleta)
// --------------------------------------------------------
void TextureLoader::UnswizzleClut(const uint8_t* rawPal, uint32_t* outPal32) {
    // O GS organiza a CLUT (256 cores) em blocos de 32 (Tile 8x2 na VRAM)
    // Em ordem de memória, os índices 0..31 são sequenciais? Não no CSM1.
    // Grupos de 8 ou 16 são trocados.
    
    // Tabela CSM1 Standard
    static const int csm1_map[32] = {
         0, 1, 2, 3, 4, 5, 6, 7, // 0-7
         16,17,18,19,20,21,22,23, // 8-15 (Offset +8 pixels em VRAM, que é +16 entries linearmente)
         8, 9,10,11,12,13,14,15,  // 16-23
         24,25,26,27,28,29,30,31  // 24-31
    };
    
    // Uma paleta tem 256 cores. São 8 grupos de 32 cores (blocos).
    // O desarranjo acontece DENTRO de cada bloco de 32.
    
    for(int b=0; b<8; b++) { // 8 blocos
        for(int i=0; i<32; i++) { // 32 cores por bloco
            int entryMem = b*32 + i;        // Onde lemos
            int entryReal = b*32 + csm1_map[i]; // Onde escrevemos (corrigido)
            
            // Mas espera, CSM1 divide a CLUT em 4 partes (4 x 64).
            // A lógica de CSA (Clut Buffer Offset) é chata. 
            // Tentativa Simplificada: Para RGBA32 Clut, apenas lê sequencial. 
            // SE a imagem ficar com cores trocadas, reativamos essa função com o mapa de indices.
            
            // Por enquanto, modo Passthrough
            const uint8_t* c = rawPal + entryMem*4;
            uint8_t a = (c[3] >= 128 ? 255 : c[3]*2);
            // PS2 Alpha
            outPal32[entryMem] = (a<<24) | (c[2]<<16) | (c[1]<<8) | c[0]; 
        }
    }
}

bool TextureLoader::Read8(const uint8_t* src, TexData& out) {
    bool hasClut = true; 
    // Em dumps OSDSYS como TEXBARRW, normalmente é RAW ALPHA (sem paleta).
    // Checamos tamanho para ver se sobra espaço
    // w*h + 1024
    // 64*128 + 1024 = 8192 + 1024 = 9216 bytes.
    // Arquivo: 8192 (+24 hdr). Sobrou 0 pra CLUT. -> Grayscale/Alpha.
    
    size_t needed = out.width * out.height;
    // O ptr 'src' já aponta depois do header. Vamos calcular espaço restante assumindo.
    // Isso requer acesso ao 'buffer original size' ou confiar que DetectPSM setou
    // offset corretamente para apontar os dados. 
    
    // Heuristic: se Width 64, Height 64 = 4096 bytes. Arquivo = 4120. Sem CLUT.
    // Logo, sem Clut.
    
    // Para simplificar, assumimos SEM CLUT para Indexed8 em OSD. 
    // Eles usam cores via Vertex Color e a textura é só mascara.
    // EXCECAO: Icones do Browser (ICOB*).
    
    hasClut = false; // Forçando visualização de mascara primeiro
    
    out.pixels.resize(out.width * out.height * 4);

    for (int y = 0; y < out.height; y++) {
        for (int x = 0; x < out.width; x++) {
            
            // Aqui é crucial usar Unswizzle para consertar listras
            uint32_t addr = GetGSAddress(x, y, out.width, GS_PSM_8);
            uint8_t val = src[addr];
            
            int dstIdx = (y * out.width + x) * 4;
            
            if (hasClut) {
                // Not implemented Clut yet
            } else {
                // Mascara Alpha Branca
                out.pixels[dstIdx+0] = 255;
                out.pixels[dstIdx+1] = 255;
                out.pixels[dstIdx+2] = 255;
                // PS2 range 0..128 para 255. Ou 0..255.
                out.pixels[dstIdx+3] = (val > 128) ? 255 : (val * 2);
            }
        }
    }
    return true;
}
bool hasHeader = false;

if (size >= 24) {
    uint32_t marker = *(uint32_t*)(data + 8);
    if (marker == 0x10) hasHeader = true;
}

offset = hasHeader ? 24 : 0;
tex.layout = hasHeader ? TexLayout::Swizzled : TexLayout::Linear;

bool TextureLoader::Read4(const uint8_t* src, TexData& out) {
    out.pixels.resize(out.width * out.height * 4);
    // 4bpp tem packing de 2 pixels/byte E swizzle.
    // Address em bytes.
    
    for (int y = 0; y < out.height; y++) {
        for (int x = 0; x < out.width; x++) {
            
            // Pega o endereço do BYTE onde está o nibble
            uint32_t addrByte = GetGSAddress(x/2, y, out.width/2, GS_PSM_4);
            // x/2 porque "largura de dados" é metade. Mas a tabela PSM4 deve cuidar disso.
            // Para simplicidade, usamos PSM8 width logic dividido.
            // Correcao: Implementar Block4 real ou usar Fallback linear.
            
            // FALLBACK Linear para 4bpp (geralmente arquivos pequenos não swizzlam)
            uint32_t linAddr = (y * out.width + x) / 2;
            
            uint8_t b = src[linAddr];
            // Se x par = Low Nibble. Impar = High.
            uint8_t v = (x % 2 == 0) ? (b & 0xF) : (b >> 4);
            v *= 17; // 0..15 -> 0..255
            
            int dst = (y * out.width + x) * 4;
            out.pixels[dst+0] = 255; 
            out.pixels[dst+1] = 255; 
            out.pixels[dst+2] = 255;
            out.pixels[dst+3] = v;
        }
    }
    return true;
}
