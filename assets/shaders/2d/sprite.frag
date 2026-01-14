#version 450 core

// Basic Sprite Fragment Shader
// Simple textured sprite with color tint

in vec4 v_Color;
in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform vec4 u_Color = vec4(1.0);
uniform float u_TilingFactor = 1.0;

out vec4 o_Color;

void main() {
    vec4 texColor = texture(u_Texture, v_TexCoord * u_TilingFactor);
    o_Color = texColor * v_Color * u_Color;
}
