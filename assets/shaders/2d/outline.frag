#version 450 core

// Outline Effect Fragment Shader
// Renders a colored outline around non-transparent pixels
//
// Usage with SpriteMaterial:
//   material->setFloat("u_OutlineWidth", 2.0f);
//   material->setVector4("u_OutlineColor", {1.0f, 0.0f, 0.0f, 1.0f});

in vec4 v_Color;
in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform vec4 u_Color = vec4(1.0);

// Outline parameters
uniform float u_OutlineWidth = 1.0;       // Width in pixels
uniform vec4 u_OutlineColor = vec4(0.0, 0.0, 0.0, 1.0);  // Outline color
uniform vec2 u_TextureSize = vec2(64.0);  // Texture dimensions for pixel calculations

out vec4 o_Color;

void main() {
    vec4 texColor = texture(u_Texture, v_TexCoord);

    // If this pixel is transparent, check if it should be part of the outline
    if (texColor.a < 0.1) {
        // Sample neighboring pixels to detect edges
        vec2 pixelSize = u_OutlineWidth / u_TextureSize;

        float maxAlpha = 0.0;

        // Sample in 8 directions
        maxAlpha = max(maxAlpha, texture(u_Texture, v_TexCoord + vec2(-pixelSize.x, 0.0)).a);
        maxAlpha = max(maxAlpha, texture(u_Texture, v_TexCoord + vec2(pixelSize.x, 0.0)).a);
        maxAlpha = max(maxAlpha, texture(u_Texture, v_TexCoord + vec2(0.0, -pixelSize.y)).a);
        maxAlpha = max(maxAlpha, texture(u_Texture, v_TexCoord + vec2(0.0, pixelSize.y)).a);
        maxAlpha = max(maxAlpha, texture(u_Texture, v_TexCoord + vec2(-pixelSize.x, -pixelSize.y)).a);
        maxAlpha = max(maxAlpha, texture(u_Texture, v_TexCoord + vec2(pixelSize.x, -pixelSize.y)).a);
        maxAlpha = max(maxAlpha, texture(u_Texture, v_TexCoord + vec2(-pixelSize.x, pixelSize.y)).a);
        maxAlpha = max(maxAlpha, texture(u_Texture, v_TexCoord + vec2(pixelSize.x, pixelSize.y)).a);

        // If any neighbor is opaque, this is an outline pixel
        if (maxAlpha > 0.5) {
            o_Color = u_OutlineColor;
            return;
        }

        // Fully transparent - discard
        discard;
    }

    // Regular sprite rendering
    o_Color = texColor * v_Color * u_Color;
}
