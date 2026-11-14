/* SKY ACE —— 直向捲軸空戰射擊。遊戲內容在 scene.c；這裡只做硬體的事。
 *
 * 背景每幀捲動，所以每幀整個畫面重送（約 40 ms），遊戲以約 25–30 fps 跑。
 */
#include "pico/stdlib.h"
#include "display.h"
#include "dpad.h"
#include "audio.h"
#include "buttons.h"
#include "gfx.h"
#include "scene.h"

static const rd_note SFX_SHOT_S[]  = { {1800, 12}, {0, 0} };
static const rd_note SFX_HIT_S[]   = { {900, 15}, {0, 0} };
static const rd_note SFX_BOOM_S[]  = { {220, 30}, {150, 30}, {100, 40}, {0, 0} };
static const rd_note SFX_BIG_S[]   = { {180, 50}, {120, 60}, {90, 80}, {60, 120}, {0, 0} };
static const rd_note SFX_ITEM_S[]  = { {880, 50}, {1320, 50}, {1760, 90}, {0, 0} };
static const rd_note SFX_LOOP_S[]  = { {500, 40}, {700, 40}, {900, 40}, {700, 40}, {500, 40}, {0, 0} };
static const rd_note SFX_HURT_S[]  = { {300, 60}, {200, 80}, {0, 0} };
static const rd_note SFX_ALARM_S[] = { {990, 150}, {660, 150}, {990, 150}, {660, 150}, {990, 150}, {660, 150}, {0, 0} };
static const rd_note SFX_CLEAR_S[] = { {784, 120}, {988, 120}, {1175, 120}, {1568, 300}, {0, 0} };
static const rd_note SFX_OVER_S[]  = { {440, 200}, {392, 200}, {349, 200}, {262, 500}, {0, 0} };

static const struct { uint16_t bit; const rd_note *seq; } SFX_TABLE[] = {   /* 優先順序 */
    { SFX_OVER, SFX_OVER_S }, { SFX_CLEAR, SFX_CLEAR_S }, { SFX_ALARM, SFX_ALARM_S },
    { SFX_BIG, SFX_BIG_S },   { SFX_HURT, SFX_HURT_S },   { SFX_ITEM, SFX_ITEM_S },
    { SFX_LOOP, SFX_LOOP_S }, { SFX_BOOM, SFX_BOOM_S },   { SFX_HIT, SFX_HIT_S },
    { SFX_SHOT, SFX_SHOT_S },
};

static scene sc;

int main(void)
{
    rd_display_init();
    rd_dpad_init();
    rd_audio_init();
    scene_init(&sc);

    rd_buttons btn;
    rd_buttons_init(&btn, 0);

    uint16_t playing_prio = 0xFF;       /* 正在播的音效在表中的位置；長音效不被射擊聲蓋掉 */
    uint32_t playing_until = 0;

    for (;;) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        uint8_t fired = rd_buttons_update(&btn, rd_dpad_raw(), now);
        sc.rng ^= now;                  /* 每局不同 */

        scene_update(&sc, btn.held, fired);

        if (now >= playing_until) playing_prio = 0xFF;
        for (unsigned i = 0; i < sizeof SFX_TABLE / sizeof SFX_TABLE[0]; i++) {
            if (!(sc.sfx & SFX_TABLE[i].bit)) continue;
            if (i <= playing_prio) {
                uint32_t len = 0;
                for (const rd_note *n = SFX_TABLE[i].seq; n->ms; n++) len += n->ms;
                rd_audio_play(SFX_TABLE[i].seq);
                playing_prio = (uint16_t)i; playing_until = now + len;
            }
            break;
        }

        rd_display_present(RD_DIRTY_ALL, scene_render, &sc);

        uint32_t spent = to_ms_since_boot(get_absolute_time()) - now;
        if (spent < 33) sleep_ms(33 - spent);
    }
}
