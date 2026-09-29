#include <cstdio>
#include <fstream>
#include <future>
#include <unordered_map>

#include "DynamicFont.h"
#include "morrow/FontManager.h"
#include "GlobalObject.h"
#include "morrow/utils/Log.h"

namespace morrow {
namespace {
// 引擎默认字体：仅中文字体（含 CJK 全字符集，约 8.4MB）。
constexpr const char* kDefaultFontName = "MorrowSansCN1.1-Regular.otf";
constexpr const char* kDefaultFontPath = "assets/fonts/MorrowSansCN1.1-Regular.otf";

std::shared_ptr<std::vector<unsigned char>> readFontFileBytes(const std::string& path) {
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) {
        LOG_E("fail to preload font {}", path);
        return nullptr;
    }
    fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size <= 0) {
        fclose(file);
        return nullptr;
    }
    auto bytes = std::make_shared<std::vector<unsigned char>>(static_cast<size_t>(size));
    const size_t read = fread(bytes->data(), 1, bytes->size(), file);
    fclose(file);
    if (read != bytes->size()) {
        return nullptr;
    }
    return bytes;
}
} // namespace

struct FontManager::Impl {
    // 字体集：每个 DynamicFont 对应一个字体文件并共享一张多字号字形图集。
    std::unordered_map<std::string, DynamicFontSharedPtr> m_fontFamilies;
    // 按文件路径去重：同一路径（即使别名不同）复用同一份字体数据与图集。
    std::unordered_map<std::string, DynamicFontSharedPtr> m_fontsByPath;

    int32_t m_initialAtlasSize = 0;
    bool m_defaultFontEnabled = true;
    bool m_preloadStarted = false;
    std::future<std::shared_ptr<std::vector<unsigned char>>> m_preloadFuture;

    DynamicFontSharedPtr createFont(const FontInfo& info) {
        auto font = std::make_shared<DynamicFont>(m_initialAtlasSize);
        bool loaded = false;

        // 预读字节就绪时直接接管（零拷贝），避免二次读盘。
        if (m_preloadFuture.valid() && info.path == kDefaultFontPath) {
            auto bytes = m_preloadFuture.get();
            if (bytes && !bytes->empty()) {
                loaded = font->AdoptFontData(std::move(*bytes));
            }
        }
        if (!loaded) {
            loaded = font->LoadFromFile(info.path);
        }
        if (!loaded) {
            return nullptr;
        }
        font->SetAntialiasingQuality(2);
        font->SetCharacterSpacing(0.5f);
        return font;
    }

    void addFonts(const std::vector<FontInfo>& fontsConfig) {
        for (const auto& info : fontsConfig) {
            if (!info.path.empty()) {
                if (m_fontFamilies.find(info.name) != m_fontFamilies.end()) {
                    LOG_W("font {} already exists, skip loading", info.name);
                    continue;
                }
                auto existing = m_fontsByPath.find(info.path);
                if (existing != m_fontsByPath.end()) {
                    // 同一路径已加载：按别名复用，避免重复读盘与双图集驻留。
                    m_fontFamilies[info.name] = existing->second;
                    LOG_I("font {} reuses already loaded font data of {}", info.name, info.path);
                    continue;
                }
                DynamicFontSharedPtr dynamicFont = createFont(info);
                if (!dynamicFont) {
                    LOG_E("load font {} error", info.path);
                    continue;
                }
                m_fontFamilies[info.name] = std::move(dynamicFont);
                m_fontsByPath[info.path] = m_fontFamilies[info.name];
            } else {
                LOG_I("fontUrl is empty");
            }
        }
    }

    DynamicFontSharedPtr getFont(const std::string& fontName) {
        if (!fontName.empty()) {
            auto it = m_fontFamilies.find(fontName);
            if (it != m_fontFamilies.end()) {
                return it->second;
            }
        }

        if (m_fontFamilies.empty()) {
            return nullptr;
        }

        return m_fontFamilies.begin()->second;
    }
};

FontManager::FontManager() : m_impl(std::make_unique<Impl>()) {}

FontManager::~FontManager() = default;

void FontManager::setDefaultFontEnabled(bool enabled) {
    m_impl->m_defaultFontEnabled = enabled;
}

void FontManager::setInitialAtlasSize(int32_t size) {
    m_impl->m_initialAtlasSize = size;
}

void FontManager::preloadDefaultFontAsync() {
    if (!m_impl->m_defaultFontEnabled || m_impl->m_preloadStarted) {
        return;
    }
    m_impl->m_preloadStarted = true;
    m_impl->m_preloadFuture = std::async(std::launch::async, [] {
        return readFontFileBytes(kDefaultFontPath);
    });
}

void FontManager::initialize() {
    if (!m_impl->m_defaultFontEnabled) {
        return;
    }
    FontInfo fontInfo = {
        .name = kDefaultFontName,
        .path = kDefaultFontPath
    };
    addFonts({fontInfo});
}

void FontManager::addFonts(const std::vector<FontInfo>& fontsConfig) {
    m_impl->addFonts(fontsConfig);
}

int32_t FontManager::getTextWidth(const TextTextureInfoSharedPtr& textInfo) {
    return 0;
}

int32_t FontManager::getTextHeight(const TextTextureInfoSharedPtr& textInfo) {
    return 0;
}

int32_t FontManager::getTextWidthNoWrap(const TextTextureInfoSharedPtr& textInfo) {
    return 0;
}

std::shared_ptr<DynamicFont> fonts_internal::getFont(const std::string& fontName) {
    auto* manager = GlobalObject::getInstance().getFontManager().get();
    return manager ? manager->m_impl->getFont(fontName) : nullptr;
}
}
