#version 450 core

in vec4 v_Color;
in vec2 v_TexCoord;
in flat float v_TexIndex;
in float v_TilingFactor;

uniform sampler2D u_Textures[32];

out vec4 o_Color;

void main() {
    int index = int(v_TexIndex);
    vec4 texColor = texture(u_Textures[index], v_TexCoord * v_TilingFactor);
    o_Color = texColor * v_Color;
}
