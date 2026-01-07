#version 330 core
// ============================================================================
// text.frag - OSDSYS Text Rendering Fragment Shader
// Samples font atlas texture with tint color
// ============================================================================

in vec2 TexCoord;
in vec4 VertexColor;

out vec4 FragColor;

uniform sampler2D uFontAtlas;

void main() {
    vec4 texColor = texture(uFontAtlas, TexCoord);
    FragColor = vec4(VertexColor.rgb, texColor.a * VertexColor.a);
}
