from pathlib import Path
from PIL import Image

p = Path("components/ui_app/assets/images/bg.png")
out_c = p.with_name("bg_png.c")
out_h = p.with_name("bg_png.h")

img = Image.open(p).convert("RGB")
w, h = img.size

raw = bytearray()
for r, g, b in img.getdata():
    v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    raw.append(v & 0xFF)
    raw.append((v >> 8) & 0xFF)

lines = [
    '#include "bg_png.h"',
    "",
    "const uint8_t ui_bg_png_map[] = {",
]

for i in range(0, len(raw), 16):
    chunk = raw[i:i + 16]
    s = ", ".join(f"0x{x:02X}" for x in chunk)
    lines.append(f"    {s}," if i + 16 < len(raw) else f"    {s}")

lines.extend(
    [
        "};",
        "",
        "#if LVGL_VERSION_MAJOR >= 9",
        "const lv_image_dsc_t ui_bg_png = {",
        "    .header.magic = LV_IMAGE_HEADER_MAGIC,",
        "    .header.cf = LV_COLOR_FORMAT_RGB565,",
        "    .header.flags = 0,",
        f"    .header.w = {w},",
        f"    .header.h = {h},",
        f"    .header.stride = {w * 2},",
        "    .header.reserved_2 = 0,",
        f"    .data_size = {len(raw)},",
        "    .data = ui_bg_png_map,",
        "    .reserved = NULL,",
        "    .reserved_2 = NULL,",
        "};",
        "#else",
        "const lv_img_dsc_t ui_bg_png = {",
        "    .header.cf = LV_IMG_CF_TRUE_COLOR,",
        "    .header.always_zero = 0,",
        "    .header.reserved = 0,",
        f"    .header.w = {w},",
        f"    .header.h = {h},",
        f"    .data_size = {len(raw)},",
        "    .data = ui_bg_png_map,",
        "};",
        "#endif",
        "",
    ]
)

out_c.write_text("\n".join(lines), encoding="utf-8")
out_h.write_text(
    """#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern \"C\" {
#endif

#if LVGL_VERSION_MAJOR >= 9
extern const lv_image_dsc_t ui_bg_png;
#else
extern const lv_img_dsc_t ui_bg_png;
#endif

#ifdef __cplusplus
}
#endif
""",
    encoding="utf-8",
)

print(f"OK RGB565 {w}x{h} bytes={len(raw)}")
