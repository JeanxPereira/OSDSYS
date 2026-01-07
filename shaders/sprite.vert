#version 330 core
// ============================================================================
// sprite.vert - OSDSYS Sprite Rendering Vertex Shader
// 2D orthographic projection for textured quads
// ============================================================================

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
