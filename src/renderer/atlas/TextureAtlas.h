//
// Created by lance on 24-7-1.
//

#ifndef MORROW_RENDERER_TEXTUREATLAS_H_
#define MORROW_RENDERER_TEXTUREATLAS_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "Texture.h"
#include "TextureRegion.h"

namespace morrow {
/// 纹理图集：解析 .atlas 文件，装配页纹理与纹理区域，供 UI 组件按名查询。
///
/// 装配流程：AtlasParser 解析出页/帧纯数据 → 每页建一个 Texture、
/// 每帧建一个 TextureRegion（UV 换算与翻转在此完成）。
class TextureAtlas {
public:
    /// 解析 packFileUrl 指向的 .atlas 文件并装配纹理。
    /// imagesDir 为帧图像所在目录；也可以直接指向单个纹理文件，此时所有页共用该纹理。
    TextureAtlas(const std::string& packFileUrl, const std::string& imagesDir, bool flip = false);

    /// 从内存装配：.atlas 文件字节 + 按页名索引的页纹理编码字节一次传入，
    /// 无需再调用 updateTexture 注入页纹理。pageImageData 的键为页名称
    /// （如 "atlas_speed.basis"），格式 png/jpg/basis/ktx2 自动识别；
    /// 未提供编码字节的页保持无像素来源并记录错误日志。
    TextureAtlas(std::shared_ptr<std::vector<unsigned char>> atlasFileData,
                 std::unordered_map<std::string, std::shared_ptr<std::vector<unsigned char>>> pageImageData, bool flip = false);

    /// 从内存装配且所有页共用一份编码纹理字节，对应 imagesDir 直接指向
    /// 单个纹理文件的 URL 用法。
    TextureAtlas(std::shared_ptr<std::vector<unsigned char>> atlasFileData, std::shared_ptr<std::vector<unsigned char>> pageImageData,
                 bool flip = false);

    ~TextureAtlas();

    /// 用新的图片地址更新与 name 匹配的页纹理。
    void updateTexture(const std::string& imageUrlOrName);

    /// 用内存中的编码图像字节（png/jpg/basis/ktx2 自动识别）更新与 pageName 匹配的页纹理。
    void updateTexture(std::shared_ptr<std::vector<unsigned char>> imageData, const std::string& pageName);

    /// 按页名称查询纹理，未找到返回空指针。
    TextureSharedPtr getTexture(const std::string& name) const;

    std::vector<TextureRegionSharedPtr>& getRegions();

    /// 按名称查找第一个匹配的区域，未找到返回空指针。
    TextureRegionSharedPtr findRegion(const std::string& name) const;

    /// 按名称与帧序号查找区域，未找到返回空指针。
    TextureRegionSharedPtr findRegion(const std::string& name, int32_t index) const;

    /// 查找名称匹配的全部区域。
    void findRegions(const std::string& name, std::vector<TextureRegionSharedPtr>& result) const;

private:
    /// 解析内存中的 .atlas 文件字节；为空则记录错误并保持无页无帧。
    void parseAtlasBuffer(const std::shared_ptr<std::vector<unsigned char>>& atlasFileData, bool flip);

    /// 为每个解析出的页创建纹理。
    void buildTextures(const std::string& imagesDir);

    /// 为每个解析出的页创建纹理（内存版本）：sharedPageImage 非空时所有页
    /// 共用，否则按 pageImageData 以页名查找。
    void buildTexturesFromMemory(const std::shared_ptr<std::vector<unsigned char>>& sharedPageImage,
                                 const std::unordered_map<std::string, std::shared_ptr<std::vector<unsigned char>>>& pageImageData);

    /// 页纹理的公共装配：采样参数、尺寸与页纹理表登记。
    void assemblePageTexture(const AtlasPageSharedPtr& page, const TextureSharedPtr& texture);

    /// 按图片名（容忍带路径前缀）查页纹理，未找到返回空指针。
    TextureSharedPtr findPageTexture(const std::string& imageUrlOrName) const;

    /// 将每个解析出的帧转换为纹理区域。
    void buildRegions();

private:
    AtlasParser m_parser;
    std::unordered_map<std::string, TextureSharedPtr> m_textures;
    std::vector<TextureRegionSharedPtr> m_regions;
};

using TextureAtlasSharedPtr = std::shared_ptr<TextureAtlas>;
} // MORROWGUI

#endif //MORROW_RENDERER_TEXTUREATLAS_H_