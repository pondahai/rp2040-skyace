/* SKY ACE —— 直向捲軸空戰射擊（致敬 1943 的玩法：能量條、翻滾閃避、POW 補給）。
 * 純 C，不碰硬體；實機 main.c 與 PC 預覽共用。
 *
 * 一次 scene_update() = 一幀（約 30 fps）。背景每幀捲動，所以每幀整個畫面重畫。
 *
 * 關卡：奇數關 = 空戰（魔王重轟炸機）；偶數關 = 空戰 3/4 後「ENEMY FLEET!」，
 * 下降過場，切成船艦關（魔王輪流是戰艦 / 航空母艦，緩慢捲過畫面，只有一次機會）。
 */
#ifndef SCENE_H
#define SCENE_H

#include <stdint.h>
#include "gfx.h"

#define MAX_PSHOT  24
#define MAX_ESHOT  48
#define MAX_ENEMY  32
#define MAX_ITEM   4
#define MAX_BOOM   20
#define MAX_CLOUD  3
#define MAP_W      20
#define MAP_H      16

#define STAGE_LEN  1500             /* 空戰關：約 50 秒後魔王 */
#define FLEET_T    1125             /* 船艦關：空戰打到 3/4 發現敵艦 */
#define FLEET_LEN  750              /* 船艦段：約 25 秒後魔王艦 */
#define DESCEND_LEN 104

enum { ST_TITLE, ST_PLAY, ST_DESCEND, ST_CLEAR, ST_OVER };

/* 音效事件（main.c 依優先順序挑一個播） */
enum {
    SFX_SHOT  = 1u << 0,
    SFX_HIT   = 1u << 1,
    SFX_BOOM  = 1u << 2,
    SFX_ITEM  = 1u << 3,
    SFX_LOOP  = 1u << 4,
    SFX_BIG   = 1u << 5,
    SFX_HURT  = 1u << 6,
    SFX_ALARM = 1u << 7,
    SFX_CLEAR = 1u << 8,
    SFX_OVER  = 1u << 9,
    SFX_DIVE  = 1u << 10,
};

typedef struct { float x, y, vx, vy; uint8_t on; } shot;

typedef struct {
    float x, y, vx, vy, x0;
    int16_t hp, t, cd, sink;        /* sink：船沉沒倒數 */
    int8_t  parent, ox, oy;         /* 砲塔：依附的船與相對位置 */
    uint8_t on, type, pat, group, flash, gun;
} enemy;

typedef struct { float x, y; int16_t t; uint8_t on, kind; } item;
typedef struct { int16_t x, y, t, size; } boom;
typedef struct { float x, y; int16_t w; } cloud;

typedef struct {
    uint8_t  state, paused;
    uint32_t frame, rng;
    int      stage, t;             /* t：關卡內幀數 */
    uint32_t score, hi;
    uint16_t sfx;

    /* 玩家 */
    float px, py;
    int   energy;                  /* 0..1000 */
    int   rolls, loop_t, inv_t, fire_cd, wpn_t, dead_t;

    int   red_left, red_killed, red_spawn, red_t;
    int   boss_on, boss_warn, state_t;

    /* 船艦關 */
    int   fleet;                   /* 1 = 低空船艦背景 */
    int   alert_t, desc_t, fleet_t;
    int   bt_total, bt_killed;     /* 魔王艦的砲塔數 / 已擊破 */
    int   rate;                    /* 過關時的擊破率 */
    float scroll_spd, scroll_acc;

    shot  ps[MAX_PSHOT], es[MAX_ESHOT];
    enemy en[MAX_ENEMY];
    item  it[MAX_ITEM];
    boom  bm[MAX_BOOM];
    cloud cl[MAX_CLOUD];

    /* 海面 / 島嶼 */
    uint8_t map[MAP_H * MAP_W];
    int     bgy;
    int     isl_cx, isl_w, isl_len, isl_j;
} scene;

void scene_init(scene *s);
/* held：按住的鍵；fired：剛按下的鍵（RD_BTN_*） */
void scene_update(scene *s, uint8_t held, uint8_t fired);
void scene_render(int y0, int h, uint16_t *buf, void *ctx);

/* 測試用：直接從第 stage 關的第 t 幀開始 */
void scene_debug(scene *s, int stage, int t);

#endif
