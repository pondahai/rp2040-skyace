#!/bin/sh
# 用主機的 gcc 編 PC 預覽。只編 lib/ 與遊戲的純 C 部分，不碰 hal/。
#   sh pc/build.sh && build_pc/preview out.png RRDA
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
GAME=$HERE/..

# 找 SDK：$RETRO_SDK_PATH > ./sdk（submodule）> ../..（SDK 的 examples/ 底下）
if [ -n "$RETRO_SDK_PATH" ]; then SDK=$RETRO_SDK_PATH
elif [ -f "$GAME/sdk/lib/gfx.c" ]; then SDK=$GAME/sdk
elif [ -f "$GAME/../../lib/gfx.c" ]; then SDK=$GAME/../..
else echo "找不到 rp2040-retro-sdk，請設 RETRO_SDK_PATH" >&2; exit 1
fi

OUT=${OUT:-$GAME/build_pc}
mkdir -p "$OUT"
${CC:-gcc} -std=c11 -O2 -Wall -Wextra \
    -I"$SDK/lib" -I"$SDK/tools/pc" -I"$GAME" \
    "$HERE/preview.c" "$GAME/scene.c" \
    "$SDK/lib/gfx.c" "$SDK/lib/buttons.c" "$SDK/tools/pc/png.c" \
    -lm -o "$OUT/preview"
echo "$OUT/preview"
