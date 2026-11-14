"""SKY ACE 封面：海面上的雙尾撐戰機（96x96）。

    python tools/make_cover.py   （需要 Pillow）
"""
import os
import struct
from PIL import Image, ImageDraw

W = H = 96
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets")

BG      = (246, 244, 238)
OUTLINE = (58, 48, 44)
SHELL_L = (222, 214, 198)
SHELL   = (196, 186, 166)   # #C4BAA6
SHELL_D = (150, 140, 122)
LABEL_L = (232, 120, 72)
LABEL   = (204, 84, 48)
LABEL_D = (150, 56, 32)
GOLD_L  = (240, 214, 120)
GOLD    = (206, 170, 72)
GOLD_D  = (150, 118, 44)

img = Image.new("RGB", (W, H), (36, 100, 180))
d = ImageDraw.Draw(img)
# 海浪
for y in range(6, 96, 14):
    for x in range((y * 7) % 20, 96, 26):
        d.line([x, y, x + 8, y], fill=(70, 130, 205))
# 主角雙尾撐戰機（像素放大 5 倍，機頭朝上）
ART = [
    ".......ww.......",
    ".......gg.......",
    "......gbbg......",
    "......gbbg......",
    ".gg...gggg...gg.",
    "gggggggggggggggg",
    "gwwggggggggggwwg",
    ".gg...gggg...gg.",
    ".gg....gg....gg.",
    ".gg....gg....gg.",
    ".gg....gg....gg.",
    ".gg....gg....gg.",
    ".gg...gggg...gg.",
    ".gggggggggggggg.",
    ".yy..........yy.",
]
COL = {"g": (176, 182, 192), "w": (250, 250, 250), "b": (120, 200, 255), "y": (255, 210, 40)}
S, OX, OY = 5, 8, 6
for yy, row in enumerate(ART):
    for xx, ch in enumerate(row):
        if ch in COL:
            d.rectangle([OX + xx * S + 4, OY + yy * S + 6, OX + xx * S + S + 3, OY + yy * S + S + 5], fill=(20, 60, 120))
for yy, row in enumerate(ART):
    for xx, ch in enumerate(row):
        if ch in COL:
            d.rectangle([OX + xx * S, OY + yy * S, OX + xx * S + S - 1, OY + yy * S + S - 1], fill=COL[ch])
# 子彈
for x in (34, 58):
    d.rectangle([x, 0, x + 2, 4], fill=(255, 255, 160))

os.makedirs(OUT, exist_ok=True)
img.save(os.path.join(OUT, "cover.png"))
px = img.tobytes()
with open(os.path.join(OUT, "SKYACE.RAW"), "wb") as f:
    for i in range(0, len(px), 3):
        r, g, b = px[i], px[i + 1], px[i + 2]
        f.write(struct.pack(">H", ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)))
print(os.path.join(OUT, "SKYACE.RAW"))
