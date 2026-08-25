#include "DockDropOverlay.h"

#include <algorithm>

#include "base/MeshRenderer.h"
#include "base/Transform.h"
#include "elements/MRButton.h"

namespace {

std::shared_ptr<morrow::UIWidget> makeZone(const morrow::Math::Vector4& color) {
    auto zone = morrow::MRButton::create();
    zone->setInteractive(false);
    zone->setCornerRadius(2.0f);
    zone->setBackgroundColor(color);
    return zone;
}

}  // namespace

namespace morrow::editor {

std::shared_ptr<DockDropOverlay> DockDropOverlay::create() {
    auto overlay = std::shared_ptr<DockDropOverlay>(new DockDropOverlay());
    overlay->initializeZones();
    return overlay;
}

DockDropOverlay::DockDropOverlay() : UIWidget(false) {
    setWidgetType("EditorDockDropOverlay");
    setDisplayLayer(9);
}

void DockDropOverlay::initializeZones() {
    m_center = makeZone(Vector4(0.20f, 0.48f, 0.82f, 0.55f));
    m_left = makeZone(Vector4(0.16f, 0.38f, 0.72f, 0.55f));
    m_right = makeZone(Vector4(0.16f, 0.38f, 0.72f, 0.55f));
    m_top = makeZone(Vector4(0.16f, 0.38f, 0.72f, 0.55f));
    m_bottom = makeZone(Vector4(0.16f, 0.38f, 0.72f, 0.55f));
    addChild(m_center);
    addChild(m_left);
    addChild(m_right);
    addChild(m_top);
    addChild(m_bottom);
    setVisible(false);
}

void DockDropOverlay::setWorkspaceBounds(const Math::Rect& bounds) {
    m_workspaceBounds = bounds;
    getTransform()->setPosition(bounds.Min.x, bounds.Min.y, 0.0f);
    getTransform()->setSize(bounds.GetWidth(), bounds.GetHeight());
    layoutZones();
}

void DockDropOverlay::setZone(DockDropZone zone) {
    m_zone = zone;
    const Vector4 active(0.30f, 0.68f, 1.0f, 0.80f);
    const Vector4 normal(0.16f, 0.38f, 0.72f, 0.55f);
    m_center->getComponent<MeshRenderer>()->getMaterial()->setVector("color", zone == DockDropZone::Center ? active : normal);
    m_left->getComponent<MeshRenderer>()->getMaterial()->setVector("color", zone == DockDropZone::Left ? active : normal);
    m_right->getComponent<MeshRenderer>()->getMaterial()->setVector("color", zone == DockDropZone::Right ? active : normal);
    m_top->getComponent<MeshRenderer>()->getMaterial()->setVector("color", zone == DockDropZone::Top ? active : normal);
    m_bottom->getComponent<MeshRenderer>()->getMaterial()->setVector("color", zone == DockDropZone::Bottom ? active : normal);
}

DockDropZone DockDropOverlay::zone() const {
    return m_zone;
}

void DockDropOverlay::layoutZones() {
    const Vector3 size = getTransform()->getSize();
    const float edgeWidth = std::max(70.0f, size.x * 0.22f);
    const float edgeHeight = std::max(55.0f, size.y * 0.22f);
    const float centerWidth = std::max(100.0f, size.x * 0.36f);
    const float centerHeight = std::max(80.0f, size.y * 0.36f);
    m_center->getTransform()->setPosition((size.x - centerWidth) * 0.5f, (size.y - centerHeight) * 0.5f, 0.0f);
    m_center->getTransform()->setSize(centerWidth, centerHeight);
    m_left->getTransform()->setPosition(10.0f, (size.y - edgeHeight) * 0.5f, 0.0f);
    m_left->getTransform()->setSize(edgeWidth, edgeHeight);
    m_right->getTransform()->setPosition(size.x - edgeWidth - 10.0f, (size.y - edgeHeight) * 0.5f, 0.0f);
    m_right->getTransform()->setSize(edgeWidth, edgeHeight);
    m_top->getTransform()->setPosition((size.x - centerWidth) * 0.5f, 10.0f, 0.0f);
    m_top->getTransform()->setSize(centerWidth, edgeHeight);
    m_bottom->getTransform()->setPosition((size.x - centerWidth) * 0.5f, size.y - edgeHeight - 10.0f, 0.0f);
    m_bottom->getTransform()->setSize(centerWidth, edgeHeight);
}

}  // namespace morrow::editor
