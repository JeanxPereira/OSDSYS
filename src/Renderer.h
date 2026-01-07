#pragma once
// ============================================================================
// Renderer.h - OSDSYS OpenGL Renderer
// 
// Abstracts OpenGL rendering with PS2-style fog, text, and primitives
// Based on OSDSYS rendering pipeline: Geometry → DMA → VU1 → GS
// ============================================================================

#include "MathTypes.h"
#include "FontLoader.h"
#include "TextureLoader.h"
#include <string>
#include <unordered_map>
#ifdef DrawText
    #undef DrawText
#endif

struct ICOBModel;

// ============================================================================
// Texture - OpenGL texture wrapper
// ============================================================================
struct Texture {
    uint32_t id = 0;
    int width = 0;
    int height = 0;
    bool valid = false;
    
    void Bind(uint32_t unit = 0) const;
    void Unbind() const;
};

// ============================================================================
// Shader - OpenGL shader program wrapper
// ============================================================================
struct Shader {
    uint32_t id = 0;
    bool valid = false;
    
    void Use() const;
    void SetInt(const char* name, int value) const;
    void SetFloat(const char* name, float value) const;
    void SetVec3(const char* name, const Vec3& value) const;
    void SetVec3(const char* name, float x, float y, float z) const;
    void SetVec4(const char* name, float x, float y, float z, float w) const;
    void SetMat4(const char* name, const float* matrix) const;
    void SetBool(const char* name, bool value) const;
};

// ============================================================================
// Renderer - Main rendering class
// ============================================================================
class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Init();
    void Shutdown();

    // Frame management
    void BeginFrame();
    void EndFrame();
    
    // Camera/View
    void SetCamera(const Vec3& position, const Vec3& target, const Vec3& up = Vec3(0,1,0));
    void SetProjection(float fov, float aspect, float nearPlane, float farPlane);
    void SetOrtho(float left, float right, float bottom, float top, float nearPlane, float farPlane);

    // Fog control (PS2 style exponential fog)
    void SetFog(float density, const Vec3& color);
    void DisableFog();

    // 3D Drawing
    void DrawCube(const Vec3& position, const Vec3& scale, const Color& color, const Vec3& rotation = Vec3(0,0,0));
    void DrawMesh(const ICOBModel& mesh, const Vec3& position, const Vec3& scale, const Color& color, const Vec3& rotation = Vec3(0,0,0));
    void DrawSphere(const Vec3& position, float radius, const Color& color, int segments = 16);

    // 2D Drawing (screen-space, PS2 resolution 640x448)
    void DrawLine(const Vec3& start, const Vec3& end, const Color& color, float width = 1.0f);
    void DrawRect(float x, float y, float w, float h, const Color& color);
    void DrawSprite(const Texture& tex, float x, float y, float w, float h, const Color& tint = Color(1.0f,1.0f,1.0f,1.0f));
    void DrawSprite(const std::string& texName, float x, float y, float w, float h, const Color& tint = Color(1.0f,1.0f,1.0f,1.0f));
    
    // Text rendering (uses FNTASCII bitmap font)
    void DrawText(const char* text, float x, float y, const Color& color, float scale = 1.0f);
    float GetTextWidth(const char* text, float scale = 1.0f);
    bool IsFontLoaded() const { return fontLoaded; }

    // Debug
    void DrawDebugGrid(float size = 100.0f, int divisions = 10);
    void DrawDebugAxis(float length = 50.0f);

    // Texture management
    Texture LoadTexture(const std::string& path);
    Texture LoadTextureByName(const std::string& name);  // Load from assets/textures/NAME.bin
    Texture CreateTexture(const uint8_t* data, int width, int height, int channels);
    void DeleteTexture(Texture& tex);
    Texture GetCachedTexture(const std::string& name);   // Get from cache or load
    
    // Shader management
    Shader LoadShader(const std::string& vertPath, const std::string& fragPath);
    void DeleteShader(Shader& shader);

    // State
    void SetBlendMode(bool additive);
    void SetDepthTest(bool enabled);
    void SetWireframe(bool enabled);

private:
    // OpenGL handles
    uint32_t vao = 0;
    uint32_t vbo = 0;
    uint32_t ebo = 0;
    
    // Text rendering VAO/VBO
    uint32_t textVao = 0;
    uint32_t textVbo = 0;
    
    // Shaders
    Shader basicShader;
    Shader textShader;
    
    // Matrices
    float projectionMatrix[16];
    float viewMatrix[16];
    float orthoMatrix[16];
    Vec3 cameraPosition;

    // Fog state
    bool fogEnabled = false;
    float fogDensity = 0.05f;
    Vec3 fogColor = Vec3(0.05f, 0.05f, 0.1f);

    // Font system
    FontLoader fontLoader;
    Texture fontTexture;
    bool fontLoaded = false;

    // Texture system
    TextureLoader textureLoader;
    std::unordered_map<std::string, Texture> textureCache;
    Shader spriteShader;

    // Helper methods
    bool LoadShaders();
    bool LoadFont();
    uint32_t CompileShader(const char* source, uint32_t type);
    bool LinkProgram(uint32_t program);
    std::string ReadShaderFile(const std::string& path);
    
    // Matrix helpers
    void SetIdentityMatrix(float* matrix);
    void SetPerspectiveMatrix(float* matrix, float fov, float aspect, float nearP, float farP);
    void SetOrthoMatrix(float* matrix, float left, float right, float bottom, float top, float nearP, float farP);
    void SetLookAtMatrix(float* matrix, const Vec3& eye, const Vec3& target, const Vec3& up);
    void MultiplyMatrices(float* result, const float* a, const float* b);
    
    // Geometry helpers
    void CreateSphereGeometry(std::vector<float>& vertices, std::vector<uint32_t>& indices, float radius, int segments);
};
