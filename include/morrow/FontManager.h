#ifndef FONT_MANAGER_H_
#define FONT_MANAGER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "morrow/TextTextureInfo.h"

namespace morrow
{
class DynamicFont;

// 内部入口：实现层获取动态字体（DynamicFont 不进入公共 API）。
namespace fonts_internal {
std::shared_ptr<DynamicFont> getFont(const std::string& fontName);
}

struct FontInfo {
    std::string name;
    std::string path;
};

/// 字体管理器（对外）。
/// 内部实现（DynamicFont / stb_truetype）经 pimpl 隐藏，动态字体句柄不出公共接口。
class FontManager
{
public:
    FontManager();

    ~FontManager();

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    /// 引擎默认字体加载开关。应用通过 addFonts 提供自己的字体时可置 false，
    /// 跳过默认字体（约 8.4MB）的读盘与驻留。须在 initialize() 前调用。
    void setDefaultFontEnabled(bool enabled);

    /// 后续创建字体的字形图集初始边长（像素）；<=0 保持引擎默认（1024）。
    /// CJK 首屏文案多时建议 2048，避免首帧扩容触发全量字形重建。
    /// 须在 initialize()/addFonts() 前调用。
    void setInitialAtlasSize(int32_t size);

    /// 在工作线程预读默认字体文件字节，与 EGL 初始化等启动任务并行。
    /// initialize() 时汇合（通常已读完，无额外等待）。幂等；仅默认字体受益。
    void preloadDefaultFontAsync();

    void initialize();

    void addFonts(const std::vector<FontInfo>& fontsConfig);

    int32_t getTextWidth(const TextTextureInfoSharedPtr& textInfo);

    int32_t getTextHeight(const TextTextureInfoSharedPtr& textInfo);

    int32_t getTextWidthNoWrap(const TextTextureInfoSharedPtr& textInfo);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    friend std::shared_ptr<DynamicFont> fonts_internal::getFont(const std::string&);
};

using FontManagerSharedPtr = std::shared_ptr<FontManager>;

}
#endif /* FONT_MANAGER_H_ */
