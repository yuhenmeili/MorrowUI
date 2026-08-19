#include "MRParticles2D.h"

#include <algorithm>
#include <cmath>

#include "base/Mesh.h"
#include "base/MeshFilter.h"
#include "base/Transform.h"

namespace morrow {
namespace {

Vector4 lerpColor(const Vector4& start, const Vector4& end, float t) {
    return Vector4(start.x + (end.x - start.x) * t, start.y + (end.y - start.y) * t, start.z + (end.z - start.z) * t, start.w + (end.w - start.w) * t);
}

void configureParticleBlend(const MaterialSharedPtr& material, bool additive) {
    material->setBlendEnabled(true);
    if (additive) {
        material->setBlendFunc(BlendFactor::SRC_ALPHA, BlendFactor::ONE, BlendFactor::ONE, BlendFactor::ONE_MINUS_SRC_ALPHA);
    } else {
        material->setBlendFunc(BlendFactor::SRC_ALPHA, BlendFactor::ONE_MINUS_SRC_ALPHA, BlendFactor::ONE, BlendFactor::ONE_MINUS_SRC_ALPHA);
    }
}

}  // namespace

std::shared_ptr<MRCPUParticles2D> MRCPUParticles2D::create() {
    auto particles = std::shared_ptr<MRCPUParticles2D>(new MRCPUParticles2D());
    particles->setAmount(128);
    return particles;
}

MRCPUParticles2D::MRCPUParticles2D() {
    setWidgetType("MRCPUParticles2D");
    m_material->setShader("particle_cpu");
    configureParticleBlend(m_material, true);
    getComponent<Transform>()->addSizeChangeListener([this]() { rebuildMesh(); });
}

void MRCPUParticles2D::setAmount(size_t amount) {
    amount = std::clamp<size_t>(amount, 1, 4096);
    m_particles.resize(amount);
    restart();
}

void MRCPUParticles2D::setLifetime(float lifetime) {
    m_lifetime = std::max(0.01f, lifetime);
    restart();
}

void MRCPUParticles2D::setEmissionSize(const Vector2& size) {
    m_emissionSize = Vector2(std::max(0.0f, size.x), std::max(0.0f, size.y));
    restart();
}

void MRCPUParticles2D::setVelocityRange(const Vector2& minimum, const Vector2& maximum) {
    m_velocityMinimum = Vector2(std::min(minimum.x, maximum.x), std::min(minimum.y, maximum.y));
    m_velocityMaximum = Vector2(std::max(minimum.x, maximum.x), std::max(minimum.y, maximum.y));
}

void MRCPUParticles2D::setGravity(const Vector2& gravity) {
    m_gravity = gravity;
}

void MRCPUParticles2D::setSizeRange(float minimum, float maximum) {
    m_minimumSize = std::max(0.1f, std::min(minimum, maximum));
    m_maximumSize = std::max(m_minimumSize, std::max(minimum, maximum));
}

void MRCPUParticles2D::setColorGradient(const Vector4& startColor, const Vector4& endColor) {
    m_startColor = startColor;
    m_endColor = endColor;
}

void MRCPUParticles2D::setEmitting(bool emitting) {
    m_emitting = emitting;
    requestRender("cpu particles emitting");
}

void MRCPUParticles2D::setAdditiveBlend(bool additive) {
    configureParticleBlend(m_material, additive);
}

void MRCPUParticles2D::restart() {
    for (size_t index = 0; index < m_particles.size(); ++index) {
        const float initialAge = m_particles.empty() ? 0.0f : m_lifetime * static_cast<float>(index) / static_cast<float>(m_particles.size());
        resetParticle(m_particles[index], initialAge);
    }
    rebuildMesh();
    requestRender("cpu particles restart");
}

void MRCPUParticles2D::update(FrameStateSharedPtr frameState) {
    const float deltaTime = frameState ? std::max(0.0f, static_cast<float>(frameState->deltaTime)) : 0.0f;
    bool hasActiveParticle = false;
    for (auto& particle : m_particles) {
        if (!particle.active)
            continue;
        hasActiveParticle = true;
        particle.age += deltaTime;
        if (particle.age >= particle.lifetime) {
            if (m_emitting) {
                resetParticle(particle);
            } else {
                particle.active = false;
            }
            continue;
        }
        particle.velocity.x += m_gravity.x * deltaTime;
        particle.velocity.y += m_gravity.y * deltaTime;
        particle.position.x += particle.velocity.x * deltaTime;
        particle.position.y += particle.velocity.y * deltaTime;
    }
    rebuildMesh();
    if (m_emitting || hasActiveParticle)
        requestRender("cpu particles update");
    UIWidget::update(frameState);
}

void MRCPUParticles2D::resetParticle(Particle& particle, float initialAge) {
    particle.position = Vector2(randomRange(-m_emissionSize.x * 0.5f, m_emissionSize.x * 0.5f), randomRange(-m_emissionSize.y * 0.5f, m_emissionSize.y * 0.5f));
    particle.velocity = Vector2(randomRange(m_velocityMinimum.x, m_velocityMaximum.x), randomRange(m_velocityMinimum.y, m_velocityMaximum.y));
    particle.age = std::clamp(initialAge, 0.0f, m_lifetime);
    particle.lifetime = m_lifetime;
    particle.size = randomRange(m_minimumSize, m_maximumSize);
    particle.active = true;
}

void MRCPUParticles2D::rebuildMesh() {
    std::vector<Vector3> vertices;
    std::vector<Vector2> uvs;
    std::vector<Vector4> colors;
    std::vector<int16_t> indices;
    vertices.reserve(m_particles.size() * 4);
    uvs.reserve(m_particles.size() * 4);
    colors.reserve(m_particles.size() * 4);
    indices.reserve(m_particles.size() * 6);

    for (const auto& particle : m_particles) {
        if (!particle.active)
            continue;
        const float halfSize = particle.size * 0.5f;
        const int16_t start = static_cast<int16_t>(vertices.size());
        vertices.emplace_back(particle.position.x - halfSize, particle.position.y + halfSize, 0.0f);
        vertices.emplace_back(particle.position.x + halfSize, particle.position.y + halfSize, 0.0f);
        vertices.emplace_back(particle.position.x + halfSize, particle.position.y - halfSize, 0.0f);
        vertices.emplace_back(particle.position.x - halfSize, particle.position.y - halfSize, 0.0f);
        uvs.emplace_back(0.0f, 0.0f);
        uvs.emplace_back(1.0f, 0.0f);
        uvs.emplace_back(1.0f, 1.0f);
        uvs.emplace_back(0.0f, 1.0f);
        const float progress = std::clamp(particle.age / particle.lifetime, 0.0f, 1.0f);
        const Vector4 color = lerpColor(m_startColor, m_endColor, progress);
        colors.insert(colors.end(), 4, color);
        indices.insert(indices.end(),
                       {start, static_cast<int16_t>(start + 1), static_cast<int16_t>(start + 2), static_cast<int16_t>(start + 2), static_cast<int16_t>(start + 3), start});
    }

    auto mesh = m_meshFilter->getMesh();
    mesh->setVertices(vertices);
    mesh->setUVs(uvs);
    mesh->setColors(colors);
    mesh->setIndices(indices);
}

float MRCPUParticles2D::randomRange(float minimum, float maximum) {
    std::uniform_real_distribution<float> distribution(minimum, maximum);
    return distribution(m_random);
}

std::shared_ptr<MRGPUParticles2D> MRGPUParticles2D::create() {
    auto particles = std::shared_ptr<MRGPUParticles2D>(new MRGPUParticles2D());
    particles->rebuildSeedMesh();
    particles->applyUniforms();
    return particles;
}

MRGPUParticles2D::MRGPUParticles2D() {
    setWidgetType("MRGPUParticles2D");
    m_material->setShader("particle_gpu");
    configureParticleBlend(m_material, true);
    getComponent<Transform>()->addSizeChangeListener([this]() { rebuildSeedMesh(); });
}

void MRGPUParticles2D::setAmount(size_t amount) {
    m_amount = std::clamp<size_t>(amount, 1, 4096);
    rebuildSeedMesh();
}

void MRGPUParticles2D::setLifetime(float lifetime) {
    m_lifetime = std::max(0.01f, lifetime);
    applyUniforms();
}

void MRGPUParticles2D::setEmissionSize(const Vector2& size) {
    m_emissionSize = Vector2(std::max(0.0f, size.x), std::max(0.0f, size.y));
    applyUniforms();
}

void MRGPUParticles2D::setVelocityRange(const Vector2& minimum, const Vector2& maximum) {
    m_velocityMinimum = Vector2(std::min(minimum.x, maximum.x), std::min(minimum.y, maximum.y));
    m_velocityMaximum = Vector2(std::max(minimum.x, maximum.x), std::max(minimum.y, maximum.y));
    applyUniforms();
}

void MRGPUParticles2D::setGravity(const Vector2& gravity) {
    m_gravity = gravity;
    applyUniforms();
}

void MRGPUParticles2D::setParticleSize(float size) {
    m_particleSize = std::max(0.1f, size);
    applyUniforms();
}

void MRGPUParticles2D::setColorGradient(const Vector4& startColor, const Vector4& endColor) {
    m_startColor = startColor;
    m_endColor = endColor;
    applyUniforms();
}

void MRGPUParticles2D::setEmitting(bool emitting) {
    m_emitting = emitting;
    requestRender("gpu particles emitting");
}

void MRGPUParticles2D::setAdditiveBlend(bool additive) {
    configureParticleBlend(m_material, additive);
}

void MRGPUParticles2D::restart() {
    m_time = 0.0f;
    m_material->setFloat("time", m_time);
    requestRender("gpu particles restart");
}

void MRGPUParticles2D::update(FrameStateSharedPtr frameState) {
    if (m_emitting && frameState) {
        m_time += std::max(0.0f, static_cast<float>(frameState->deltaTime));
        m_material->setFloat("time", m_time);
        requestRender("gpu particles update");
    }
    UIWidget::update(frameState);
}

void MRGPUParticles2D::rebuildSeedMesh() {
    std::mt19937 random(0x4D524750u);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::vector<Vector3> vertices;
    std::vector<Vector2> uvs;
    std::vector<Vector4> colors;
    std::vector<int16_t> indices;
    vertices.reserve(m_amount * 4);
    uvs.reserve(m_amount * 4);
    colors.reserve(m_amount * 4);
    indices.reserve(m_amount * 6);

    for (size_t index = 0; index < m_amount; ++index) {
        const int16_t start = static_cast<int16_t>(vertices.size());
        vertices.emplace_back(-0.5f, 0.5f, 0.0f);
        vertices.emplace_back(0.5f, 0.5f, 0.0f);
        vertices.emplace_back(0.5f, -0.5f, 0.0f);
        vertices.emplace_back(-0.5f, -0.5f, 0.0f);
        uvs.emplace_back(0.0f, 0.0f);
        uvs.emplace_back(1.0f, 0.0f);
        uvs.emplace_back(1.0f, 1.0f);
        uvs.emplace_back(0.0f, 1.0f);
        const Vector4 seed(unit(random), unit(random), unit(random), unit(random));
        colors.insert(colors.end(), 4, seed);
        indices.insert(indices.end(),
                       {start, static_cast<int16_t>(start + 1), static_cast<int16_t>(start + 2), static_cast<int16_t>(start + 2), static_cast<int16_t>(start + 3), start});
    }

    auto mesh = m_meshFilter->getMesh();
    mesh->setVertices(vertices);
    mesh->setUVs(uvs);
    mesh->setColors(colors);
    mesh->setIndices(indices);
    requestRender("gpu particles amount");
}

void MRGPUParticles2D::applyUniforms() {
    m_material->setFloat("time", m_time);
    m_material->setFloat("lifetime", m_lifetime);
    m_material->setFloat("particleSize", m_particleSize);
    m_material->setVector("emissionSize", m_emissionSize);
    m_material->setVector("velocityMin", m_velocityMinimum);
    m_material->setVector("velocityMax", m_velocityMaximum);
    m_material->setVector("gravity", m_gravity);
    m_material->setVector("startColor", m_startColor);
    m_material->setVector("endColor", m_endColor);
    requestRender("gpu particles uniforms");
}

}  // namespace morrow
