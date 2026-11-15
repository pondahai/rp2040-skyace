/* PC 預覽：用跟實機同一份 scene_render()，一條一條畫成 320×240，存 PNG。
 *
 *   sh pc/build.sh
 *   build_pc/preview out.png [按鍵序列]
 *
 * 每個字元模擬 10 幀：L R U D 按住方向（同時一直開火），A 按一下 A，
 * B 翻滾，S START，. 什麼都不按，數字 n = 跳過 n*100 幀（邊飛邊開火、左右閃）。
 * 測試用：#n 從第 n 關開始，F 跳到發現敵艦前，E 補滿能量，f 前進 1 幀。
 * 先把畫面填滿洋紅色：render 沒畫到的像素在實機會是上一條的殘影。
 */
#include <stdio.h>
#include "gfx.h"
#include "buttons.h"
#include "scene.h"
#include "png.h"

static uint16_t screen[RD_GFX_W * RD_GFX_H];
static scene sc;

static void run(int frames, uint8_t held, uint8_t fired)
{
    for (int f = 0; f < frames; f++) {
        scene_update(&sc, held, f == 0 ? fired : 0);
        if (sc.state == ST_OVER) return;
        if (sc.energy < 300) sc.energy = 1000;      /* 預覽不讓能量耗盡 */
    }
}

int main(int argc, char **argv)
{
    const char *out = argc > 1 ? argv[1] : "preview.png";
    const char *keys = argc > 2 ? argv[2] : "";

    scene_init(&sc);
    for (const char *k = keys; *k; k++) {
        switch (*k) {
        case 'L': run(10, RD_BTN_LEFT | RD_BTN_A, 0);  break;
        case 'R': run(10, RD_BTN_RIGHT | RD_BTN_A, 0); break;
        case 'U': run(10, RD_BTN_UP | RD_BTN_A, 0);    break;
        case 'D': run(10, RD_BTN_DOWN | RD_BTN_A, 0);  break;
        case 'A': run(10, RD_BTN_A, RD_BTN_A);         break;
        case 'B': run(10, RD_BTN_A, RD_BTN_B);         break;
        case 'S': run(1, 0, RD_BTN_START);             break;
        case '.': run(10, 0, 0);                       break;
        case 'f': run(1, RD_BTN_A, 0);                 break;
        case 'E': sc.energy = 1000;                    break;
        case 'F': sc.t = FLEET_T - 2;                  break;
        case '#': if (k[1]) { scene_debug(&sc, k[1] - '0', 0); k++; } break;
        default:
            if (*k >= '1' && *k <= '9')
                for (int i = 0; i < (*k - '0') * 10; i++)
                    run(10, RD_BTN_A | ((i / 3) & 1 ? RD_BTN_LEFT : RD_BTN_RIGHT), 0);
        }
    }

    for (int i = 0; i < RD_GFX_W * RD_GFX_H; i++) screen[i] = 0xF81F;
    for (int y = 0; y < RD_GFX_H; y += RD_STRIP_H)
        scene_render(y, RD_STRIP_H, screen + y * RD_GFX_W, &sc);

    if (rd_png_write_rgb565(out, screen, RD_GFX_W, RD_GFX_H)) {
        fprintf(stderr, "cannot write %s\n", out);
        return 1;
    }
    printf("%s  state=%d stage=%d t=%d fleet=%d/%d boss=%d bt=%d/%d rate=%d score=%lu energy=%d\n",
           out, sc.state, sc.stage, sc.t, sc.fleet, sc.fleet_t, sc.boss_on, sc.bt_killed, sc.bt_total, sc.rate,
           (unsigned long)sc.score, sc.energy);
    return 0;
}
