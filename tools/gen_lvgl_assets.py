#!/usr/bin/env python3
from pathlib import Path
import re
import subprocess
import sys

# =========================================================
# CONFIG
# =========================================================
PROJECT_ROOT = Path(__file__).resolve().parent.parent

SRC_DIR = PROJECT_ROOT / "components" / "ui_app" / "assets" / "src"
OUT_DIR = PROJECT_ROOT / "components" / "ui_app" / "assets" / "images"

# Ajustá esta ruta según dónde tengas LVGL dentro del proyecto
LVGL_IMAGE_PY = PROJECT_ROOT / "lv_port_pc_vscode" / "lvgl" / "scripts" / "LVGLImage.py"

# Formato recomendado para iconos/UI con transparencia
COLOR_FORMAT = "ARGB8888"

# Si querés usar compresión, cambiar a True
USE_COMPRESSION = False


# =========================================================
# HELPERS
# =========================================================
def sanitize_symbol_name(name: str) -> str:
    """
    Convierte el nombre base del archivo en un símbolo C válido
    y SIEMPRE le agrega prefijo img_.
    """
    name = name.lower()
    name = re.sub(r'[^a-z0-9_]', '_', name)
    name = re.sub(r'_+', '_', name).strip('_')

    if not name:
        name = "asset"

    return f"img_{name}"

#para usar lvgl8 cambiar lv_image_dsc_t por lv_img_dsc_t
def write_header(h_path: Path, symbol_name: str):
    content = f"""#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {{
#endif

extern const lv_img_dsc_t {symbol_name};

#ifdef __cplusplus
}}
#endif
"""
    h_path.write_text(content, encoding="utf-8")


def patch_generated_c(c_path: Path, original_base_name: str, symbol_name: str):
    """
    Fuerza el nombre final del descriptor a symbol_name.
    Ej: bg -> img_bg
    Compatible con LVGL 8 y 9.
    """
    txt = c_path.read_text(encoding="utf-8")

    base_escaped = re.escape(original_base_name)

    # Renombrar descriptor principal:
    # const lv_image_dsc_t bg = {
    # const lv_img_dsc_t bg = {
    txt = re.sub(
        rf'(\bconst\s+lv_(?:image|img)_dsc_t\s+){base_escaped}(\s*=)',
        rf'\1{symbol_name}\2',
        txt
    )

    # Algunas versiones podrían usar no-const
    txt = re.sub(
        rf'(\blv_(?:image|img)_dsc_t\s+){base_escaped}(\s*=)',
        rf'\1{symbol_name}\2',
        txt
    )

    # Por si hubiera extern
    txt = re.sub(
        rf'(\bextern\s+const\s+lv_(?:image|img)_dsc_t\s+){base_escaped}(\s*;)',
        rf'\1{symbol_name}\2',
        txt
    )

    c_path.write_text(txt, encoding="utf-8")


def generate_one(png_path: Path):
    base_name = png_path.stem
    symbol_name = sanitize_symbol_name(base_name)

    out_c = OUT_DIR / f"{symbol_name}.c"
    out_h = OUT_DIR / f"{symbol_name}.h"

    print(f"[ASSET] {png_path.name} -> {out_c.name}, {out_h.name}")
    print(f"[ASSET] symbol: {symbol_name}")

    cmd = [
        sys.executable,
        str(LVGL_IMAGE_PY),
        str(png_path),
        "--ofmt", "C",
        "--cf", COLOR_FORMAT,
        "-o", str(OUT_DIR),
    ]

    if USE_COMPRESSION:
        cmd += ["--compress"]

    result = subprocess.run(cmd, capture_output=True, text=True)

    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr, file=sys.stderr)
        raise RuntimeError(f"Error convirtiendo {png_path.name}")

    generated_candidates = [
        OUT_DIR / f"{base_name}.c",
        OUT_DIR / base_name / f"{base_name}.c",
        OUT_DIR / png_path.name / f"{base_name}.c",
    ]

    generated_c = None
    for candidate in generated_candidates:
        if candidate.exists():
            generated_c = candidate
            break

    if generated_c is None:
        matches = list(OUT_DIR.rglob(f"{base_name}.c"))
        if matches:
            generated_c = matches[0]

    if generated_c is None:
        raise FileNotFoundError(
            f"No encontré el .c generado para {png_path.name} dentro de {OUT_DIR}"
        )

    # mover al nombre final con prefijo img_
    if generated_c.resolve() != out_c.resolve():
        out_c.write_bytes(generated_c.read_bytes())
        generated_c.unlink()

        parent_dir = generated_c.parent
        if parent_dir != OUT_DIR:
            try:
                parent_dir.rmdir()
            except OSError:
                pass

    patch_generated_c(out_c, base_name, symbol_name)
    write_header(out_h, symbol_name)

    return symbol_name


def write_assets_cmake(asset_symbol_names):
    cmake_path = OUT_DIR / "assets.cmake"

    lines = []
    lines.append("# Archivo autogenerado por gen_lvgl_assets.py")
    lines.append("set(UI_ASSET_SRCS")
    for symbol_name in asset_symbol_names:
        lines.append(f'    assets/images/{symbol_name}.c')
    lines.append(")")
    lines.append("")

    lines.append("set(UI_ASSET_HEADERS")
    for symbol_name in asset_symbol_names:
        lines.append(f'    assets/images/{symbol_name}.h')
    lines.append(")")
    lines.append("")

    cmake_path.write_text("\n".join(lines), encoding="utf-8")


def remove_stale_generated_files(valid_symbol_names):
    valid_c = {f"{name}.c" for name in valid_symbol_names}
    valid_h = {f"{name}.h" for name in valid_symbol_names}
    reserved = {"assets.cmake"}

    for f in OUT_DIR.glob("*"):
        if not f.exists():
            continue

        if f.is_file():
            if f.name in reserved:
                continue

            if f.suffix == ".c" and f.name not in valid_c:
                print(f"[ASSET] Eliminando viejo: {f.name}")
                f.unlink()

            elif f.suffix == ".h" and f.name not in valid_h:
                print(f"[ASSET] Eliminando viejo: {f.name}")
                f.unlink()

    for d in OUT_DIR.iterdir():
        if d.is_dir():
            print(f"[ASSET] Eliminando carpeta residual: {d.name}")
            for sub in d.rglob("*"):
                if sub.is_file():
                    sub.unlink()
            for subdir in sorted(d.rglob("*"), reverse=True):
                if subdir.is_dir():
                    try:
                        subdir.rmdir()
                    except OSError:
                        pass
            try:
                d.rmdir()
            except OSError:
                pass


# =========================================================
# MAIN
# =========================================================
def main():
    if not SRC_DIR.exists():
        raise FileNotFoundError(f"No existe carpeta origen: {SRC_DIR}")

    if not LVGL_IMAGE_PY.exists():
        raise FileNotFoundError(
            f"No encuentro el convertidor de LVGL en:\n{LVGL_IMAGE_PY}\n"
            "Corregí la variable LVGL_IMAGE_PY dentro del script."
        )

    OUT_DIR.mkdir(parents=True, exist_ok=True)

    pngs = sorted(SRC_DIR.glob("*.png"))
    if not pngs:
        print(f"[ASSET] No hay PNG en {SRC_DIR}")
        write_assets_cmake([])
        return

    generated_symbols = []
    for png in pngs:
        symbol_name = generate_one(png)
        generated_symbols.append(symbol_name)

    remove_stale_generated_files(generated_symbols)
    write_assets_cmake(generated_symbols)

    print("[ASSET] OK. Conversión finalizada.")


if __name__ == "__main__":
    main()