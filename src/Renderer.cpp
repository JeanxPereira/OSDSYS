#define OSDSYS_USE_OPENGL
#include "Platform.h"
#include <GL/glew.h>
#include "Renderer.h"
#include "Assets.h"
#include <cmath>
#include <vector>
#include <fstream>
#include <sstream>

// ============================================================================
// Texture Implementation
// ============================================================================
void Texture::Bind(uint32_t unit) const {
    if (valid) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }
}

void Texture::Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================================
// Shader Implementation
// ============================================================================
void Shader::Use() const {
    if (valid) {
        glUseProgram(id);
    }
}

void Shader::SetInt(const char* name, int value) const {
    glUniform1i(glGetUniformLocation(id, name), value);
}

void Shader::SetFloat(const char* name, float value) const {
    glUniform1f(glGetUniformLocation(id, name), value);
}

void Shader::SetVec3(const char* name, const Vec3& value) const {
    glUniform3f(glGetUniformLocation(id, name), value.x, value.y, value.z);
}

void Shader::SetVec3(const char* name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(id, name), x, y, z);
}

void Shader::SetVec4(const char* name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(id, name), x, y, z, w);
}

void Shader::SetMat4(const char* name, const float* matrix) const {
    glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, matrix);
}

void Shader::SetBool(const char* name, bool value) const {
    glUniform1i(glGetUniformLocation(id, name), value ? 1 : 0);
}

// ============================================================================
// Fallback Shaders (embedded)
// ============================================================================
static const char* fallbackVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;

out vec3 FragPos;
out vec4 VertexColor;

uniform mat4 uProjection;
uniform mat4 uView;

void main() {
    FragPos = aPos;
    VertexColor = aColor;
    gl_Position = uProjection * uView * vec4(aPos, 1.0);
}
)";

static const char* fallbackFragmentShader = R"(
#version 330 core
in vec3 FragPos;
in vec4 VertexColor;
out vec4 FragColor;

uniform bool fogEnabled;
uniform float fogDensity;
uniform vec3 fogColor;
uniform vec3 viewPos;

void main() {
    vec4 color = VertexColor;
    
    if (fogEnabled) {
        float distance = length(viewPos - FragPos);
        float fogFactor = exp(-fogDensity * distance);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        color.rgb = mix(fogColor, color.rgb, fogFactor);
    }
    
    FragColor = color;
}
)";

static const char* fallbackTextVertShader = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;

out vec2 TexCoord;
out vec4 VertexColor;

uniform mat4 uProjection;

void main() {
    TexCoord = aTexCoord;
    VertexColor = aColor;
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
}
)";

static const char* fallbackTextFragShader = R"(
#version 330 core
in vec2 TexCoord;
in vec4 VertexColor;
out vec4 FragColor;

uniform sampler2D uFontAtlas;

void main() {
    vec4 texColor = texture(uFontAtlas, TexCoord);
    FragColor = vec4(VertexColor.rgb, texColor.a * VertexColor.a);
}
)";

static const char* fallbackSpriteVertShader = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;

out vec2 TexCoord;
out vec4 VertexColor;

uniform mat4 uProjection;

void main() {
    TexCoord = aTexCoord;
    VertexColor = aColor;
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
}
)";

static const char* fallbackSpriteFragShader = R"(
#version 330 core
in vec2 TexCoord;
in vec4 VertexColor;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform bool uUseTexture;

void main() {
    if (uUseTexture) {
        vec4 texColor = texture(uTexture, TexCoord);
        FragColor = texColor * VertexColor;
    } else {
        FragColor = VertexColor;
    }
}
)";

// ============================================================================
// Renderer Implementation
// ============================================================================
Renderer::Renderer() {
    SetIdentityMatrix(projectionMatrix);
    SetIdentityMatrix(viewMatrix);
    SetIdentityMatrix(orthoMatrix);
    cameraPosition = Vec3(0, 0, 300);
}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Init() {
    printf("[Renderer] Initializing...\n");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

    if (!LoadShaders()) {
        printf("[Renderer] Failed to load shaders!\n");
        return false;
    }

    // Main VAO/VBO for 3D geometry
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    // Text VAO/VBO
    glGenVertexArrays(1, &textVao);
    glGenBuffers(1, &textVbo);

    // Set default projection (perspective)
    float aspect = 640.0f / 448.0f;
    SetProjection(45.0f, aspect, 0.1f, 1000.0f);
    
    // Set ortho matrix for 2D (PS2 resolution)
    SetOrthoMatrix(orthoMatrix, 0.0f, 640.0f, 448.0f, 0.0f, -1.0f, 1.0f);
    
    // Set default camera
    SetCamera(Vec3(0, 0, 300), Vec3(0, 0, 0));

    // Load bitmap font
    if (!LoadFont()) {
        printf("[Renderer] Warning: Font not loaded, text rendering disabled\n");
    }

    printf("[Renderer] Initialized successfully\n");
    return true;
}

void Renderer::Shutdown() {
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (ebo) { glDeleteBuffers(1, &ebo); ebo = 0; }
    if (textVao) { glDeleteVertexArrays(1, &textVao); textVao = 0; }
    if (textVbo) { glDeleteBuffers(1, &textVbo); textVbo = 0; }
    
    if (basicShader.valid) {
        glDeleteProgram(basicShader.id);
        basicShader.valid = false;
    }
    if (textShader.valid) {
        glDeleteProgram(textShader.id);
        textShader.valid = false;
    }
    if (spriteShader.valid) {
        glDeleteProgram(spriteShader.id);
        spriteShader.valid = false;
    }
    
    if (fontTexture.valid) {
        DeleteTexture(fontTexture);
    }
    
    // Clean up texture cache
    for (auto& pair : textureCache) {
        if (pair.second.valid) {
            glDeleteTextures(1, &pair.second.id);
        }
    }
    textureCache.clear();
}

bool Renderer::LoadFont() {
    printf("[Renderer] Loading font banks...\n");
    
    // Caminho da pasta de fontes
    if (!fontLoader.LoadAll("assets/fonts/")) {
        printf("[Renderer] Failed to load any fonts from assets/fonts/\n");
        return false;
    }

    // Mantemos compatibilidade carregando a textura MAIN (ASCII) na GPU para o sistema legado
    int atlasWidth = fontLoader.GetAtlasWidth();
    int atlasHeight = fontLoader.GetAtlasHeight();
    const auto& rawData = fontLoader.GetTextureData();
    
    if (rawData.empty()) return false;

    printf("[Renderer] Creating MAIN font texture %dx%d...\n", atlasWidth, atlasHeight);
    
    fontTexture = CreateTexture(
        rawData.data(),
        atlasWidth,
        atlasHeight,
        4
    );

    if (!fontTexture.valid) return false;

    fontLoaded = true;
    return true;
}

void Renderer::BeginFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::EndFrame() {
}

// ============================================================================
// Camera/View
// ============================================================================
void Renderer::SetCamera(const Vec3& position, const Vec3& target, const Vec3& up) {
    cameraPosition = position;
    SetLookAtMatrix(viewMatrix, position, target, up);
}

void Renderer::SetProjection(float fov, float aspect, float nearPlane, float farPlane) {
    SetPerspectiveMatrix(projectionMatrix, fov, aspect, nearPlane, farPlane);
}

void Renderer::SetOrtho(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
    SetOrthoMatrix(projectionMatrix, left, right, bottom, top, nearPlane, farPlane);
}

// ============================================================================
// Fog Control
// ============================================================================
void Renderer::SetFog(float density, const Vec3& color) {
    fogEnabled = true;
    fogDensity = density;
    fogColor = color;
}

void Renderer::DisableFog() {
    fogEnabled = false;
}

// ============================================================================
// 3D Drawing
// ============================================================================
void Renderer::DrawCube(const Vec3& position, const Vec3& scale, const Color& color, const Vec3& rotation) {
    float vertices[] = {
        -1.0f, -1.0f, -1.0f,   color.r, color.g, color.b, color.a,
         1.0f, -1.0f, -1.0f,   color.r, color.g, color.b, color.a,
         1.0f,  1.0f, -1.0f,   color.r, color.g, color.b, color.a,
        -1.0f,  1.0f, -1.0f,   color.r, color.g, color.b, color.a,
        -1.0f, -1.0f,  1.0f,   color.r, color.g, color.b, color.a,
         1.0f, -1.0f,  1.0f,   color.r, color.g, color.b, color.a,
         1.0f,  1.0f,  1.0f,   color.r, color.g, color.b, color.a,
        -1.0f,  1.0f,  1.0f,   color.r, color.g, color.b, color.a,
    };
    
    unsigned int indices[] = {
        0, 1, 2,  2, 3, 0,
        1, 5, 6,  6, 2, 1,
        5, 4, 7,  7, 6, 5,
        4, 0, 3,  3, 7, 4,
        3, 2, 6,  6, 7, 3,
        4, 5, 1,  1, 0, 4
    };
    
    float cosX = cosf(rotation.x), sinX = sinf(rotation.x);
    float cosY = cosf(rotation.y), sinY = sinf(rotation.y);
    float cosZ = cosf(rotation.z), sinZ = sinf(rotation.z);
    
    for (int i = 0; i < 8; i++) {
        float x = vertices[i*7 + 0] * scale.x;
        float y = vertices[i*7 + 1] * scale.y;
        float z = vertices[i*7 + 2] * scale.z;
        
        float xr = x * cosY + z * sinY;
        float zr = -x * sinY + z * cosY;
        x = xr; z = zr;
        
        float yr = y * cosX - z * sinX;
        zr = y * sinX + z * cosX;
        y = yr; z = zr;
        
        xr = x * cosZ - y * sinZ;
        yr = x * sinZ + y * cosZ;
        x = xr; y = yr;
        
        vertices[i*7 + 0] = x + position.x;
        vertices[i*7 + 1] = y + position.y;
        vertices[i*7 + 2] = z + position.z;
    }
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    basicShader.Use();
    basicShader.SetMat4("uProjection", projectionMatrix);
    basicShader.SetMat4("uView", viewMatrix);
    basicShader.SetBool("fogEnabled", fogEnabled);
    basicShader.SetFloat("fogDensity", fogDensity);
    basicShader.SetVec3("fogColor", fogColor);
    basicShader.SetVec3("viewPos", cameraPosition);
    
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

void Renderer::DrawMesh(const ICOBModel& mesh, const Vec3& position, const Vec3& scale, const Color& color, const Vec3& rotation) {
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        return;
    }

    std::vector<float> vertices;
    vertices.reserve(mesh.vertices.size() * 7);
    
    float cosX = cosf(rotation.x), sinX = sinf(rotation.x);
    float cosY = cosf(rotation.y), sinY = sinf(rotation.y);
    float cosZ = cosf(rotation.z), sinZ = sinf(rotation.z);
    
    for (const auto& v : mesh.vertices) {
        float x = v.position.x * scale.x;
        float y = v.position.y * scale.y;
        float z = v.position.z * scale.z;
        
        float xr = x * cosY + z * sinY;
        float zr = -x * sinY + z * cosY;
        x = xr; z = zr;
        
        float yr = y * cosX - z * sinX;
        zr = y * sinX + z * cosX;
        y = yr; z = zr;
        
        xr = x * cosZ - y * sinZ;
        yr = x * sinZ + y * cosZ;
        x = xr; y = yr;
        
        vertices.push_back(x + position.x);
        vertices.push_back(y + position.y);
        vertices.push_back(z + position.z);
        vertices.push_back(v.color.r * color.r);
        vertices.push_back(v.color.g * color.g);
        vertices.push_back(v.color.b * color.b);
        vertices.push_back(v.color.a * color.a);
    }
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint16_t), mesh.indices.data(), GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    basicShader.Use();
    basicShader.SetMat4("uProjection", projectionMatrix);
    basicShader.SetMat4("uView", viewMatrix);
    basicShader.SetBool("fogEnabled", fogEnabled);
    basicShader.SetFloat("fogDensity", fogDensity);
    basicShader.SetVec3("fogColor", fogColor);
    basicShader.SetVec3("viewPos", cameraPosition);
    
    glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indices.size(), GL_UNSIGNED_SHORT, 0);
}

void Renderer::DrawSphere(const Vec3& position, float radius, const Color& color, int segments) {
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    
    CreateSphereGeometry(vertices, indices, radius, segments);
    
    for (size_t i = 0; i < vertices.size(); i += 7) {
        vertices[i + 0] += position.x;
        vertices[i + 1] += position.y;
        vertices[i + 2] += position.z;
        vertices[i + 3] = color.r;
        vertices[i + 4] = color.g;
        vertices[i + 5] = color.b;
        vertices[i + 6] = color.a;
    }
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    basicShader.Use();
    basicShader.SetMat4("uProjection", projectionMatrix);
    basicShader.SetMat4("uView", viewMatrix);
    basicShader.SetBool("fogEnabled", fogEnabled);
    basicShader.SetFloat("fogDensity", fogDensity);
    basicShader.SetVec3("fogColor", fogColor);
    basicShader.SetVec3("viewPos", cameraPosition);
    
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
}

// ============================================================================
// 2D Drawing
// ============================================================================
void Renderer::DrawLine(const Vec3& start, const Vec3& end, const Color& color, float width) {
    float vertices[] = {
        start.x, start.y, start.z, color.r, color.g, color.b, color.a,
        end.x, end.y, end.z, color.r, color.g, color.b, color.a
    };
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    basicShader.Use();
    basicShader.SetMat4("uProjection", projectionMatrix);
    basicShader.SetMat4("uView", viewMatrix);
    basicShader.SetBool("fogEnabled", false);
    
    glLineWidth(width);
    glDrawArrays(GL_LINES, 0, 2);
    glLineWidth(1.0f);
}

void Renderer::DrawRect(float x, float y, float w, float h, const Color& color) {
    float vertices[] = {
        x, y, 0, color.r, color.g, color.b, color.a,
        x + w, y, 0, color.r, color.g, color.b, color.a,
        x + w, y + h, 0, color.r, color.g, color.b, color.a,
        x, y + h, 0, color.r, color.g, color.b, color.a
    };
    
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };
    
    glDisable(GL_DEPTH_TEST);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    basicShader.Use();
    basicShader.SetMat4("uProjection", orthoMatrix);
    
    float identityView[16];
    SetIdentityMatrix(identityView);
    basicShader.SetMat4("uView", identityView);
    basicShader.SetBool("fogEnabled", false);
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawSprite(const Texture& tex, float x, float y, float w, float h, const Color& tint) {
    if (!tex.valid || !spriteShader.valid) {
        // Fallback to colored rect
        DrawRect(x, y, w, h, tint);
        return;
    }

    // Vertex data: pos.xy, tex.uv, color.rgba
    float vertices[] = {
        // Bottom-left
        x,     y + h,  0.0f, 1.0f,  tint.r, tint.g, tint.b, tint.a,
        // Bottom-right
        x + w, y + h,  1.0f, 1.0f,  tint.r, tint.g, tint.b, tint.a,
        // Top-right
        x + w, y,      1.0f, 0.0f,  tint.r, tint.g, tint.b, tint.a,
        // Top-right (dup)
        x + w, y,      1.0f, 0.0f,  tint.r, tint.g, tint.b, tint.a,
        // Top-left
        x,     y,      0.0f, 0.0f,  tint.r, tint.g, tint.b, tint.a,
        // Bottom-left (dup)
        x,     y + h,  0.0f, 1.0f,  tint.r, tint.g, tint.b, tint.a
    };

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(textVao);  // Reuse text VAO since same format
    glBindBuffer(GL_ARRAY_BUFFER, textVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    // Stride: 8 floats
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    spriteShader.Use();
    spriteShader.SetMat4("uProjection", orthoMatrix);
    spriteShader.SetBool("uUseTexture", true);
    spriteShader.SetInt("uTexture", 0);

    tex.Bind(0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    tex.Unbind();

    glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawSprite(const std::string& texName, float x, float y, float w, float h, const Color& tint) {
    Texture tex = GetCachedTexture(texName);
    DrawSprite(tex, x, y, w, h, tint);
}

void Renderer::DrawText(const char* text, float x, float y, const Color& color, float scale) {
    if (!fontLoaded || !textShader.valid) {
        // Fallback: draw placeholder rectangles
        float charWidth = 8.0f * scale;
        float charHeight = 16.0f * scale;
        float spacing = 2.0f * scale;
        float curX = x;
        
        while (*text) {
            if (*text == ' ') {
                curX += charWidth + spacing;
                text++;
                continue;
            }
            if (*text == '\n') {
                curX = x;
                y += charHeight + spacing;
                text++;
                continue;
            }
            DrawRect(curX, y, charWidth, charHeight, color);
            curX += charWidth + spacing;
            text++;
        }
        return;
    }

    // Build vertex buffer for all characters
    std::vector<float> vertices;
    // Arredondar para pixels inteiros para alinhamento perfeito
    float curX = floorf(x);
    float curY = floorf(y);
    float glyphH = FontLoader::GLYPH_HEIGHT * scale;

    while (*text) {
        char c = *text++;
        
        if (c == '\n') {
            curX = x;
            curY += glyphH + 2.0f * scale;
            continue;
        }

        const FontGlyph& glyph = fontLoader.GetGlyph(c);
        float glyphW = glyph.width * scale;
        float advance = glyph.advance * scale;

        // Skip rendering for space but advance cursor
        if (c == ' ') {
            curX += floorf(advance);
            continue;
        }

        // Quad vertices: pos.x, pos.y, tex.u, tex.v, color.r, g, b, a
        // Bottom-left
        // Usar coordenadas arredondadas para alinhamento pixel-perfect
        float x0 = floorf(curX);
        float y0 = floorf(curY);
        float x1 = floorf(curX + glyphW);
        float y1 = floorf(curY + glyphH);
        
        vertices.push_back(x0);
        vertices.push_back(y1);
        vertices.push_back(glyph.u0);
        vertices.push_back(glyph.v1);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
        vertices.push_back(color.a);

        // Bottom-right
        vertices.push_back(x1);
        vertices.push_back(y1);
        vertices.push_back(glyph.u1);
        vertices.push_back(glyph.v1);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
        vertices.push_back(color.a);

        // Top-right
        vertices.push_back(x1);
        vertices.push_back(y0);
        vertices.push_back(glyph.u1);
        vertices.push_back(glyph.v0);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
        vertices.push_back(color.a);

        // Top-right (duplicate for second triangle)
        vertices.push_back(x1);
        vertices.push_back(y0);
        vertices.push_back(glyph.u1);
        vertices.push_back(glyph.v0);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
        vertices.push_back(color.a);

        // Top-left
        vertices.push_back(x0);
        vertices.push_back(y0);
        vertices.push_back(glyph.u0);
        vertices.push_back(glyph.v0);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
        vertices.push_back(color.a);

        // Bottom-left (duplicate for second triangle)
        vertices.push_back(x0);
        vertices.push_back(y1);
        vertices.push_back(glyph.u0);
        vertices.push_back(glyph.v1);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
        vertices.push_back(color.a);

        curX += floorf(advance);
    }

    if (vertices.empty()) {
        return;
    }

    // Render
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(textVao);
    glBindBuffer(GL_ARRAY_BUFFER, textVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    // Stride: 8 floats (pos.xy, tex.uv, color.rgba)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    textShader.Use();
    textShader.SetMat4("uProjection", orthoMatrix);
    textShader.SetInt("uFontAtlas", 0);

    fontTexture.Bind(0);

    size_t numVertices = vertices.size() / 8;
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)numVertices);

    fontTexture.Unbind();
    glEnable(GL_DEPTH_TEST);
}

float Renderer::GetTextWidth(const char* text, float scale) {
    if (!fontLoaded) {
        return strlen(text) * 10.0f * scale;
    }
    return fontLoader.GetTextWidth(text, scale);
}

// ============================================================================
// Debug Drawing
// ============================================================================
void Renderer::DrawDebugGrid(float size, int divisions) {
    Color gridColor(0.3f, 0.3f, 0.3f, 0.5f);
    float halfSize = size / 2.0f;
    float step = size / divisions;
    
    for (int i = 0; i <= divisions; i++) {
        float pos = -halfSize + i * step;
        DrawLine(Vec3(pos, 0, -halfSize), Vec3(pos, 0, halfSize), gridColor);
        DrawLine(Vec3(-halfSize, 0, pos), Vec3(halfSize, 0, pos), gridColor);
    }
}

void Renderer::DrawDebugAxis(float length) {
    DrawLine(Vec3(0, 0, 0), Vec3(length, 0, 0), Color(1.0f, 0.0f, 0.0f, 1.0f), 2.0f);
    DrawLine(Vec3(0, 0, 0), Vec3(0, length, 0), Color(0.0f, 1.0f, 0.0f, 1.0f), 2.0f);
    DrawLine(Vec3(0, 0, 0), Vec3(0, 0, length), Color(0.0f, 0.0f, 1.0f, 1.0f), 2.0f);
}

// ============================================================================
// Texture Management
// ============================================================================
Texture Renderer::LoadTexture(const std::string& path) {
    TexData texData;
    if (!textureLoader.LoadFromPath(path, texData)) {
        printf("[Renderer] Failed to load texture: %s\n", path.c_str());
        return Texture();
    }
    return CreateTexture(texData.pixels.data(), texData.width, texData.height, 4);
}

Texture Renderer::LoadTextureByName(const std::string& name) {
    TexData texData;
    if (!textureLoader.Load(name, texData)) {
        printf("[Renderer] Failed to load texture: %s\n", name.c_str());
        return Texture();
    }
    return CreateTexture(texData.pixels.data(), texData.width, texData.height, 4);
}

Texture Renderer::GetCachedTexture(const std::string& name) {
    auto it = textureCache.find(name);
    if (it != textureCache.end()) {
        return it->second;
    }
    Texture tex = LoadTextureByName(name);
    if (tex.valid) {
        textureCache[name] = tex;
    }
    return tex;
}

Texture Renderer::CreateTexture(const uint8_t* data, int width, int height, int channels) {
    Texture tex;
    tex.width = width;
    tex.height = height;
    
    printf("[CreateTexture] Creating %dx%d texture (%d channels)\n", width, height, channels);
    
    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    
    // Alinhamento forçado novamente para garantir
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLenum format = GL_RGBA;
    GLenum internalFormat = GL_RGBA;

    if (channels == 4) {
        format = GL_RGBA;
    } else if (channels == 3) {
        format = GL_RGB;
    } else if (channels == 1) {
        format = GL_RED;
        // Se estivermos usando Core Profile moderno, podemos precisar de um Swizzle
        // para fazer o canal R atuar como Alpha para a fonte
        GLint swizzleMask[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    }
    
    // IMPORTANTE: Usar as dimensões EXATAS passadas, sem padding!
    printf("[CreateTexture] glTexImage2D(%d, %d, %s)\n", width, height, 
           channels == 4 ? "RGBA" : channels == 3 ? "RGB" : "RED");
    
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    
    // Verifica erros de OpenGL
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("[CreateTexture] OpenGL error: 0x%X\n", error);
    }
    
    // Configuração para ficar "Crocante" igual PS2
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    tex.valid = true;
    printf("[CreateTexture] Texture created successfully (ID: %u)\n", tex.id);
    return tex;
}

void Renderer::DeleteTexture(Texture& tex) {
    if (tex.valid && tex.id) {
        glDeleteTextures(1, &tex.id);
        tex.id = 0;
        tex.valid = false;
    }
}

// ============================================================================
// Shader Management
// ============================================================================
bool Renderer::LoadShaders() {
    printf("[Renderer] Loading shaders...\n");
    
    // Load basic shader
    std::string vertSource = ReadShaderFile("shaders/basic.vert");
    std::string fragSource = ReadShaderFile("shaders/basic.frag");
    
    if (vertSource.empty()) {
        printf("[Renderer] Using fallback vertex shader\n");
        vertSource = fallbackVertexShader;
    }
    if (fragSource.empty()) {
        printf("[Renderer] Using fallback fragment shader\n");
        fragSource = fallbackFragmentShader;
    }
    
    uint32_t vertShader = CompileShader(vertSource.c_str(), GL_VERTEX_SHADER);
    if (vertShader == 0) return false;
    
    uint32_t fragShader = CompileShader(fragSource.c_str(), GL_FRAGMENT_SHADER);
    if (fragShader == 0) {
        glDeleteShader(vertShader);
        return false;
    }
    
    basicShader.id = glCreateProgram();
    glAttachShader(basicShader.id, vertShader);
    glAttachShader(basicShader.id, fragShader);
    
    if (!LinkProgram(basicShader.id)) {
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        glDeleteProgram(basicShader.id);
        return false;
    }
    
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    basicShader.valid = true;

    // Load text shader
    std::string textVertSource = ReadShaderFile("shaders/text.vert");
    std::string textFragSource = ReadShaderFile("shaders/text.frag");
    
    if (textVertSource.empty()) {
        printf("[Renderer] Using fallback text vertex shader\n");
        textVertSource = fallbackTextVertShader;
    }
    if (textFragSource.empty()) {
        printf("[Renderer] Using fallback text fragment shader\n");
        textFragSource = fallbackTextFragShader;
    }
    
    uint32_t textVertShader = CompileShader(textVertSource.c_str(), GL_VERTEX_SHADER);
    if (textVertShader == 0) {
        printf("[Renderer] Warning: Text vertex shader failed, text rendering disabled\n");
        return true; // Continue without text
    }
    
    uint32_t textFragShader = CompileShader(textFragSource.c_str(), GL_FRAGMENT_SHADER);
    if (textFragShader == 0) {
        glDeleteShader(textVertShader);
        printf("[Renderer] Warning: Text fragment shader failed, text rendering disabled\n");
        return true;
    }
    
    textShader.id = glCreateProgram();
    glAttachShader(textShader.id, textVertShader);
    glAttachShader(textShader.id, textFragShader);
    
    if (!LinkProgram(textShader.id)) {
        glDeleteShader(textVertShader);
        glDeleteShader(textFragShader);
        glDeleteProgram(textShader.id);
        printf("[Renderer] Warning: Text shader link failed\n");
        return true;
    }
    
    glDeleteShader(textVertShader);
    glDeleteShader(textFragShader);
    textShader.valid = true;

    // Load sprite shader
    std::string spriteVertSource = ReadShaderFile("shaders/sprite.vert");
    std::string spriteFragSource = ReadShaderFile("shaders/sprite.frag");
    
    if (spriteVertSource.empty()) {
        printf("[Renderer] Using fallback sprite vertex shader\n");
        spriteVertSource = fallbackSpriteVertShader;
    }
    if (spriteFragSource.empty()) {
        printf("[Renderer] Using fallback sprite fragment shader\n");
        spriteFragSource = fallbackSpriteFragShader;
    }
    
    uint32_t spriteVertShader = CompileShader(spriteVertSource.c_str(), GL_VERTEX_SHADER);
    if (spriteVertShader == 0) {
        printf("[Renderer] Warning: Sprite vertex shader failed\n");
        return true;
    }
    
    uint32_t spriteFragShader = CompileShader(spriteFragSource.c_str(), GL_FRAGMENT_SHADER);
    if (spriteFragShader == 0) {
        glDeleteShader(spriteVertShader);
        printf("[Renderer] Warning: Sprite fragment shader failed\n");
        return true;
    }
    
    spriteShader.id = glCreateProgram();
    glAttachShader(spriteShader.id, spriteVertShader);
    glAttachShader(spriteShader.id, spriteFragShader);
    
    if (!LinkProgram(spriteShader.id)) {
        glDeleteShader(spriteVertShader);
        glDeleteShader(spriteFragShader);
        glDeleteProgram(spriteShader.id);
        printf("[Renderer] Warning: Sprite shader link failed\n");
        return true;
    }
    
    glDeleteShader(spriteVertShader);
    glDeleteShader(spriteFragShader);
    spriteShader.valid = true;

    printf("[Renderer] All shaders loaded successfully\n");
    return true;
}

std::string Renderer::ReadShaderFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        printf("[Renderer] Could not open shader file: %s\n", path.c_str());
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

uint32_t Renderer::CompileShader(const char* source, uint32_t type) {
    uint32_t shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        printf("[Renderer] Shader compilation failed: %s\n", infoLog);
        return 0;
    }

    return shader;
}

bool Renderer::LinkProgram(uint32_t program) {
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        printf("[Renderer] Program linking failed: %s\n", infoLog);
        return false;
    }

    return true;
}

Shader Renderer::LoadShader(const std::string& vertPath, const std::string& fragPath) {
    Shader shader;
    
    std::string vertSource = ReadShaderFile(vertPath);
    std::string fragSource = ReadShaderFile(fragPath);
    
    if (vertSource.empty() || fragSource.empty()) {
        printf("[Renderer] Failed to read shader files\n");
        return shader;
    }
    
    uint32_t vertShader = CompileShader(vertSource.c_str(), GL_VERTEX_SHADER);
    if (vertShader == 0) return shader;
    
    uint32_t fragShader = CompileShader(fragSource.c_str(), GL_FRAGMENT_SHADER);
    if (fragShader == 0) {
        glDeleteShader(vertShader);
        return shader;
    }
    
    shader.id = glCreateProgram();
    glAttachShader(shader.id, vertShader);
    glAttachShader(shader.id, fragShader);
    
    if (!LinkProgram(shader.id)) {
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        glDeleteProgram(shader.id);
        return shader;
    }
    
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    
    shader.valid = true;
    return shader;
}

void Renderer::DeleteShader(Shader& shader) {
    if (shader.valid && shader.id) {
        glDeleteProgram(shader.id);
        shader.id = 0;
        shader.valid = false;
    }
}

// ============================================================================
// State Management
// ============================================================================
void Renderer::SetBlendMode(bool additive) {
    if (additive) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    } else {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
}

void Renderer::SetDepthTest(bool enabled) {
    if (enabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void Renderer::SetWireframe(bool enabled) {
    glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
}

// ============================================================================
// Matrix Helpers
// ============================================================================
void Renderer::SetIdentityMatrix(float* matrix) {
    memset(matrix, 0, 16 * sizeof(float));
    matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
}

void Renderer::SetPerspectiveMatrix(float* matrix, float fov, float aspect, float nearP, float farP) {
    float fovRad = fov * Math::PI / 180.0f;
    float f = 1.0f / tanf(fovRad / 2.0f);
    
    memset(matrix, 0, 16 * sizeof(float));
    matrix[0] = f / aspect;
    matrix[5] = f;
    matrix[10] = (farP + nearP) / (nearP - farP);
    matrix[11] = -1.0f;
    matrix[14] = (2.0f * farP * nearP) / (nearP - farP);
}

void Renderer::SetOrthoMatrix(float* matrix, float left, float right, float bottom, float top, float nearP, float farP) {
    memset(matrix, 0, 16 * sizeof(float));
    matrix[0] = 2.0f / (right - left);
    matrix[5] = 2.0f / (top - bottom);
    matrix[10] = -2.0f / (farP - nearP);
    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[14] = -(farP + nearP) / (farP - nearP);
    matrix[15] = 1.0f;
}

void Renderer::SetLookAtMatrix(float* matrix, const Vec3& eye, const Vec3& target, const Vec3& up) {
    Vec3 f = (target - eye).Normalize();
    Vec3 s = Vec3(f.y * up.z - f.z * up.y, f.z * up.x - f.x * up.z, f.x * up.y - f.y * up.x).Normalize();
    Vec3 u = Vec3(s.y * f.z - s.z * f.y, s.z * f.x - s.x * f.z, s.x * f.y - s.y * f.x);
    
    matrix[0] = s.x; matrix[4] = s.y; matrix[8] = s.z;
    matrix[1] = u.x; matrix[5] = u.y; matrix[9] = u.z;
    matrix[2] = -f.x; matrix[6] = -f.y; matrix[10] = -f.z;
    matrix[3] = 0.0f; matrix[7] = 0.0f; matrix[11] = 0.0f;
    matrix[12] = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
    matrix[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
    matrix[14] = (f.x * eye.x + f.y * eye.y + f.z * eye.z);
    matrix[15] = 1.0f;
}

void Renderer::MultiplyMatrices(float* result, const float* a, const float* b) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i * 4 + j] = 
                a[i * 4 + 0] * b[0 * 4 + j] +
                a[i * 4 + 1] * b[1 * 4 + j] +
                a[i * 4 + 2] * b[2 * 4 + j] +
                a[i * 4 + 3] * b[3 * 4 + j];
        }
    }
}

// ============================================================================
// Geometry Helpers
// ============================================================================
void Renderer::CreateSphereGeometry(std::vector<float>& vertices, std::vector<uint32_t>& indices, float radius, int segments) {
    vertices.clear();
    indices.clear();
    
    for (int lat = 0; lat <= segments; lat++) {
        float theta = lat * Math::PI / segments;
        float sinTheta = sinf(theta);
        float cosTheta = cosf(theta);
        
        for (int lon = 0; lon <= segments; lon++) {
            float phi = lon * 2.0f * Math::PI / segments;
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            vertices.push_back(x * radius);
            vertices.push_back(y * radius);
            vertices.push_back(z * radius);
            
            vertices.push_back(1.0f);
            vertices.push_back(1.0f);
            vertices.push_back(1.0f);
            vertices.push_back(1.0f);
        }
    }
    
    for (int lat = 0; lat < segments; lat++) {
        for (int lon = 0; lon < segments; lon++) {
            int first = lat * (segments + 1) + lon;
            int second = first + segments + 1;
            
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
}
