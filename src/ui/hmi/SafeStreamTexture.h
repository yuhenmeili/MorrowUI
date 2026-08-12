//
// SafeStreamTexture.h — 安全流媒体纹理组件（RVC/CMS 视频流）
//
// 设计目标：
//   预分配 GPU 纹理，运行时仅更新纹理像素数据（glTexSubImage2D），
//   不重新分配 GPU 资源。QNX 环境下通过 EGLImage 零拷贝硬解码。
//   Windows/Linux 环境通过 updateFrame() 直接写入 RGBA 像素。
//
// 典型用途：
//   - SafetyRvcPlayer 倒车影像
//   - CMS 电子后视镜
//   - 车载 DVR 实时画面
//
// 使用流程：
//   1. SafeStreamTexture::create(width, height)
//   2. 每帧调用 updateFrame(rgbaData) 或 updateFromEGLImage(image)
//   3. 引擎自动渲染（复用默认 Quad Mesh + UV）
//
// 条件编译：
//   OPENGL_EGL 宏由 CMake 在 QNX 平台自动定义
//

#ifndef MORROW_SAFE_STREAM_TEXTURE_H
#define MORROW_SAFE_STREAM_TEXTURE_H

#include "base/UIWidget.h"
#include "Texture.h"

#include <functional>
#include <memory>

namespace morrow {

// ---------------------------------------------------------------------------
// SafeStreamTexture
// ---------------------------------------------------------------------------
class SafeStreamTexture;
using SafeStreamTextureSharedPtr = std::shared_ptr<SafeStreamTexture>;

class SafeStreamTexture : public UIWidget {
public:
    /// 工厂方法
    /// @param width  纹理宽度（像素）
    /// @param height 纹理高度（像素）
    static SafeStreamTextureSharedPtr create(int width, int height);

    virtual ~SafeStreamTexture() = default;

    // -----------------------------------------------------------------------
    // 帧更新接口
    // -----------------------------------------------------------------------

    /// 更新纹理像素数据（RGBA8 格式）
    /// @param rgbaData  像素数据首地址（width × height × 4 字节）
    void updateFrame(const unsigned char* rgbaData);

    /// 获取纹理宽度
    int getWidth() const { return m_width; }

    /// 获取纹理高度
    int getHeight() const { return m_height; }

#ifdef OPENGL_EGL
    /// 从 EGLImage 更新纹理（QNX 零拷贝路径）
    /// @param eglImage  EGLImageKHR 句柄
    void updateFromEGLImage(void* eglImage, std::function<void()> gpuUseCompleteCallback = {});
#endif

protected:
    SafeStreamTexture(int width, int height);

    /// 初始化内置图像着色器
    void initShader();

    /// 预分配 GPU 纹理
    void initTexture();

private:
    int m_width;
    int m_height;
    TextureSharedPtr m_streamTexture;
};

} // namespace morrow

#endif // MORROW_SAFE_STREAM_TEXTURE_H
