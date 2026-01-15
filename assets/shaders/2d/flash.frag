#version 450 core

// Flash/Hit Effect Fragment Shader
// Overlays a solid color on the sprite (useful for damage flash effects)
//
// Usage with SpriteMaterial:
//   material->setFloat("u_FlashAmount", 1.0f);  // 0.0 = normal, 1.0 = fully flashed
//   material->setVector4("u_FlashColor", {1.0f, 1.0f, 1.0f, 1.0f});  // Flash color

in vec4 v_Color;
in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform vec4 u_Color = vec4(1.0);

// Flash parameters
uniform float u_FlashAmount = 0.0;        // 0.0 to 1.0 (0 = normal, 1 = fully flashed)
uniform vec4 u_FlashColor = vec4(1.0);    // Color to flash to (default white)

out vec4 o_Color;

void main() {
    vec4 texColor = texture(u_Texture, v_TexCoord);

    // Apply base color tint
    vec4 baseColor = texColor * v_Color * u_Color;

    // Mix between base color and flash color based on flash amount
    // Keep the original alpha to preserve sprite shape
    vec3 flashedRGB = mix(baseColor.rgb, u_FlashColor.rgb, u_FlashAmount);

    o_Color = vec4(flashedRGB, baseColor.a);
}
