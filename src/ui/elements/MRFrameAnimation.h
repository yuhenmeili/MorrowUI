//
// Created by lance on 24-7-1.
//

#ifndef MORROW_MRFrameAnimation_H_
#define MORROW_MRFrameAnimation_H_

#include <memory>
#include "atlas/AtlasRegion.h"
#include "atlas/TextureAtlas.h"
#include "atlas/Animation.h"
#include "base/UIWidget.h"

namespace morrow
{
/**
 * @brief 按纹理图集帧序列播放动画的 UI 组件。
 *
 * 内部维护当前帧、动画时间与图集区域切换：通过 setTextureAtlas() 注入
 * 帧数据后，update() 会按帧序列自动推进当前帧。适合表情、提示动画、
 * loading、仪表指针序列等场景。
 *
 * ============================================================================
 * 纹理图集制作流程
 * ============================================================================
 *
 * 一、生成 .atlas 元数据文件
 *
 * 推荐使用 GDX Texture Packer（libGDX 生态的纹理打包工具）生成 .atlas
 * 文件与对应的图集纹理，任何能输出下述格式的工具均可。
 *
 * .atlas 是纯文本元数据文件，描述每帧在图集纹理中的位置。解析规则：
 *   - 每行以第一个冒号 ':' 分割为 key 与 value，value 内多个字段以逗号 ',' 分隔；
 *   - 不含冒号的行被识别为帧名称/标识符；
 *   - 空格与换行会被自动去除，对缩进无强制要求。
 *
 * 文件结构：
 *   第 1 行   图集纹理文件名，必须以 "atlas_" 开头，如 atlas_cube.basis
 *   第 2 行   纹理尺寸，格式 size:<宽>,<高>，如 size:2048,2048
 *   第 3 行   重复模式，固定 repeat:none
 *   之后      逐帧定义，每帧两行，支持以下两种模式
 *
 * 模式 A（索引模式）：适用于统一尺寸的逐帧动画，帧按 index 升序播放。
 *   256                        帧尺寸，仅作标记
 *   index:11                   播放序号
 *   bounds:2,1550,256,256      图集内像素区域 left,top,width,height
 *
 * 模式 B（命名模式）：适用于散图打包，按文件出现顺序排列。
 *   O_HundredDigit_1.png       原始图片文件名
 *   bounds:690,356,82,130      图集内像素区域 left,top,width,height
 *
 * 完整示例（assets/textures/frame_animation/atlas_cube.atlas）：
 *
 *   atlas_cube.basis
 *   size:2048,2048
 *   repeat:none
 *   256
 *   index:11
 *   bounds:2,1550,256,256
 *   ...
 *
 * 二、压缩图集纹理（可选）
 *
 * 图集纹理需与 .atlas 放在同一目录，支持 .png、.basis 等格式（取决于
 * 引擎的图像加载能力）。可使用 tools/basisuD.exe 将图片压缩为 basis
 * 纹理以减小体积、提升加载速度：
 *
 *   .\tools\basisuD.exe .\assets\textures\brickwall.jpg -output_file .\assets\textures\brickwall.basis
 *
 * basisu 工具来自 basis_universal 项目：
 *   https://github.com/BinomialLLC/basis_universal
 * Windows 平台可直接使用仓库内 tools/basisuD.exe；其他平台需自行拉取
 * 上游源码，编译出对应平台的 basisu 工具后使用。
 *
 * 生成后，将 .atlas 第一行的纹理文件后缀改为 .basis 即可。
 */
class MRFrameAnimation : public UIWidget
{
public:
    /// 创建一个帧动画组件。
    static std::shared_ptr<MRFrameAnimation> create();

    /// 销毁帧动画组件。
    ~MRFrameAnimation() override = default;

    /// 每帧推进动画时间并更新当前图集区域。
    void update(FrameStateSharedPtr frameState) override;

    /// 设置包含动画帧数据的纹理图集。
    void setTextureAtlas(TextureAtlasSharedPtr textureAtlas);

    /// 将动画重置到起始帧。
    void resetFrame();

private:
    MRFrameAnimation();

    TextureAtlasSharedPtr m_textureAtlas;
    AnimationSharedPtr<AtlasRegionSharedPtr> m_animation;
    AtlasRegionSharedPtr m_currentFrame;
    float m_animationTime = 0.0f;
};

using MRFrameAnimationSharedPtr = std::shared_ptr<MRFrameAnimation>;
}

#endif //MORROW_MRFrameAnimation_H_
