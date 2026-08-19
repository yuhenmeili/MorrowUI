#ifndef MORROW_GUI_MRPARTICLES2D_H
#define MORROW_GUI_MRPARTICLES2D_H

#include <cstdint>
#include <memory>
#include <random>
#include <vector>

#include "base/UIWidget.h"

namespace morrow {

class MRCPUParticles2D : public UIWidget {
public:
    /// 创建一个在 CPU 上模拟运动并生成动态网格的 2D 粒子组件。
    static std::shared_ptr<MRCPUParticles2D> create();

    /// 设置粒子数量，取值会限制在 1 到 4096 之间。
    void setAmount(size_t amount);

    /// 获取当前配置的粒子数量。
    size_t getAmount() const {
        return m_particles.size();
    }

    /// 设置单个粒子的生命周期，单位为秒。
    void setLifetime(float lifetime);

    /// 获取单个粒子的生命周期。
    float getLifetime() const {
        return m_lifetime;
    }

    /// 设置发射区域尺寸；粒子会在以组件中心为原点的矩形区域内生成。
    void setEmissionSize(const Vector2& size);

    /// 设置粒子初速度的随机最小值和最大值。
    void setVelocityRange(const Vector2& minimum, const Vector2& maximum);

    /// 设置作用于粒子的恒定加速度，例如重力。
    void setGravity(const Vector2& gravity);

    /// 设置粒子尺寸的随机最小值和最大值。
    void setSizeRange(float minimum, float maximum);

    /// 设置粒子从出生到消失的颜色渐变。
    void setColorGradient(const Vector4& startColor, const Vector4& endColor);

    /// 设置是否持续发射粒子；关闭后已有粒子会自然消失。
    void setEmitting(bool emitting);

    /// 获取当前是否持续发射粒子。
    bool isEmitting() const {
        return m_emitting;
    }

    /// 设置是否使用加法混合，适合发光、火花等效果。
    void setAdditiveBlend(bool additive);

    /// 重新生成全部粒子，并从初始状态开始播放。
    void restart();

    /// 每帧更新 CPU 粒子状态并提交动态粒子网格。
    void update(FrameStateSharedPtr frameState) override;

private:
    struct Particle {
        Vector2 position;
        Vector2 velocity;
        float age = 0.0f;
        float lifetime = 1.0f;
        float size = 8.0f;
        bool active = false;
    };

    MRCPUParticles2D();

    void resetParticle(Particle& particle, float initialAge = 0.0f);

    void rebuildMesh();

    float randomRange(float minimum, float maximum);

    std::vector<Particle> m_particles;
    std::mt19937 m_random{0x4D525043u};
    Vector2 m_emissionSize = Vector2(200.0f, 100.0f);
    Vector2 m_velocityMinimum = Vector2(-20.0f, -80.0f);
    Vector2 m_velocityMaximum = Vector2(20.0f, -30.0f);
    Vector2 m_gravity = Vector2(0.0f, 40.0f);
    Vector4 m_startColor = Vector4(1.0f, 0.8f, 0.25f, 1.0f);
    Vector4 m_endColor = Vector4(1.0f, 0.15f, 0.02f, 0.0f);
    float m_lifetime = 2.0f;
    float m_minimumSize = 6.0f;
    float m_maximumSize = 14.0f;
    bool m_emitting = true;
};

using MRCPUParticles2DSharedPtr = std::shared_ptr<MRCPUParticles2D>;

class MRGPUParticles2D : public UIWidget {
public:
    /// 创建一个由顶点着色器计算运动轨迹的 2D 粒子组件。
    static std::shared_ptr<MRGPUParticles2D> create();

    /// 设置粒子数量，修改后会重新生成静态粒子种子网格。
    void setAmount(size_t amount);

    /// 获取当前配置的粒子数量。
    size_t getAmount() const {
        return m_amount;
    }

    /// 设置单个粒子的生命周期，单位为秒。
    void setLifetime(float lifetime);

    /// 设置粒子发射矩形区域的尺寸。
    void setEmissionSize(const Vector2& size);

    /// 设置粒子初速度的随机最小值和最大值。
    void setVelocityRange(const Vector2& minimum, const Vector2& maximum);

    /// 设置 GPU 轨迹计算使用的恒定加速度。
    void setGravity(const Vector2& gravity);

    /// 设置所有 GPU 粒子的基础尺寸。
    void setParticleSize(float size);

    /// 设置粒子从出生到消失的颜色渐变。
    void setColorGradient(const Vector4& startColor, const Vector4& endColor);

    /// 设置是否播放粒子动画；暂停时保留当前画面。
    void setEmitting(bool emitting);

    /// 获取当前是否播放粒子动画。
    bool isEmitting() const {
        return m_emitting;
    }

    /// 设置是否使用加法混合，适合光点和氛围粒子。
    void setAdditiveBlend(bool additive);

    /// 将 GPU 粒子动画时间重置为零。
    void restart();

    /// 每帧推进 GPU 粒子时间参数，具体运动由着色器计算。
    void update(FrameStateSharedPtr frameState) override;

private:
    MRGPUParticles2D();

    void rebuildSeedMesh();

    void applyUniforms();

    size_t m_amount = 256;
    float m_time = 0.0f;
    float m_lifetime = 4.0f;
    float m_particleSize = 10.0f;
    Vector2 m_emissionSize = Vector2(300.0f, 160.0f);
    Vector2 m_velocityMinimum = Vector2(-10.0f, -45.0f);
    Vector2 m_velocityMaximum = Vector2(10.0f, -15.0f);
    Vector2 m_gravity = Vector2(0.0f, 18.0f);
    Vector4 m_startColor = Vector4(0.25f, 0.75f, 1.0f, 0.9f);
    Vector4 m_endColor = Vector4(0.15f, 0.25f, 1.0f, 0.0f);
    bool m_emitting = true;
};

using MRGPUParticles2DSharedPtr = std::shared_ptr<MRGPUParticles2D>;

}  // namespace morrow

#endif  // MORROW_GUI_MRPARTICLES2D_H
