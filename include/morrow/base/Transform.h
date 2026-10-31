#ifndef MORROW_TRANSFORM_H
#define MORROW_TRANSFORM_H

#include "morrow/base/Component.h"
#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix4.h"
#include "Vector2.h"
#include "Vector4.h"

namespace morrow {
using namespace Math;
// 锚点定义，用于相对定位。锚点值为父矩形比例（0-1），
// 原点取父矩形左上角、Y 向下，与 UI 局部坐标系一致。
struct Anchor {
    Vector2 min; // 左上角锚点 (0-1 相对父 size)
    Vector2 max; // 右下角锚点 (0-1 相对父 size)

    Anchor() : min(0.0f, 0.0f), max(1.0f, 1.0f) {
    }

    Anchor(float minX, float minY, float maxX, float maxY) : min(minX, minY), max(maxX, maxY) {
    }

    Anchor(const Vector2& min, const Vector2& max) : min(min), max(max) {
    }

    // 常用锚点预设（左上原点、Y 向下语义）
    static Anchor TopLeft() { return Anchor(0.0f, 0.0f, 0.0f, 0.0f); }

    static Anchor TopCenter() { return Anchor(0.5f, 0.0f, 0.5f, 0.0f); }

    static Anchor TopRight() { return Anchor(1.0f, 0.0f, 1.0f, 0.0f); }

    static Anchor MiddleLeft() { return Anchor(0.0f, 0.5f, 0.0f, 0.5f); }

    static Anchor MiddleCenter() { return Anchor(0.5f, 0.5f, 0.5f, 0.5f); }

    static Anchor MiddleRight() { return Anchor(1.0f, 0.5f, 1.0f, 0.5f); }

    static Anchor BottomLeft() { return Anchor(0.0f, 1.0f, 0.0f, 1.0f); }

    static Anchor BottomCenter() { return Anchor(0.5f, 1.0f, 0.5f, 1.0f); }

    static Anchor BottomRight() { return Anchor(1.0f, 1.0f, 1.0f, 1.0f); }

    static Anchor StretchAll() { return Anchor(0.0f, 0.0f, 1.0f, 1.0f); }

    static Anchor StretchHorizontal() { return Anchor(0.0f, 0.5f, 1.0f, 0.5f); }

    static Anchor StretchVertical() { return Anchor(0.5f, 0.0f, 0.5f, 1.0f); }
};

class Transform : public Component {
public:
    Transform();

    ~Transform() override = default;

    // 位置相关方法
    void setPosition(float x, float y, float z);

    void setPosition(const Vector3& position);

    Vector3 getPosition() const;

    // 尺寸相关方法
    void setSize(float width, float height);

    void setSize(const Vector3& size);

    Vector3 getSize() const;

    Vector3 getCenter() const;

    // 缩放相关方法
    void setScale(float x, float y, float z);

    void setScale(const Vector3& scale);

    Vector3 getScale() const;

    // 旋转相关方法
    void setRotation(float x, float y, float z, float w);

    void setRotation(const Quaternion& rotation);

    void setRotation(const Vector3& axis, float angle);

    Quaternion getRotation() const;

    //pivot
    void setPivot(const Vector3& pivot);

    void setPivot(float x, float y, float z);

    Vector3 getPivot() const;

    // 锚点相关方法
    void setAnchor(const Anchor& anchor);

    void setAnchor(float minX, float minY, float maxX, float maxY);

    Anchor getAnchor() const;

    /// 边距（Godot 语义）：left/top/right/bottom 相对锚点的偏移。
    /// point 锚点时 left/top 为位置偏移；stretch 锚点时四值决定内缩。
    void setAnchorOffset(float left, float top, float right, float bottom);

    Vector4 getAnchorOffset() const;

    /// 锚点生效开关：默认关闭保持既有绝对布局行为；开启后每帧按父尺寸
    /// 重算 position/size（updateAnchoredPosition）。
    void setAnchorEnabled(bool enabled);

    bool isAnchorEnabled() const;

    /// 立即按当前锚点+父尺寸重算一次（场景实例化/属性应用后调用；
    /// 帧循环 update 内部也会自动调用）。
    void updateAnchoredPosition();

    // 父子关系相关方法
    // void setParent(Transform* parent);
    //
    // Transform* getParent() const;
    //
    // const std::vector<Transform*>& getChildren() const;

    // 矩阵相关方法
    const Matrix4& getLocalMatrix();

    const Matrix4& getWorldMatrix();

    // 组件生命周期方法重写
    void update(FrameStateSharedPtr frameState) override;

    void addSizeChangeListener(const std::function<void()>& listener);

    void notifySizeChange();

    /// 世界矩阵版本号：世界矩阵实际重算后递增，供子节点检测父链变化。
    uint64_t getWorldVersion() const { return m_worldVersion; }

    // 标记矩阵需要更新
    void setDirty();

private:
    /// 自身变换数据变化时调用：递增局部数据版本。
    void markDirty();

    void updateMatrix();

private:
    ///基础变换属性
    Vector3 m_localPosition; // 相对于锚点的局部位置
    Vector3 m_localScale;
    Vector3 m_size; // UI 元素的尺寸
    Quaternion m_localRotation;
    Anchor m_anchor;
    Vector4 m_offset;
    bool m_anchorEnabled = false;
    Vector3 m_pivotScreen = {0.5f, 0.5f, 0.0f};

    // 父子关系
    Transform* m_parent = nullptr;
    std::vector<Transform*> m_children;

    Matrix4 m_localMatrix;
    Matrix4 m_worldMatrix;

    /// Local transform data revision. Incremented whenever transform data changes.
    uint64_t m_localVersion = 1;
    /// Local data revision used to build m_localMatrix.
    uint64_t m_localMatrixVersion = 0;
    /// Local matrix revision used to build m_worldMatrix.
    uint64_t m_worldLocalMatrixVersion = 0;
    /// Incremented whenever m_worldMatrix is actually rebuilt.
    uint64_t m_worldVersion = 1;
    /// Parent identity and world revision used to build m_worldMatrix.
    Transform* m_cachedWorldParent = nullptr;
    uint64_t m_cachedParentWorldVersion = 0;

    std::vector<std::function<void()>> m_sizeChangeListeners;
};

using TransformSharedPtr = std::shared_ptr<Transform>;
} // namespace morrow

#endif // MORROW_TRANSFORM_H
