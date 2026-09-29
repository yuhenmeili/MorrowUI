//
// FontInfo — 字体注册配置（公共）。
//
// 应用经 Engine::addFonts 注册字体：name 为业务侧引用的字体名
// （MRLabel::setText 的 fontName 参数），path 为字体文件路径。
// 字体加载与字形图集管理是引擎内部能力（FontManager 不进公共面，见
// docs/road_map/PUBLIC_API_ENCAPSULATION_PROPOSAL.md §5.1 文本一行）。
//

#ifndef MORROW_FONTINFO_H
#define MORROW_FONTINFO_H

#include <string>

namespace morrow {

struct FontInfo {
    std::string name;
    std::string path;
};

} // namespace morrow

#endif //MORROW_FONTINFO_H
