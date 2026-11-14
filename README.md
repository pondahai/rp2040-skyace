# SKY ACE

![SKY ACE 畫面](docs/screenshot.png)

RP2040 復古掌機的直向捲軸空戰射擊，玩法致敬 1943：能量條會隨時間消耗、
被打中扣能量、吃 POW 補給，B 鍵翻滾閃避。用 [rp2040-retro-sdk](https://github.com/pondahai/rp2040-retro-sdk) 開發（`sdk/` submodule）。

| 按鍵 | 作用 |
| :--- | :--- |
| 方向鍵 | 移動 |
| A（按住） | 連射 |
| B | 翻滾（無敵，每關 3 次） |
| START | 暫停 |

- 一般敵機成波出現；紅色 5 機編隊全滅掉道具（POW 補能量 / 3WY 三向射擊）
- 轟炸機耐打、會齊射，擊落必掉道具
- 約 50 秒後魔王重轟炸機出場，擊落過關，剩餘能量換分數

## 建置

```bash
git clone --recursive https://github.com/pondahai/rp2040-skyace
source sdk/tools/env.sh
sh pc/build.sh && build_pc/preview out.png A96      # PC 預覽（按鍵序列見 pc/preview.c）
cmake -S . -B build_offset -G Ninja -DLOADER_OFFSET_BUILD=ON
ninja -C build_offset skyace_run                    # 編譯、推上掌機、啟動
```
