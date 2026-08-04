//
// Created by lance on 2026/3/31.
//

#ifndef MORROW_GUI_GLTFANIMATIONCONTROLLER_H
#define MORROW_GUI_GLTFANIMATIONCONTROLLER_H

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "GLTFTypes.h"
#include "base/Component.h"

namespace morrow {
class Widget;

/**
 * GLTFAnimationController – drives GLTF animation on a Widget subtree.
 *
 * Attach this Component to the scene root Widget returned by GLTFSceneBuilder.
 *
 * Usage:
 *   auto anim = sceneRoot->addComponent<GLTFAnimationController>();
 *   anim->init(scene, sceneRoot);
 *   anim->play("Run");
 */
class GLTFAnimationController : public Component {
public:
    GLTFAnimationController();

    ~GLTFAnimationController() override = default;

    /**
     * Bind the animation data and the live Widget subtree.
     * @param scene      The same GLTFScene used to build the subtree.
     * @param sceneRoot  Root Widget returned by GLTFSceneBuilder::build().
     */
    void init(const std::shared_ptr<GLTFScene>& scene,
              const std::shared_ptr<Widget>& sceneRoot);

    /// Start playing a named animation (loops by default).
    void play(const std::string& name, bool loop = true);

    /// Stop animation and reset to t=0.
    void stop();

    /// Pause / resume.
    void setPaused(bool paused);

    bool isPaused() const { return m_paused; }

    /// Speed multiplier (1.0 = normal speed).
    void setSpeed(float speed);

    float getSpeed() const { return m_speed; }

    // Component lifecycle
    void update(FrameStateSharedPtr frameState) override;

private:
    /// Sample a single channel at time t and apply to the target Widget.
    void applyChannel(const GLTFAnimationChannel& channel,
                      const GLTFAnimationSampler& sampler,
                      float time);

    /// Linear interpolation helpers
    static Vector3 lerpVec3(const std::vector<float>& output, int i, float t);

    static Quaternion lerpQuat(const std::vector<float>& output, int i, float t);

    /// Map GLTF node index → Widget in the live subtree (built during init)
    void buildNodeMap(const std::shared_ptr<Widget>& root, int depth = 0);

private:
    std::shared_ptr<GLTFScene> m_scene;
    std::unordered_map<int, std::weak_ptr<Widget>> m_nodeIndexToWidget;

    int m_currentAnimIndex = -1;
    float m_time = 0.0f;
    float m_speed = 1.0f;
    float m_duration = 0.0f;
    bool m_paused = false;
    bool m_loop = true;
};
} // namespace morrow

#endif //MORROW_GUI_GLTFANIMATIONCONTROLLER_H