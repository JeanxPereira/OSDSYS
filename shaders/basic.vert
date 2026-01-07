#version 330 core
// ============================================================================
// basic.vert - OSDSYS Basic Vertex Shader
// Compatible with Renderer.cpp vertex format: position (3) + color (4)
// ============================================================================

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
