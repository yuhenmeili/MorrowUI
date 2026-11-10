// 上屏贴图片元着色器：把离屏内容 RT 原样采样输出。alpha 保持 straight
// 语义——与引擎直写窗口的 alpha 行为一致（EGLWindow 的 SCREEN_PRE_MULTIPLIED_
// ALPHA 合成语义与直写路径相同，不在此处额外预乘）。混合已关闭，
// letterbox 黑边由 clear 提供。
precision mediump float;

in vec2 v_uv;

uniform sampler2D u_texture;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(u_texture, v_uv);
}
