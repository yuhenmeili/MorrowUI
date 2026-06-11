//
// SafeDynamicVectorCanvas.cpp — 安全动态矢量画布实现
//
// 着色器源码硬编码于只读内存段（.rodata），不依赖文件 IO。
// VBO 容量在构造时预分配，运行时仅更新顶点数据，不产生新 GPU 分配。
//

#include "SafeDynamicVectorCanvas.h"

#include "base/Mesh.h"
#include "base/MeshFilter.h"
#include "Material.h"
#include "utils/Log.h"

#include <cmath>
#include <cstring>

namespace morrow {

using namespace Math;

// =========================================================================
// 1. 硬编码着色器源码（引擎内置 ROM .rodata）
//    顶点着色器：位置 + 颜色 → MVP 变换
//    片元着色器：直接输出顶点颜色
// =========================================================================
#ifdef OPENGL_EGL
static const char kVectorCanvasVert[] = R"GLSL(#version 320 es
layout (location = 0) in vec3 a_position;
layout (location = 3) in vec4 a_color;

uniform mat4 u_mvp;

out vec4 v_color;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_color = a_color;
}
)GLSL";

static const char kVectorCanvasFrag[] = R"GLSL(#version 320 es
in vec4 v_color;

layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = v_color;
}
)GLSL";
#else
static const char kVectorCanvasVert[] = R"GLSL(#version 460 core
layout (location = 0) in vec3 a_position;
layout (location = 3) in vec4 a_color;

uniform mat4 u_mvp;

out vec4 v_color;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_color = a_color;
}
)GLSL";

static const char kVectorCanvasFrag[] = R"GLSL(#version 460 core
in vec4 v_color;

layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = v_color;
}
)GLSL";
#endif

// =========================================================================
// SafeDynamicVectorCanvas 实现
// =========================================================================

SafeDynamicVectorCanvasSharedPtr SafeDynamicVectorCanvas::create(int maxVertices) {
    return SafeDynamicVectorCanvasSharedPtr(new SafeDynamicVectorCanvas(maxVertices));
}

SafeDynamicVectorCanvas::SafeDynamicVectorCanvas(int maxVertices)
    : m_maxVertices(maxVertices) {
    m_widgetType = "SafeDynamicVectorCanvas";

    // 预分配 CPU 端顶点缓冲
    m_vertices.reserve(maxVertices);
    m_colors.reserve(maxVertices);

    // 将默认 Quad Mesh 改为线框模式，清除三角形索引和 UV（否则引擎用索引渲染导致顶点错乱）
    auto mesh = m_meshFilter->getMesh();
    mesh->setDrawMode(PrimitiveType::LINES);
    mesh->setIndices({});   // 清除 Quad 的 6 个三角形索引 → 使用 glDrawArrays
    mesh->setUVs({});       // 清除 Quad 的 UV 数据 → VBO 中 location 2 留给 color

    // 注入内置着色器
    initShader();
}

// -------------------------------------------------------------------------
// 着色器初始化
// -------------------------------------------------------------------------
void SafeDynamicVectorCanvas::initShader() {
    m_material->setShaderFromMemory(
        "safe_dynamic_vector",
        std::string(kVectorCanvasVert),
        std::string(kVectorCanvasFrag)
    );
    m_material->setBlendEnabled(true);
}

// -------------------------------------------------------------------------
// 每帧绘制命令
// -------------------------------------------------------------------------

void SafeDynamicVectorCanvas::clear() {
    m_vertices.clear();
    m_colors.clear();
}

void SafeDynamicVectorCanvas::addLineVertices(const Vector3& p1, const Vector3& p2,
                                               const Vector4& color) {
    if (static_cast<int>(m_vertices.size()) + 2 > m_maxVertices) {
        LOG_W("SafeDynamicVectorCanvas: max vertices ({}) exceeded, skipping draw",
              m_maxVertices);
        return;
    }
    m_vertices.push_back(p1);
    m_vertices.push_back(p2);
    m_colors.push_back(color);
    m_colors.push_back(color);
}

void SafeDynamicVectorCanvas::drawLine(float x1, float y1, float x2, float y2,
                                        float r, float g, float b, float a) {
    Vector4 color(r, g, b, a);
    addLineVertices(Vector3(x1, y1, 0.0f), Vector3(x2, y2, 0.0f), color);
}

void SafeDynamicVectorCanvas::drawRect(float x, float y, float w, float h,
                                        float r, float g, float b, float a) {
    float x2 = x + w;
    float y2 = y + h;
    // 上边
    drawLine(x,  y,  x2, y,  r, g, b, a);
    // 右边
    drawLine(x2, y,  x2, y2, r, g, b, a);
    // 下边
    drawLine(x2, y2, x,  y2, r, g, b, a);
    // 左边
    drawLine(x,  y2, x,  y,  r, g, b, a);
}

void SafeDynamicVectorCanvas::drawPolyline(const Vector2* points, int count,
                                            float r, float g, float b, float a) {
    if (!points || count < 2) return;
    Vector4 color(r, g, b, a);
    for (int i = 0; i < count - 1; ++i) {
        addLineVertices(
            Vector3(points[i].x, points[i].y, 0.0f),
            Vector3(points[i + 1].x, points[i + 1].y, 0.0f),
            color
        );
    }
}

void SafeDynamicVectorCanvas::drawCircle(float cx, float cy, float radius, int segments,
                                          float r, float g, float b, float a) {
    if (segments < 3) return;
    Vector4 color(r, g, b, a);
    const float step = 2.0f * 3.14159265f / static_cast<float>(segments);

    Vector3 firstPt(cx + radius, cy, 0.0f);
    Vector3 prevPt = firstPt;

    for (int i = 1; i <= segments; ++i) {
        float angle = step * static_cast<float>(i);
        Vector3 pt(cx + radius * std::cos(angle),
                    cy + radius * std::sin(angle),
                    0.0f);
        addLineVertices(prevPt, pt, color);
        prevPt = pt;
    }
}

// -------------------------------------------------------------------------
// 提交 GPU 并触发渲染
// -------------------------------------------------------------------------
void SafeDynamicVectorCanvas::commit() {
    if (m_vertices.empty()) return;

    // 直接更新 Mesh 的顶点和颜色数据（自动标记脏，引擎渲染管线下一帧上传）
    auto mesh = m_meshFilter->getMesh();
    mesh->setVertices(m_vertices);
    mesh->setColors(m_colors);

    requestRender("SafeDynamicVectorCanvas::commit");
}

} // namespace morrow
