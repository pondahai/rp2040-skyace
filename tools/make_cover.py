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
# 主角戰機：長主翼雙尾撐，佔滿封面寬度（像素放大 4 倍，機頭朝上）
ART = [
    "...........ww...........",
    "..........gggg..........",
    "..........gbbg..........",
    ".....dd...gbbg...dd.....",
    ".....gg...gggg...gg.....",
    "dddddddddddddddddddddddd",
    "gwwggggggggggggggggggwwg",
    ".gggggggggggggggggggggg.",
    ".....gg...gggg...gg.....",
    ".....gg....gg....gg.....",
    ".....gg..........gg.....",
    ".....gg..........gg.....",
    ".....gg..........gg.....",
    "....gggggggggggggggg....",
    ".....yy..........yy.....",
]
COL = {"g": (184, 190, 200), "d": (124, 130, 142), "w": (250, 250, 250),
       "b": (120, 200, 255), "y": (255, 170, 30)}
S, OX, OY = 3, 12, 46
SHADOW = (20, 62, 124)
for yy, row in enumerate(ART):          # 海面上的影子
    for xx, ch in enumerate(row):
        if ch in COL:
            d.rectangle([OX + xx * S + 5, OY + yy * S + 8, OX + xx * S + S + 4, OY + yy * S + S + 7], fill=SHADOW)
for yy, row in enumerate(ART):
    for xx, ch in enumerate(row):
        if ch in COL:
            d.rectangle([OX + xx * S, OY + yy * S, OX + xx * S + S - 1, OY + yy * S + S - 1], fill=COL[ch])
# 機槍連射：兩串子彈射向前方的敵機
for x in (28, 64):
    for y in (12, 22, 32):
        d.rectangle([x, y, x + 2, y + 5], fill=(255, 250, 150))
# 前方被擊中的敵機（機頭朝下）與爆炸火光
ENEMY = [
    "....gggggggg....",
    "......gggg......",
    ".gggggggggggggg.",
    "gggggggggggggggg",
    "......gbbg......",
    ".......gg.......",
]
for yy, row in enumerate(ENEMY):
    for xx, ch in enumerate(row):
        if ch != ".":
            d.rectangle([24 + xx * 3, 2 + yy * 3, 26 + xx * 3, 4 + yy * 3],
                        fill=(120, 200, 255) if ch == "b" else (200, 52, 40))
d.rectangle([58, 2, 72, 12], fill=(255, 140, 20))
d.rectangle([61, 0, 69, 15], fill=(255, 140, 20))
d.rectangle([62, 4, 68, 10], fill=(255, 240, 120))

os.makedirs(OUT, exist_ok=True)
img.save(os.path.join(OUT, "cover.png"))
px = img.tobytes()
with open(os.path.join(OUT, "SKYACE.RAW"), "wb") as f:
    for i in range(0, len(px), 3):
        r, g, b = px[i], px[i + 1], px[i + 2]
        f.write(struct.pack(">H", ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)))
print(os.path.join(OUT, "SKYACE.RAW"))
