#version 450 core

// Dissolve Effect Fragment Shader
// Creates a dissolve/burn effect using noise pattern
//
// Usage with SpriteMaterial:
//   material->setFloat("u_DissolveAmount", 0.5f);  // 0.0 = fully visible, 1.0 = fully dissolved
//   material->setVector4("u_EdgeColor", {1.0f, 0.5f, 0.0f, 1.0f});  // Burn edge color
//   material->setFloat("u_EdgeWidth", 0.1f);

in vec4 v_Color;
in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform vec4 u_Color = vec4(1.0);

// Dissolve parameters
uniform float u_DissolveAmount = 0.0;     // 0.0 to 1.0 (0 = visible, 1 = dissolved)
uniform vec4 u_EdgeColor = vec4(1.0, 0.5, 0.0, 1.0);  // Color of dissolve edge
uniform float u_EdgeWidth = 0.05;         // Width of the colored edge
uniform float u_NoiseScale = 10.0;        // Scale of the noise pattern

out vec4 o_Color;

// Simple hash-based noise function (no external texture needed)
float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);

    // Four corners
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    // Smooth interpolation
    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

void main() {
    vec4 texColor = texture(u_Texture, v_TexCoord);

    // Generate noise value for this pixel
    float noiseValue = noise(v_TexCoord * u_NoiseScale);

    // Calculate dissolve threshold
    float threshold = u_DissolveAmount;

    // If noise is below threshold, pixel is dissolved
    if (noiseValue < threshold) {
        discard;
    }

    // Check if we're near the edge of dissolution
    float edgeStart = threshold;
    float edgeEnd = threshold + u_EdgeWidth;

    if (noiseValue < edgeEnd && noiseValue >= edgeStart) {
        // Blend between edge color and original color
        float edgeFactor = (noiseValue - edgeStart) / u_EdgeWidth;
        vec4 finalColor = texColor * v_Color * u_Color;
        o_Color = mix(u_EdgeColor, finalColor, edgeFactor);
        o_Color.a = texColor.a;
    } else {
        // Normal sprite rendering
        o_Color = texColor * v_Color * u_Color;
    }
}
