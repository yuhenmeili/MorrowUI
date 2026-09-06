precision highp float;

layout (location = 0) in vec2 v_texCoord;

uniform sampler2D u_texture;
uniform vec4 u_shadowColor;
uniform float u_alpha;
uniform float u_shadowBlur;     //模糊半径（px），0 = 硬阴影
uniform float u_shadowSpread;   //阴影相对字形的扩展/收缩（px）

layout (location = 0) out vec4 fragColor;

// SDF 重建参数（必须与 DynamicFont.h 中 SDF_ONEDGE / SDF_PIXEL_DIST_SCALE 一致）
const float SDF_EDGE = 0.70588f;       // 180 / 255
const float SDF_PX_PER_UNIT = 7.0833f; // 255 / 36

void main() {
    float d = (texture(u_texture, v_texCoord.st).r - SDF_EDGE) * SDF_PX_PER_UNIT;
    // spread 外扩：d - spread < 0 区域为阴影形状，blur 高斯衰减
    float dEff = d - u_shadowSpread;
    float t = max(dEff, 0.0) / max(u_shadowBlur, 0.001);
    float alpha = exp(-t * t * 3.0);
    fragColor = vec4(u_shadowColor.rgb, u_shadowColor.a * u_alpha * alpha);
}
