//
// SafeDynamicVectorCanvas.h — 安全动态矢量画布组件（HMI ADAS 专用）
//
// 设计目标：
//   预分配固定容量 VBO，运行时仅更新顶点数据（glBufferSubData），
//   不产生新的 GPU 资源分配。着色器源码硬编码于 .rodata 段。
//   用于智驾 ADAS 覆盖层：障碍物红框、车道线、倒车轨迹线等矢量图形。
//
// 渲染方式：
//   原生几何线条绘制（GL_LINES），每线段由两个顶点+颜色定义。
//   顶点数据每帧通过 setVertices/setColors 标记脏后自动上传 GPU。
//
// 典型用途：
//   - AsilAdasOverlay 障碍物检测包围框（动态红色矩形）
//   - 车道线渲染（虚线/实线）
//   - 倒车动态轨迹线（曲线逼近）
//
// 使用流程：
//   1. 创建 SafeDynamicVectorCanvas::create(maxVertices)
//   2. 每帧调用 clear() → drawXxx() → commit()
//   3. commit() 自动触发 requestRender，引擎渲染管线拾取绘制
//
// 性能特征：
//   - VBO 容量在构造时固定，超出容量的绘制调用被静默丢弃
//   - 无运行时堆分配（除首次构造）
//   - 顶点更新走 RecyclePool → VBOData 回收机制
//

#ifndef MORROW_SAFE_DYNAMIC_VECTOR_CANVAS_H
#define MORROW_SAFE_DYNAMIC_VECTOR_CANVAS_H

#include "base/UIWidget.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

#include <vector>
#include <memory>

namespace morrow {

// ---------------------------------------------------------------------------
// SafeDynamicVectorCanvas
// ---------------------------------------------------------------------------
class SafeDynamicVectorCanvas;
using SafeDynamicVectorCanvasSharedPtr = std::shared_ptr<SafeDynamicVectorCanvas>;

class SafeDynamicVectorCanvas : public UIWidget {
public:
    /// 工厂方法
    /// @param maxVertices  预分配的最大顶点数（每个线段 = 2 顶点；默认 4096）
    static SafeDynamicVectorCanvasSharedPtr create(int maxVertices = 4096);

    virtual ~SafeDynamicVectorCanvas() = default;

    // -----------------------------------------------------------------------
    // 每帧绘制接口（在 clear() 和 commit() 之间调用）
    // -----------------------------------------------------------------------

    /// 清空当前帧所有绘制命令，必须在每帧开始时调用
    void clear();

    /// 绘制单条线段
    /// @param x1, y1  起点（局部坐标：中心原点，X 右 Y 上）
    /// @param x2, y2  终点
    /// @param r, g, b, a  颜色（0~1）
    void drawLine(float x1, float y1, float x2, float y2,
                  float r, float g, float b, float a = 1.0f);

    /// 绘制矩形线框
    /// @param x, y  左上角坐标
    /// @param w, h  宽高
    void drawRect(float x, float y, float w, float h,
                  float r, float g, float b, float a = 1.0f);

    /// 绘制折线（连续线段）
    /// @param points  点数组
    /// @param count   点数（至少 2）
    void drawPolyline(const Math::Vector2* points, int count,
                      float r, float g, float b, float a = 1.0f);

    /// 绘制圆形逼近（用线段逼近圆周）
    /// @param cx, cy  圆心
    /// @param radius  半径
    /// @param segments  分段数（越大越圆滑，建议 32~64）
    void drawCircle(float cx, float cy, float radius, int segments,
                    float r, float g, float b, float a = 1.0f);

    /// 提交所有绘制命令到 GPU，触发渲染
    /// 必须在每帧所有 drawXxx() 调用后调用
    void commit();

    /// 获取当前已使用的顶点数
    int getVertexCount() const { return static_cast<int>(m_vertices.size()); }

    /// 获取最大顶点容量
    int getMaxVertices() const { return m_maxVertices; }

protected:
    explicit SafeDynamicVectorCanvas(int maxVertices);

    /// 初始化内置线框着色器（引擎内置，无文件 IO）
    void initShader();

    /// 安全地追加一对线段顶点（含容量检查）
    void addLineVertices(const Math::Vector3& p1, const Math::Vector3& p2,
                         const Math::Vector4& color);

private:
    int m_maxVertices;                      // 预分配最大顶点数
    std::vector<Math::Vector3> m_vertices; // 顶点位置缓冲
    std::vector<Math::Vector4> m_colors;   // 顶点颜色缓冲
};

} // namespace morrow

#endif // MORROW_SAFE_DYNAMIC_VECTOR_CANVAS_H
