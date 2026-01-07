#version 330 core

// Post-processing shader
// Fog + Bloom + Dithering (efeitos do PS2)

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform float time;

// Fog settings
uniform bool fogEnabled;
uniform float fogDensity;
uniform vec3 fogColor;

// Bloom settings
uniform bool bloomEnabled;
uniform float bloomIntensity;

// Dithering matrix 4x4 (PS2 style)
const float dither[16] = float[](
    0.0,  8.0,  2.0, 10.0,
    12.0, 4.0, 14.0,  6.0,
    3.0, 11.0,  1.0,  9.0,
    15.0, 7.0, 13.0,  5.0
);

float getDither(vec2 coord) {
    int x = int(mod(coord.x, 4.0));
    int y = int(mod(coord.y, 4.0));
    return dither[y * 4 + x] / 16.0;
}

void main() {
    vec4 color = texture(screenTexture, TexCoord);
    
    // Bloom (simple gaussian-like)
    if (bloomEnabled) {
        vec2 texelSize = 1.0 / textureSize(screenTexture, 0);
        vec4 bloom = vec4(0.0);
        
        // 3x3 kernel
        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                vec2 offset = vec2(float(x), float(y)) * texelSize;
                bloom += texture(screenTexture, TexCoord + offset);
            }
        }
        
        bloom /= 9.0;
        color = mix(color, bloom, bloomIntensity * 0.3);
    }
    
    // Dithering (PS2 style - opcional)
    float ditherValue = getDither(gl_FragCoord.xy);
    color.rgb += (ditherValue - 0.5) * 0.02; // Subtle dithering
    
    // Clamp
    color = clamp(color, 0.0, 1.0);
    
    FragColor = color;
}
