#!/usr/bin/env python3
"""
generate_atlas_c_array.py — 将 PNG 图集转换为 C++ 硬编码数组头文件

用途：
  将 HMI 仪表盘 Atlas 纹理（PNG 格式）编译期嵌入 SafeStaticSprite 组件，
  生成 static const unsigned char[] 数组，由编译器放入 .rodata 只读内存段。

用法：
  python tools/generate_atlas_c_array.py assets/textures/hmi_atlas.png --rgba -o src/ui/hmi/SafeStaticSpriteAtlas.h

参数：
  input               输入的 PNG 图集文件路径
  --rgba               输出 RGBA 格式（4 通道，默认）
  --rgb                输出 RGB 格式（3 通道）
  -o, --output PATH    输出头文件路径（默认 stdout）
  --var-name NAME      数组变量名（默认 kAtlasPixelData）
  --width W            若图片尺寸与图集逻辑尺寸不同，可手动指定宽度
  --height H           手动指定高度

依赖：
  pip install pillow    （如未安装：pip install pillow）

示例（嵌入到 SafeStaticSpriteAtlas.h）：
  python tools/generate_atlas_c_array.py hmi_atlas.png --rgba -o src/ui/hmi/SafeStaticSpriteAtlas.h
"""

import argparse
import os
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("错误：需要 Pillow 库。请运行: pip install pillow")
    sys.exit(1)


def generate_c_array(
    image_path: str,
    channels: int,
    var_name: str,
    output_path: str | None,
    force_width: int = 0,
    force_height: int = 0,
) -> tuple[int, int, int]:
    """将 PNG 转换为 C unsigned char 数组并写入文件。

    Returns:
        (width, height, total_bytes)
    """
    img = Image.open(image_path)

    # 转换为 RGBA 或 RGB
    if channels == 4:
        if img.mode != "RGBA":
            img = img.convert("RGBA")
    else:
        if img.mode != "RGB":
            img = img.convert("RGB")

    width, height = img.size
    if force_width > 0:
        width = force_width
    if force_height > 0:
        height = force_height

    pixels = list(img.getdata())
    total_bytes = width * height * channels

    lines: list[str] = []
    lines.append("// SafeStaticSpriteAtlas.h -- Compile-time hardcoded Atlas pixel data (ROM .rodata)")
    lines.append("// Auto-generated -- DO NOT EDIT MANUALLY")
    lines.append(f"// Source: {os.path.basename(image_path)}")
    lines.append(f"// Spec: {width}x{height} {'RGBA' if channels == 4 else 'RGB'}8, {total_bytes} bytes")
    lines.append(f"// Command: python tools/generate_atlas_c_array.py {image_path} " +
                 f"{'--rgba' if channels == 4 else '--rgb'} -o {output_path or '<stdout>'}")
    lines.append("")
    lines.append(f"static const unsigned char {var_name}[] = {{")

    # 每行最多 16 个值，便于阅读
    values_per_line = 16
    flat: list[int] = []
    for pixel in pixels:
        if channels == 4:
            flat.extend(pixel[:4])  # R, G, B, A
        else:
            flat.extend(pixel[:3])  # R, G, B

    for i in range(0, len(flat), values_per_line):
        chunk = flat[i : i + values_per_line]
        line = "    " + ",".join(str(v) for v in chunk) + ","
        lines.append(line)

    # 确保数组非空（移除最后的逗号由 C++ 标准允许，保留以简化生成）
    lines.append("};")

    content = "\n".join(lines) + "\n"

    if output_path:
        os.makedirs(os.path.dirname(output_path) or ".", exist_ok=True)
        with open(output_path, "w", encoding="utf-8") as f:
            f.write(content)
        print(f"✓ 已生成: {output_path}  ({width}×{height}, {total_bytes} bytes)")
    else:
        print(content)

    return width, height, total_bytes


def main():
    parser = argparse.ArgumentParser(
        description="将 PNG 图集转换为 C++ 硬编码数组（用于 SafeStaticSprite 安全组件）"
    )
    parser.add_argument("input", help="输入 PNG 图集文件路径")
    parser.add_argument(
        "--rgba", action="store_true", default=True,
        help="输出 RGBA 4 通道（默认）"
    )
    parser.add_argument(
        "--rgb", action="store_true",
        help="输出 RGB 3 通道"
    )
    parser.add_argument(
        "-o", "--output", default=None,
        help="输出头文件路径（默认输出到 stdout）"
    )
    parser.add_argument(
        "--var-name", default="kAtlasPixelData",
        help="C 数组变量名（默认 kAtlasPixelData）"
    )
    parser.add_argument("--width", type=int, default=0, help="手动指定图集宽度")
    parser.add_argument("--height", type=int, default=0, help="手动指定图集高度")

    args = parser.parse_args()

    if not os.path.isfile(args.input):
        print(f"错误：找不到输入文件 '{args.input}'")
        sys.exit(1)

    channels = 3 if args.rgb else 4

    generate_c_array(
        image_path=args.input,
        channels=channels,
        var_name=args.var_name,
        output_path=args.output,
        force_width=args.width,
        force_height=args.height,
    )


if __name__ == "__main__":
    main()
