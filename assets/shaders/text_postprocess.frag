in vec2 v_texCoord;
in vec4 v_color;

uniform sampler2D u_texture;
uniform float u_alpha;
uniform vec2 u_displaySize;
uniform vec4 u_blurKernal;
uniform vec4 u_textColor;
uniform float u_shadowAlpha;

layout(location = 0) out vec4 o_fragColor;

void main()
{
    float alpha = texture(u_texture, v_texCoord).a;
    vec4 currentColor = alpha == 0.0 ? vec4(0.0) : vec4(u_textColor.rgb, alpha);
    if (currentColor.a >= 0.4){
        o_fragColor = currentColor;
    }
    else {
        vec2 offset = vec2(1.0/u_displaySize.x, 1.0/u_displaySize.y) * 2.0;
        vec4 sum = vec4(0.0);
        int minX = int(u_blurKernal.x);
        int maxX = int(u_blurKernal.y);
        //Y flip
        int minY = -int(u_blurKernal.w);
        int maxY = -int(u_blurKernal.z);
        for (int i = minX; i < maxX; i++) {
            for (int j = minY; j < maxY; j++){
                vec2 uv = clamp(v_texCoord + offset * vec2(i, j), 0.0, 1.0);
                sum += texture(u_texture, uv);
            }
        }
        sum /= float((maxX - minX) * (maxY - minY));
        o_fragColor = vec4(0.0, 0.0, 0.0, sum.a * u_shadowAlpha);
    }
    o_fragColor.a *= u_alpha;
}