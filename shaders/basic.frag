#version 330 core
// ============================================================================
// basic.frag - OSDSYS Basic Fragment Shader
// Exponential fog matching PS2 hardware behavior
// ============================================================================

in vec3 FragPos;
in vec4 VertexColor;

out vec4 FragColor;

uniform bool fogEnabled;
uniform float fogDensity;
uniform vec3 fogColor;
uniform vec3 viewPos;

void main() {
    vec4 color = VertexColor;
    
    // Exponential fog (PS2 style)
    if (fogEnabled) {
        float distance = length(viewPos - FragPos);
        float fogFactor = exp(-fogDensity * distance);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        color.rgb = mix(fogColor, color.rgb, fogFactor);
    }
    
    FragColor = color;
}
