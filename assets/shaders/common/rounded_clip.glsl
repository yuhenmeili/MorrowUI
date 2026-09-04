// 圆角裁剪：localPosition 为顶点局部坐标（中心为原点）。
void applyRoundedClip(vec2 displaySize, float rounding, vec3 localPosition) {
    if (rounding <= 0.0) {
        return;
    }
    vec3 center = vec3(displaySize.x / 2.0 - rounding, displaySize.y / 2.0 - rounding, localPosition.z);
    vec3 current = vec3(abs(localPosition));
    float distanceSq = dot(current - center, current - center);
    float roundingSq = rounding * rounding;
    if (current.x > center.x && current.y > center.y && distanceSq > roundingSq) {
        discard;
    }
}
