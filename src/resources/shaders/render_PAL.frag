#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D Indexed_FRM;
uniform sampler2D Indexed_PAL;
uniform uint ColorPaletteUINT[256];

// Overlay layer system
const int MAX_OVERLAYS = 8;
uniform sampler2D Overlay0;
uniform sampler2D Overlay1;
uniform sampler2D Overlay2;
uniform sampler2D Overlay3;
uniform sampler2D Overlay4;
uniform sampler2D Overlay5;
uniform sampler2D Overlay6;
uniform sampler2D Overlay7;
uniform int overlay_count;
uniform int overlay_blend[MAX_OVERLAYS];   // 0=WHITE_MIX, 1=COLOR_MIX, 2=RGBA_BLEND
uniform vec4 overlay_color[MAX_OVERLAYS];

vec4 sample_overlay(int i, vec2 uv) {
    // GLSL 330 cannot dynamically index sampler arrays, so use if-chain
    if (i == 0) return texture(Overlay0, uv);
    if (i == 1) return texture(Overlay1, uv);
    if (i == 2) return texture(Overlay2, uv);
    if (i == 3) return texture(Overlay3, uv);
    if (i == 4) return texture(Overlay4, uv);
    if (i == 5) return texture(Overlay5, uv);
    if (i == 6) return texture(Overlay6, uv);
    if (i == 7) return texture(Overlay7, uv);
    return vec4(0);
}

void main()
{
    vec4 texel;
    vec4 index1 = texture(Indexed_FRM, vec2(TexCoord.x, TexCoord.y));
    vec4 index2 = texture(Indexed_PAL, vec2(TexCoord.x, TexCoord.y));

    if (index2.r == 0) {
        uint idx = uint(index1.r*255);
        uint color = ColorPaletteUINT[idx];
        texel = vec4(
            float(int(color >>  0) & 0xFF) / 255.0,    // r
            float(int(color >>  8) & 0xFF) / 255.0,    // g
            float(int(color >> 16) & 0xFF) / 255.0,    // b
            1.0
        );
    }
    else {
        uint idx = uint(index2.r*255);
        uint color = ColorPaletteUINT[idx];
        texel = vec4(
            float(int(color >>  0) & 0xFF) / 255.0,    // r
            float(int(color >>  8) & 0xFF) / 255.0,    // g
            float(int(color >> 16) & 0xFF) / 255.0,    // b
            1.0
        );
    }

    // Composite overlay layers
    float overlay_sum = 0.0;
    for (int i = 0; i < overlay_count && i < MAX_OVERLAYS; i++) {
        vec4 val = sample_overlay(i, TexCoord);

        if (overlay_blend[i] == 0) {
            // WHITE_MIX: mix with white where overlay > 0
            if (val.r > 0) {
                texel = mix(texel, vec4(1,1,1,1), overlay_color[i].a);
            }
        }
        else if (overlay_blend[i] == 1) {
            // COLOR_MIX: tint with layer color
            if (val.r > 0) {
                texel = mix(texel, vec4(overlay_color[i].rgb, 1.0), val.r * overlay_color[i].a);
            }
        }
        else if (overlay_blend[i] == 2) {
            // RGBA_BLEND: alpha-blend pre-rendered RGBA from CPU
            texel = mix(texel, vec4(val.rgb, 1.0), val.a);
        }

        overlay_sum += val.r;
    }

    //alpha channel background
    if ((index1.r + index2.r + overlay_sum) == 0) {
        float xTile = mod(TexCoord.x, 0.1);
        float yTile = mod(TexCoord.y, 0.1);
        if ((xTile < 0.05 && yTile < 0.05) || (xTile >= 0.05 && yTile >= 0.05))
        {
            FragColor = vec4(0.5,0.5,0.5,1.0);
        }
        else {
            FragColor = vec4(0.2,0.2,0.2,1.0);
        }
    }
    else {
    //forground
        FragColor = texel;
    }
}
