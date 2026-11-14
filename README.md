# SKY ACE

![SKY ACE 畫面](docs/screenshot.png)

RP2040 復古掌機的直向捲軸空戰射擊，玩法致敬 1943：能量條會隨時間消耗、
被打中扣能量、吃 POW 補給，B 鍵翻滾閃避。

| 按鍵 | 作用 |
| :--- | :--- |
| 方向鍵 | 移動 |
| A（按住） | 連射 |
| B | 翻滾（無敵，每關 3 次） |
| START | 暫停 |

- 一般敵機成波出現；紅色 5 機編隊全滅掉道具（POW 補能量 / 3WY 三向射擊）
- 轟炸機耐打、會齊射，擊落必掉道具
- 約 50 秒後魔王重轟炸機出場，擊落過關，剩餘能量換分數

## 怎麼玩

1. **接硬體**：照 [rp2040-retro-handheld](https://github.com/pondahai/rp2040-retro-handheld) README 的「🛠️ 硬體開源」章節，用 Raspberry Pi Pico、2.4"/2.8" ILI9341 SPI 螢幕、按鍵和喇叭模組接好電路（BOM 與接線都在那裡）。
2. **下載**：到 [Releases](https://github.com/pondahai/rp2040-skyace/releases/latest) 下載 `skyace_standalone.uf2`。
3. **燒錄**：按住 Pico 的 BOOTSEL 鍵接上 USB，電腦會出現 `RPI-RP2` 磁碟，把 `skyace_standalone.uf2` 拖進去，燒完會自動開始遊戲。

> 燒 standalone 版會蓋掉板子上原本的韌體。已經裝了 [rp2040-retro-loader](https://github.com/pondahai/rp2040-retro-loader) 開機選單的話，改用 release 裡的 `skyace.uf2` 和 `SKYACE.RAW`，放進 SD 卡就會出現在選單裡。

## 生態系

SKY ACE 是 [rp2040-retro-handheld](https://github.com/pondahai/rp2040-retro-handheld) 生態系裡的一款遊戲。那個生態系的想法是：**同一塊硬體（RP2040 + ILI9341 320×240 螢幕 + 8 顆按鍵），燒不同的 `.uf2` 就變成不同的機器**，例如 Doom、NES 模擬器、Apple II、電子字典、SDR 收音機、USB 鍵盤。

跟這款遊戲有關的幾個 repo：

| Repo | 角色 |
| :--- | :--- |
| [rp2040-retro-handheld](https://github.com/pondahai/rp2040-retro-handheld) | 總綱：硬體 BOM、接線、所有韌體一覽 |
| [rp2040-retro-loader](https://github.com/pondahai/rp2040-retro-loader) | 開機選單，從 SD 卡挑遊戲／韌體執行（release 的 `skyace.uf2` + `SKYACE.RAW` 就是給它用的） |
| [rp2040-retro-sdmount](https://github.com/pondahai/rp2040-retro-sdmount) | 不拔 SD 卡，從電腦把檔案推進掌機 |

SKY ACE 用作者自己的 rp2040-retro-sdk（未公開）開發，所以本 repo 的 `sdk/` 子模組無法 clone；想玩直接用 release 的 uf2 即可。
