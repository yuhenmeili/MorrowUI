#ifndef FONT_MANAGER_H_
#define FONT_MANAGER_H_

#include <future>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "DynamicFont.h"
#include "TextTextureInfo.h"
#include "core/Language.h"

namespace morrow
{
struct FontInfo {
    std::string name;
    std::string path;
};

class FontManager
{
public:
    FontManager() = default;

    /// 引擎默认字体加载开关。应用通过 addFonts 提供自己的字体时可置 false，
    /// 跳过默认字体（约 8.4MB）的读盘与驻留。须在 initialize() 前调用。
    void setDefaultFontEnabled(bool enabled) { m_defaultFontEnabled = enabled; }

    /// 后续创建字体的字形图集初始边长（像素）；<=0 保持引擎默认（1024）。
    /// CJK 首屏文案多时建议 2048，避免首帧扩容触发全量字形重建。
    /// 须在 initialize()/addFonts() 前调用。
    void setInitialAtlasSize(int32_t size) { m_initialAtlasSize = size; }

    /// 在工作线程预读默认字体文件字节，与 EGL 初始化等启动任务并行。
    /// initialize() 时汇合（通常已读完，无额外等待）。幂等；仅默认字体受益。
    void preloadDefaultFontAsync();

    void initialize();

    void addFonts(const std::vector<FontInfo>& fontsConfig);

    DynamicFontSharedPtr getFont(const std::string& fontName);

    // std::shared_ptr<unsigned char> getTextBitmap(const TextTextureInfoSharedPtr& textInfo, int32_t textWidth, int32_t textHeight);

    int32_t getTextWidth(const TextTextureInfoSharedPtr& textInfo);

    int32_t getTextHeight(const TextTextureInfoSharedPtr& textInfo);

    int32_t getTextWidthNoWrap(const TextTextureInfoSharedPtr& textInfo);

private:
    DynamicFontSharedPtr createFont(const FontInfo& info);

    // 字体集：每个 DynamicFont 对应一个字体文件并共享一张多字号字形图集。
    std::unordered_map<std::string, DynamicFontSharedPtr> m_fontFamilies;
    // 按文件路径去重：同一路径（即使别名不同）复用同一份字体数据与图集。
    std::unordered_map<std::string, DynamicFontSharedPtr> m_fontsByPath;

    int32_t m_initialAtlasSize = 0;
    bool m_defaultFontEnabled = true;
    bool m_preloadStarted = false;
    std::future<std::shared_ptr<std::vector<unsigned char>>> m_preloadFuture;

    Language m_language = Language::CHINESE;
};

using FontManagerSharedPtr = std::shared_ptr<FontManager>;

}
#endif /* FONT_MANAGER_H_ */
