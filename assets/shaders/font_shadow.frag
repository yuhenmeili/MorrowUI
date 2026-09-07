precision highp float;

layout (location = 0) in vec2 v_texCoord;

uniform sampler2D u_texture;
uniform vec4 u_shadowColor;
uniform float u_alpha;
uniform float u_shadowBlur;     //模糊半径（px），0 = 硬阴影
uniform float u_shadowSpread;   //阴影相对字形的扩展/收缩（px）
uniform float u_sdfScale;       //目标字号 / 参考字号(44px)

layout (location = 0) out vec4 fragColor;

// SDF 重建参数（必须与 DynamicFont.h 中 SDF_ONEDGE / SDF_PIXEL_DIST_SCALE 一致）
const float SDF_EDGE = 0.50196f;        // 128 / 255
const float SDF_PX_PER_UNIT = 15.9375f; // 255 / 16

// erf 逼近（Abramowitz-Stegun 7.1.26，最大误差 ~1.5e-7）
float erf(float x) {
    const float t = 1.0 / (1.0 + 0.3275911 * abs(x));
    const float poly = t * (0.254829592 + t * (-0.284496736 + t * (1.421413741 + t * (-1.453152027 + t * 1.061405429))));
    return sign(x) * (1.0 - poly * exp(-x * x));
}

void main() {
    // 字体 SDF 约定：字形内部为正、外部为负（见 DynamicFont encodeSdfFromCoverage）。
    // 阴影形状 = 字形外扩 spread；衰减发生在形状边界向外（d 越负越远）。
    // 注意与 widget 阴影（sdRoundedBox，外部为正）的符号方向相反。
    float d = (texture(u_texture, v_texCoord.st).r - SDF_EDGE) * SDF_PX_PER_UNIT * u_sdfScale;
    float dOut = -(d + u_shadowSpread);              //距阴影形状边界的向外距离（px）
    // 高斯模糊剪影剖面（0.5*erfc）：边界处 50% 强度，向外长尾平滑衰减，
    // 视觉上 ~1.3*blur 内趋近 0。轮廓处不产生"满强度贴边壳"，观感更接近
    // 真实的柔和投影/柔光。
    float alpha = 0.5 * (1.0 - erf(dOut / (max(u_shadowBlur, 0.5) * 0.6)));
    fragColor = vec4(u_shadowColor.rgb, u_shadowColor.a * u_alpha * alpha);
}
