#!/usr/bin/env python3
"""Genera las placas del killfeed: pics/kf_bg_<juego><equipo>_<ancho>.png.

Cada placa mide <ancho> x 14 px (ancho de 48 a 344 en pasos de 8). Equipo `a`
(equipo 0), `x` (equipo 1) o `n` (sin equipo y Free For All). Un juego de placas
por estilo de killfeed (comando `killfeed` del jugador, ver src/p_killfeed.c):

- ``  (kf_bg_a_*)  estilo 2, como VisorDemos pero de color: fondo oscuro
                   semitransparente apenas teñido y franja de 3 px del equipo.
- `s` (kf_bg_sa_*) estilo 3, sólidas: verde / rojo vivos, franja clara de 3 px.
- `t` (kf_bg_ta_*) estilo 4: fondo oscuro teñido, más claro junto a una franja
                   de 7 px, con borde.
- `v` (kf_bg_v_*)  estilo 1, como VisorDemos: rectángulo negro con alfa 120
                   (una sola, sin equipo).
- `o` (kf_bg_oa_*) estilo 5, las originales de Snako: no se generan aquí, se
                   copian del commit 1314b1c (git show 1314b1c:pics/kf_bg_a_<ancho>.png).

Uso: python3 tools/killfeed/kf_plates.py [carpeta pics]
"""
import os
import sys

from PIL import Image

H = 14
WIDTHS = range(48, 344 + 1, 8)

# estilo 2 (y la base del 1 en color): colores de VisorDemos
VISOR_COLORS = {"a": (120, 175, 255), "x": (255, 115, 95), "n": (200, 190, 150)}
VISOR_TINT = 0.22
VISOR_BG_ALPHA = 160

# estilo 3: (franja, fondo, borde)
SOLID = {
    "a": ((170, 230, 80, 250), (70, 120, 32, 235), (120, 180, 60, 240)),
    "x": ((250, 90, 70, 250), (160, 40, 32, 235), (220, 80, 60, 240)),
    "n": ((220, 200, 130, 250), (110, 100, 66, 230), (170, 155, 100, 240)),
}

# estilo 4: (franja, borde, fondo junto a la franja, fondo a la derecha)
TINTED = {
    "a": ((150, 205, 70, 245), (110, 150, 50, 230), (62, 92, 30, 215), (34, 48, 20, 200)),
    "x": ((230, 72, 56, 245), (172, 56, 42, 230), (104, 34, 26, 215), (52, 22, 18, 200)),
    "n": ((196, 174, 108, 240), (140, 126, 82, 225), (78, 72, 50, 210), (42, 40, 32, 195)),
}


def lerp(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(4))


def corner(x, y, w):
    return x in (0, w - 1) and y in (0, H - 1)


def plate_visor_color(team, w):
    c = VISOR_COLORS[team]
    body = tuple(round(v * VISOR_TINT) for v in c) + (VISOR_BG_ALPHA,)
    im = Image.new("RGBA", (w, H), (0, 0, 0, 0))
    px = im.load()
    for x in range(w):
        for y in range(H):
            if not corner(x, y, w):
                px[x, y] = c + (235,) if x < 3 else body
    return im


def plate_solid(team, w):
    stripe, body, border = SOLID[team]
    im = Image.new("RGBA", (w, H), (0, 0, 0, 0))
    px = im.load()
    for x in range(w):
        for y in range(H):
            if corner(x, y, w):
                continue
            if x < 3:
                px[x, y] = stripe
            elif y in (0, H - 1) or x == w - 1:
                px[x, y] = border
            else:
                px[x, y] = body
    return im


def plate_tinted(team, w):
    stripe, border, left, right = TINTED[team]
    im = Image.new("RGBA", (w, H), (0, 0, 0, 0))
    px = im.load()
    for x in range(w):
        body = lerp(left, right, min(1.0, (x - 7) / max(1, (w - 7) * 0.6))) if x >= 7 else stripe
        for y in range(H):
            if corner(x, y, w):
                continue
            if y in (0, H - 1) or x == w - 1:
                px[x, y] = stripe if x < 7 else border
            else:
                px[x, y] = body
    return im


def plate_visor(w):
    return Image.new("RGBA", (w, H), (0, 0, 0, 120))


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "..", "pics")
    n = 0
    for w in WIDTHS:
        for team in "axn":
            for prefix, make in (("", plate_visor_color), ("s", plate_solid), ("t", plate_tinted)):
                make(team, w).save(os.path.join(out, f"kf_bg_{prefix}{team}_{w}.png"), optimize=True)
                n += 1
        plate_visor(w).save(os.path.join(out, f"kf_bg_v_{w}.png"), optimize=True)
        n += 1
    print(f"{n} placas en {os.path.abspath(out)}")


if __name__ == "__main__":
    main()
