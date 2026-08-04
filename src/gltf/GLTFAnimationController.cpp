//
// Created by lance on 2026/3/31.
//

#include "GLTFAnimationController.h"
#include "base/Widget.h"
#include "base/Transform.h"
#include "base/Transform3D.h"

#include <cmath>
#include <algorithm>

#include "GlobalObject.h"
#include "Log.h"

namespace morrow {
GLTFAnimationController::GLTFAnimationController() = default;

void GLTFAnimationController::init(const std::shared_ptr<GLTFScene>& scene,
                                   const std::shared_ptr<Widget>& sceneRoot) {
    m_scene = scene;
    m_nodeIndexToWidget.clear();

    if (sceneRoot) {
        // Walk the widget tree in the same depth-first order as GLTFSceneBuilder,
        // matching by widget name (GLTFNode::name → Widget::widgetName).
        // We build an index → widget map by walking node indices in order.
        std::function<void(const std::shared_ptr<Widget>&, int)> walk =
            [&](const std::shared_ptr<Widget>& w, int nodeIdx) {
            m_nodeIndexToWidget[nodeIdx] = w;
            // children follow in the same order as node.children
            if (!scene || nodeIdx >= int(scene->nodes.size())) return;
            const auto& node = scene->nodes[nodeIdx];
            int ci = 0;
            for (auto& child : w->m_children) {
                if (ci < int(node.children.size()))
                    walk(child, node.children[ci++]);
            }
        };

        int ci = 0;
        for (auto& child : sceneRoot->m_children) {
            if (m_scene && ci < int(m_scene->rootNodes.size()))
                walk(child, m_scene->rootNodes[ci++]);
        }
    }
}

void GLTFAnimationController::play(const std::string& name, bool loop) {
    if (!m_scene) return;
    for (int i = 0; i < int(m_scene->animations.size()); ++i) {
        if (m_scene->animations[i].name == name) {
            m_currentAnimIndex = i;
            m_loop = loop;
            m_paused = false;
            m_time = 0.0f;

            // Compute duration as the max input time across all samplers
            m_duration = 0.0f;
            for (const auto& s : m_scene->animations[i].samplers) {
                if (!s.input.empty())
                    m_duration = std::max(m_duration, s.input.back());
            }
            LOG_I("GLTFAnimationController: playing '{}' ({:.2f}s)", name, m_duration);
            REQUESTRENDER;
            return;
        }
    }
    LOG_W("GLTFAnimationController: animation '{}' not found", name);
}

void GLTFAnimationController::stop() {
    m_currentAnimIndex = -1;
    m_time = 0.0f;
    m_paused = false;
}

void GLTFAnimationController::setPaused(bool paused) {
    if (m_paused == paused) return;
    m_paused = paused;
    if (!m_paused && m_currentAnimIndex >= 0 && m_speed != 0.0f) {
        REQUESTRENDER;
    }
}

void GLTFAnimationController::setSpeed(float speed) {
    if (m_speed == speed) return;
    m_speed = speed;
    if (!m_paused && m_currentAnimIndex >= 0 && m_speed != 0.0f) {
        REQUESTRENDER;
    }
}

bool GLTFAnimationController::isPlaying() const {
    return m_currentAnimIndex >= 0 && !m_paused && m_speed != 0.0f && m_scene;
}

bool GLTFAnimationController::requiresContinuousUpdate() const {
    return isPlaying();
}

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------

void GLTFAnimationController::update(FrameStateSharedPtr frameState) {
    if (m_currentAnimIndex < 0 || m_paused || !m_scene) return;
    if (m_currentAnimIndex >= int(m_scene->animations.size())) return;

    const float dt = frameState ? float(frameState->deltaTime) : (1.0f / 60.0f);
    m_time += dt * m_speed;

    bool completed = false;
    if (m_duration > 0.0f) {
        if (m_loop) {
            m_time = std::fmod(m_time, m_duration);
        } else {
            m_time = std::min(m_time, m_duration);
            completed = m_time >= m_duration;
        }
    } else if (!m_loop) {
        completed = true;
    }

    const GLTFAnimation& anim = m_scene->animations[m_currentAnimIndex];
    for (const auto& channel : anim.channels) {
        if (channel.samplerIndex < 0 ||
            channel.samplerIndex >= int(anim.samplers.size()))
            continue;
        applyChannel(channel, anim.samplers[channel.samplerIndex], m_time);
    }

    if (completed) {
        m_currentAnimIndex = -1;
    } else if (m_speed != 0.0f) {
        REQUESTRENDER;
    }
}

// ---------------------------------------------------------------------------
// Channel application
// ---------------------------------------------------------------------------

static int findKeyframe(const std::vector<float>& times, float t) {
    for (int i = 0; i < int(times.size()) - 1; ++i) {
        if (t >= times[i] && t < times[i + 1]) return i;
    }
    return std::max(0, int(times.size()) - 2);
}

static float keyframeFraction(const std::vector<float>& times, int i, float t) {
    float span = times[i + 1] - times[i];
    return span > 0.0f ? (t - times[i]) / span : 0.0f;
}

void GLTFAnimationController::applyChannel(const GLTFAnimationChannel& channel,
                                           const GLTFAnimationSampler& sampler,
                                           float time) {
    auto it = m_nodeIndexToWidget.find(channel.nodeIndex);
    if (it == m_nodeIndexToWidget.end()) return;
    auto widget = it->second.lock();
    if (!widget) return;
    auto transform3D = widget->getComponent<Transform3D>();
    auto transform = widget->getComponent<Transform>();
    if (!transform3D && !transform) return;

    const auto& times = sampler.input;
    if (times.size() < 2) return;

    int i = findKeyframe(times, time);
    float f = keyframeFraction(times, i, time);

    switch (channel.path) {
        case GLTFAnimationPath::Translation: {
            Vector3 v = lerpVec3(sampler.output, i, f);
            if (transform3D) transform3D->setLocalPosition(v);
            else transform->setPosition(v);
            break;
        }
        case GLTFAnimationPath::Scale: {
            Vector3 v = lerpVec3(sampler.output, i, f);
            if (transform3D) transform3D->setLocalScale(v);
            else transform->setScale(v);
            break;
        }
        case GLTFAnimationPath::Rotation: {
            Quaternion q = lerpQuat(sampler.output, i, f);
            if (transform3D) transform3D->setLocalRotation(q);
            else transform->setRotation(q);
            break;
        }
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// Interpolation helpers
// ---------------------------------------------------------------------------

Vector3 GLTFAnimationController::lerpVec3(const std::vector<float>& output, int i, float f) {
    // 3 floats per keyframe
    Vector3 a(output[i * 3 + 0], output[i * 3 + 1], output[i * 3 + 2]);
    Vector3 b(output[(i + 1) * 3 + 0], output[(i + 1) * 3 + 1], output[(i + 1) * 3 + 2]);
    return Vector3(a.x + (b.x - a.x) * f,
                   a.y + (b.y - a.y) * f,
                   a.z + (b.z - a.z) * f);
}

Quaternion GLTFAnimationController::lerpQuat(const std::vector<float>& output, int i, float f) {
    // 4 floats per keyframe: x y z w
    Quaternion a(output[i * 4 + 0],
                 output[i * 4 + 1],
                 output[i * 4 + 2],
                 output[i * 4 + 3]);
    Quaternion b(output[(i + 1) * 4 + 0],
                 output[(i + 1) * 4 + 1],
                 output[(i + 1) * 4 + 2],
                 output[(i + 1) * 4 + 3]);
    // Basic nlerp (fast, good enough for Phase 3 prototype)
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (dot < 0.0f) {
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        b.w = -b.w;
    }
    Quaternion r(a.x + (b.x - a.x) * f,
                 a.y + (b.y - a.y) * f,
                 a.z + (b.z - a.z) * f,
                 a.w + (b.w - a.w) * f);
    float len = std::sqrt(r.x * r.x + r.y * r.y + r.z * r.z + r.w * r.w);
    if (len > 0.0f) {
        r.x /= len;
        r.y /= len;
        r.z /= len;
        r.w /= len;
    }
    return r;
}
} // namespace morrow
