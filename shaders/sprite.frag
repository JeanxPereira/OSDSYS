#version 330 core
// ============================================================================
// sprite.frag - OSDSYS Sprite Rendering Fragment Shader
// Samples texture with color tint
// ============================================================================

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
