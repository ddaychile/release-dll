#!/usr/bin/env python3
"""Iconos del killfeed al doble de resolución, para el reemplazo en alta de Q2PRO.

Q2PRO dibuja `picn kf_x` con `pics/kf_x.png` si existe, pero si además existe `pics/kf_x.pcx` usa el tamaño del
PCX (images.c, get_image_dimensions): el PNG puede tener el doble de píxeles y en pantalla ocupa lo mismo. Con el
HUD escalado (scr_scale 2 o 3, lo normal en pantalla completa) se ve el detalle; los clientes sin PNG usan el PCX.

Por cada icono `pics/kf_<x>.png` (no las placas `kf_bg_*`):
  - `pics/kf_<x>.pcx`: el icono chico de antes (8 bits con la paleta de Quake 2, índice 255 transparente);
  - `pics/kf_<x>.png`: el doble de ancho y alto (28 px de alto).

Armas (`kf_w_<icono>`): del icono original del juego (`pics/w_<icono>.pcx`, pak0.pak o suelto), recortado y
llevado al doble de la caja del icono chico, en el mismo tono hueso (R = 0.627 * luminancia + 138.1, G = 0.975 R,
B = 0.874 R: ajustado sobre los iconos de Snako, error medio < 1). Banderas (`kf_<facción>`): de
`pics/teams/<facción>.pcx`, con sus colores. Las dibujadas a mano (calavera, granada, avión, explosión, tanque,
headshot) no tienen original: Scale2x del icono chico.

Se puede correr más de una vez: el icono chico se lee del .pcx si ya existe.

Uso: python3 tools/killfeed/kf_icons_hd.py <carpeta dday del juego, con pak*.pak> [carpeta pics]
"""
import glob
import io
import os
import struct
import sys

from PIL import Image

FACTIONS = ("usa", "usm", "gbr", "grm", "ita", "jpn", "pol", "rus")


def read_originals(gamedir):
    """pics/w_*.pcx y pics/teams/*.pcx de los paks (y sueltos, que mandan) -> {ruta: bytes}"""
    out = {}
    for pak in sorted(glob.glob(os.path.join(gamedir, "*.pak"))):
        with open(pak, "rb") as f:
            head = f.read(12)
            if head[:4] != b"PACK":
                continue
            off, size = struct.unpack("<ii", head[4:])
            f.seek(off)
            entries = [f.read(64) for _ in range(size // 64)]
            for e in entries:
                name = e[:56].split(b"\0")[0].decode("latin1").lower()
                pos, length = struct.unpack("<ii", e[56:])
                if name.endswith(".pcx") and (name.startswith("pics/w_") or name.startswith("pics/teams/")):
                    f.seek(pos)
                    out[name] = f.read(length)
    for p in glob.glob(os.path.join(gamedir, "pics", "w_*.pcx")) + glob.glob(os.path.join(gamedir, "pics", "teams", "*.pcx")):
        with open(p, "rb") as f:
            out[os.path.relpath(p, gamedir).replace(os.sep, "/").lower()] = f.read()
    return out


def pcx_rgba(data):
    im = Image.open(io.BytesIO(data))
    idx = im.load()
    rgba = im.convert("RGBA")
    px = rgba.load()
    for y in range(im.size[1]):
        for x in range(im.size[0]):
            if idx[x, y] == 255:
                px[x, y] = (0, 0, 0, 0)
    return rgba


def bone(im):
    """los colores del icono pasados al tono hueso de los iconos de Snako (el alfa queda)"""
    px = im.load()
    for y in range(im.size[1]):
        for x in range(im.size[0]):
            r, g, b, a = px[x, y]
            if not a:
                continue
            red = 0.627 * (0.299 * r + 0.587 * g + 0.114 * b) + 138.1
            px[x, y] = (round(red), round(red * 0.975), round(red * 0.874), a)
    return im


def scale2x(im):
    """Scale2x (EPX) sobre RGBA: el doble, con las diagonales suavizadas como pixel art"""
    w, h = im.size
    src = im.load()
    out = Image.new("RGBA", (w * 2, h * 2))
    dst = out.load()

    def p(x, y):
        return src[min(max(x, 0), w - 1), min(max(y, 0), h - 1)]

    for y in range(h):
        for x in range(w):
            c, a, b, d, e = p(x, y), p(x, y - 1), p(x + 1, y), p(x - 1, y), p(x, y + 1)
            e0 = e1 = e2 = e3 = c
            if a != e and d != b:
                e0 = d if d == a else c
                e1 = b if a == b else c
                e2 = d if d == e else c
                e3 = b if e == b else c
            dst[2 * x, 2 * y], dst[2 * x + 1, 2 * y] = e0, e1
            dst[2 * x, 2 * y + 1], dst[2 * x + 1, 2 * y + 1] = e2, e3
    return out


def to_pcx(im, palette, path):
    """RGBA -> PCX de 8 bits con la paleta de Quake 2 (alfa < 128: índice 255, transparente)"""
    pal_img = Image.new("P", (1, 1))
    pal_img.putpalette(palette[:255 * 3] + palette[:3])  # el 255 no se usa para colores
    rgb = Image.new("RGB", im.size, (0, 0, 0))
    rgb.paste(im, mask=im.split()[3])
    q = rgb.quantize(palette=pal_img, dither=Image.Dither.NONE)
    qp = q.load()
    ap = im.load()
    for y in range(im.size[1]):
        for x in range(im.size[0]):
            if ap[x, y][3] < 128:
                qp[x, y] = 255
    q.putpalette(palette)
    q.save(path)


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    gamedir = sys.argv[1]
    pics = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), "..", "..", "pics")
    originals = read_originals(gamedir)
    conchars = os.path.join(gamedir, "..", "baseq2", "pics", "conchars.pcx")
    palette = Image.open(conchars).getpalette()[:768]  # la paleta de Quake 2

    done = []
    for png in sorted(glob.glob(os.path.join(pics, "kf_*.png"))):
        name = os.path.basename(png)[3:-4]
        if name.startswith("bg_"):
            continue
        pcx = png[:-4] + ".pcx"
        small = (Image.open(pcx) if os.path.exists(pcx) else Image.open(png))
        if small.mode == "P":
            small = pcx_rgba(open(pcx, "rb").read())
        else:
            small = small.convert("RGBA")
        sw, sh = small.size
        box = small.getbbox()

        if name.startswith("w_"):
            src = originals.get(f"pics/{name}.pcx")
        elif name in FACTIONS:
            src = originals.get(f"pics/teams/{name}.pcx")
        else:
            src = None

        if src and box:
            orig = pcx_rgba(src)
            orig = orig.crop(orig.getbbox())
            big_box = (box[0] * 2, box[1] * 2, box[2] * 2, box[3] * 2)
            orig = orig.resize((big_box[2] - big_box[0], big_box[3] - big_box[1]), Image.Resampling.BOX)
            if name.startswith("w_"):
                orig = bone(orig)
            big = Image.new("RGBA", (sw * 2, sh * 2), (0, 0, 0, 0))
            big.paste(orig, big_box[:2])
            how = "original"
        else:
            big = scale2x(small)
            how = "scale2x"

        if not os.path.exists(pcx):
            to_pcx(small, palette, pcx)
        big.save(png, optimize=True)
        done.append((name, how))

    print(f"{len(done)} iconos: {sum(1 for d in done if d[1] == 'original')} desde el original, "
          f"{sum(1 for d in done if d[1] == 'scale2x')} con scale2x ({', '.join(d[0] for d in done if d[1] == 'scale2x')})")


if __name__ == "__main__":
    main()
