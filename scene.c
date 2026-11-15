#include <stdio.h>
#include <string.h>
#include <math.h>
#include "internal.h"
#include "buttons.h"

/* ---------------------------------------------------------------- 常數 */
#define TOPBAR   12                 /* HUD 高度 */
#define PSPEED   3.0f
#define ENERGY_MAX 1000

#define C_SEA     RD_RGB565(28, 84, 160)
#define C_SEA2    RD_RGB565(44, 104, 184)
#define C_DEEP    RD_RGB565(18, 56, 120)
#define C_DEEP2   RD_RGB565(34, 80, 150)
#define C_SAND    RD_RGB565(214, 196, 130)
#define C_GRASS   RD_RGB565(60, 140, 60)
#define C_GRASS2  RD_RGB565(40, 108, 44)
#define C_TREE    RD_RGB565(24, 80, 32)
#define C_CLOUD   RD_RGB565(236, 240, 248)
#define C_SHOT    RD_RGB565(255, 255, 160)
#define C_ESHOT   RD_RGB565(255, 90, 200)
#define C_SHADOW  RD_RGB565(12, 40, 90)
#define KEY       0xF81Fu           /* 洋紅 = 透明 */

/* 主角影子的位置：高空離得遠，低空貼近 */
#define SHADOW_AIR_X   22
#define SHADOW_AIR_Y   34
#define SHADOW_LOW_X   6
#define SHADOW_LOW_Y   12

/* ---------------------------------------------------------------- 亂數 */
static uint32_t rnd(scene *s)
{
    uint32_t x = s->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return s->rng = x;
}
int rndi(scene *s, int n) { return (int)(rnd(s) % (uint32_t)n); }

static int is_fleet_stage(int stage) { return stage % 2 == 0; }

/* ---------------------------------------------------------------- 圖素 */
static const char *PLAYER_ART[16] = {   /* 24×16：長主翼、雙尾撐 */
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
    "........................",
};
static const char *FIGHTER_ART[16] = {   /* 機頭朝下 */
    "................",
    "................",
    ".......gg.......",
    "....gggggggg....",
    ".....gggggg.....",
    ".......gg.......",
    ".......gg.......",
    "......gbbg......",
    "......gbbg......",
    ".gggggggggggggg.",
    "gyyggggggggggyyg",
    "gggggggggggggggg",
    "......gggg......",
    ".......gg.......",
    ".......kk.......",
    "................",
};

static uint16_t spr_player[24 * 16], spr_shadow[24 * 16];
static uint16_t spr_fighter[256], spr_red[256];
static uint16_t tiles[8 * 256];
static rd_tilemap tmap;

static void build(uint16_t *dst, const char **art, int w, uint16_t g, uint16_t hi, int mono)
{
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < w; x++) {
            uint16_t c = KEY;
            switch (art[y][x]) {
            case 'g': c = g; break;
            case 'w': c = RD_WHITE; break;
            case 'y': c = hi; break;
            case 'b': c = RD_RGB565(120, 200, 255); break;
            case 'k': c = RD_RGB565(40, 40, 40); break;
            case 'd': c = RD_RGB565(120, 126, 138); break;
            }
            if (mono && c != KEY && art[y][x] != 'y') c = mono == 1 ? C_SHADOW : c;
            if (mono && art[y][x] == 'y') c = KEY;
            dst[y * w + x] = c;
        }
}

static uint32_t hash(uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352d; x ^= x >> 15; x *= 0x846ca68b; x ^= x >> 16;
    return x;
}

/* 0 海 1 浪 2 沙 3 草 4 樹 5 深草 6 深海 7 深海浪 */
static void build_tiles(void)
{
    for (int t = 0; t < 8; t++)
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                uint32_t h = hash((uint32_t)(t * 977 + y * 16 + x));
                uint16_t c = C_SEA;
                switch (t) {
                case 0: c = (h & 63) == 0 ? C_SEA2 : C_SEA; break;
                case 1: c = ((y == 6 && x > 3 && x < 10) || (y == 12 && x > 8 && x < 14))
                            ? C_SEA2 : C_SEA; break;
                case 2: c = (h & 7) == 0 ? RD_RGB565(190, 172, 110) : C_SAND; break;
                case 3: c = (h & 7) == 0 ? C_GRASS2 : C_GRASS; break;
                case 4: {
                    int dx = x - 8, dy = y - 8;
                    c = dx * dx + dy * dy < 30 ? ((h & 3) ? C_TREE : C_GRASS2) : C_GRASS;
                    break; }
                case 5: c = (h & 3) == 0 ? C_GRASS : C_GRASS2; break;
                case 6: c = (h & 31) == 0 ? C_DEEP2 : C_DEEP; break;
                case 7: c = ((y == 4 && x > 1 && x < 9) || (y == 11 && x > 7 && x < 15))
                            ? C_DEEP2 : C_DEEP; break;
                }
                tiles[t * 256 + y * 16 + x] = c;
            }
}

static void gen_row(scene *s, uint8_t *row)
{
    if (s->fleet) {                                 /* 低空：整片深海，浪比較多 */
        for (int x = 0; x < MAP_W; x++) row[x] = (uint8_t)(rndi(s, 4) == 0 ? 7 : 6);
        return;
    }
    if (s->isl_len == 0 && rndi(s, 100) < 6) {
        s->isl_len = 6 + rndi(s, 10);
        s->isl_w = 2 + rndi(s, 5);
        s->isl_cx = 2 + rndi(s, MAP_W - 4);
        s->isl_j = 0;
    }
    int hw = -1;
    if (s->isl_len) {
        int L = s->isl_len, j = s->isl_j;
        hw = s->isl_w * 4 * j * (L - j) / (L * L);
        if (++s->isl_j > L) s->isl_len = 0;
    }
    for (int x = 0; x < MAP_W; x++) {
        int d = x - s->isl_cx; if (d < 0) d = -d;
        uint8_t t;
        if (hw >= 0 && d <= hw) {
            if (d == hw) t = 2;
            else t = (uint8_t)(rndi(s, 4) == 0 ? 4 : (rndi(s, 3) == 0 ? 5 : 3));
        } else t = rndi(s, 7) == 0 ? 1 : 0;
        row[x] = t;
    }
}

static void regen_map(scene *s)
{
    s->isl_len = 0;
    for (int y = MAP_H - 1; y >= 0; y--) gen_row(s, &s->map[y * MAP_W]);
}

/* ---------------------------------------------------------------- 物件生成 */
enemy *spawn(scene *s, int type, float x, float y, int pat)
{
    for (int i = 0; i < MAX_ENEMY; i++) {
        enemy *e = &s->en[i];
        if (e->on) continue;
        memset(e, 0, sizeof *e);
        e->on = 1; e->type = (uint8_t)type; e->pat = (uint8_t)pat;
        e->x = e->x0 = x; e->y = y;
        e->hp = type == E_BOMBER ? 14 + 3 * s->stage
              : type == E_BOSS   ? 160 + 60 * s->stage : 1;
        e->cd = 30 + rndi(s, 60);
        return e;
    }
    return 0;
}

void eshot(scene *s, float x, float y, float vx, float vy)
{
    for (int i = 0; i < MAX_ESHOT; i++)
        if (!s->es[i].on) { s->es[i] = (shot){ x, y, vx, vy, 1 }; return; }
}

void aimed(scene *s, float x, float y, float spd, float spread)
{
    float dx = s->px + 12 - x, dy = s->py + 8 - y;
    float a = atan2f(dy, dx) + spread;
    eshot(s, x, y, cosf(a) * spd, sinf(a) * spd);
}

void add_boom(scene *s, int x, int y, int size)
{
    for (int i = 0; i < MAX_BOOM; i++)
        if (s->bm[i].t <= 0) { s->bm[i] = (boom){ (int16_t)x, (int16_t)y, 16, (int16_t)size }; return; }
}

void add_item(scene *s, float x, float y, int kind)
{
    for (int i = 0; i < MAX_ITEM; i++)
        if (!s->it[i].on) { s->it[i] = (item){ x, y, 0, 1, (uint8_t)kind }; return; }
}

void stage_end(scene *s, int rate)
{
    if (s->dead_t || s->state != ST_PLAY) return;
    for (int i = 0; i < MAX_ESHOT; i++) s->es[i].on = 0;
    for (int i = 0; i < MAX_PSHOT; i++) s->ps[i].on = 0;
    s->rate = rate;
    s->state = ST_CLEAR; s->state_t = 0; s->sfx |= SFX_CLEAR;
}

/* 依關卡時間丟出一波敵機；sparse = 船艦段，敵機少一半 */
static void spawn_waves(scene *s, int t, int sparse)
{
    int every = 70 - 8 * s->stage; if (every < 30) every = 30;
    if (sparse) every *= 2;
    if (t % every == 0) {
        int k = rndi(s, 4);
        if (k == 0) {                                /* 一排俯衝 */
            int n = 3 + rndi(s, 3), x = 30 + rndi(s, 200);
            for (int i = 0; i < n; i++) spawn(s, E_FIGHTER, (float)(x + i * 22), (float)(-16 - i * 10), P_DIVE);
        } else if (k == 1) {                         /* 蛇行 */
            int x = 60 + rndi(s, 180);
            for (int i = 0; i < 4; i++) spawn(s, E_FIGHTER, (float)x, (float)(-16 - i * 24), P_SINE);
        } else if (k == 2) {                         /* 側面切入 */
            int left = rndi(s, 2), y = 30 + rndi(s, 70);
            for (int i = 0; i < 3; i++) {
                enemy *e = spawn(s, E_FIGHTER, left ? (float)(-16 - i * 24) : (float)(320 + i * 24), (float)y, P_SIDE);
                if (e) e->vx = left ? 2.5f : -2.5f;
            }
        } else {                                     /* 下拉後掉頭 */
            int x = 40 + rndi(s, 240);
            for (int i = 0; i < 2; i++) spawn(s, E_FIGHTER, (float)(x + i * 20 - 10), (float)(-16 - i * 16), P_SWOOP);
        }
    }
    if (sparse) return;
    if (t % 400 == 200 && s->red_left == 0) {       /* 紅色編隊：全滅掉道具 */
        s->red_left = 5; s->red_killed = 0; s->red_spawn = 5; s->red_t = 0;
    }
    if (s->red_spawn > 0 && s->red_t-- <= 0) {
        enemy *e = spawn(s, E_RED, -16, 40, P_SWOOP);
        if (e) { e->vx = 2.8f; e->vy = 1.2f; e->group = 1; }
        s->red_spawn--; s->red_t = 10;
    }
    if (t % 520 == 400) spawn(s, E_BOMBER, (float)(60 + rndi(s, 200)), -30, P_HOVER);
}

/* ---------------------------------------------------------------- 狀態 */
static void start_stage(scene *s)
{
    memset(s->en, 0, sizeof s->en);
    memset(s->es, 0, sizeof s->es);
    memset(s->ps, 0, sizeof s->ps);
    memset(s->it, 0, sizeof s->it);
    s->t = 0; s->rolls = 3; s->boss_on = 0; s->boss_warn = 0;
    s->red_left = s->red_spawn = 0;
    s->alert_t = s->desc_t = s->fleet_t = 0;
    s->bt_total = s->bt_killed = 0;
    s->scroll_spd = 1.0f;
    if (s->fleet) { s->fleet = 0; regen_map(s); }   /* 回到高空 */
    s->px = 148; s->py = 200; s->inv_t = 60; s->loop_t = 0;
}

static void new_game(scene *s)
{
    s->stage = 1; s->score = 0; s->energy = ENERGY_MAX; s->wpn_t = 0; s->dead_t = 0;
    start_stage(s);
    s->state = ST_PLAY; s->paused = 0;
}

void scene_init(scene *s)
{
    static int built;
    if (!built) {
        build(spr_player, PLAYER_ART, 24, RD_RGB565(170, 176, 186), C_YEL, 0);
        build(spr_shadow, PLAYER_ART, 24, 0, 0, 1);
        build(spr_fighter, FIGHTER_ART, 16, RD_RGB565(90, 130, 70), C_YEL, 0);
        build(spr_red, FIGHTER_ART, 16, RD_RGB565(210, 50, 40), RD_WHITE, 0);
        build_tiles();
        built = 1;
    }
    uint32_t hi = s->hi;
    memset(s, 0, sizeof *s);
    s->hi = hi ? hi : 20000;
    s->rng = 0x1943u;
    s->scroll_spd = 1.0f;
    regen_map(s);
    for (int i = 0; i < MAX_CLOUD; i++)
        s->cl[i] = (cloud){ (float)rndi(s, 300), (float)(rndi(s, 240)), (int16_t)(30 + rndi(s, 40)) };
    s->state = ST_TITLE;
}

void scene_debug(scene *s, int stage, int t)
{
    new_game(s);
    s->stage = stage;
    start_stage(s);
    s->t = t;
}

static void scroll_bg(scene *s)
{
    s->scroll_acc += s->scroll_spd;
    while (s->scroll_acc >= 1.0f) {
        s->scroll_acc -= 1.0f;
        if (++s->bgy >= 16) {
            s->bgy = 0;
            memmove(&s->map[MAP_W], &s->map[0], (MAP_H - 1) * MAP_W);
            gen_row(s, &s->map[0]);
        }
    }
    for (int i = 0; i < MAX_CLOUD; i++) {
        s->cl[i].y += 2.2f * s->scroll_spd;
        if (s->cl[i].y > 260) { s->cl[i].y = -40; s->cl[i].x = (float)rndi(s, 300); s->cl[i].w = (int16_t)(30 + rndi(s, 40)); }
    }
}

static int overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh)
{
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static void enemy_box(const enemy *e, float *w, float *h)
{
    switch (e->type) {
    case E_BOMBER: *w = 40; *h = 28; break;
    case E_BOSS:   *w = 96; *h = 56; break;
    default:       *w = 16; *h = 16; break;
    }
}

static int hitbox(const scene *s, const enemy *e, float *x, float *y, float *w, float *h)
{
    if (IS_SHIP(e->type)) return ship_hitbox(s, e, x, y, w, h);
    *x = e->x; *y = e->y; enemy_box(e, w, h);
    return 1;
}

static void hurt(scene *s, int dmg)
{
    if (s->inv_t > 0 || s->loop_t > 0 || s->dead_t) return;
    s->energy -= dmg; s->inv_t = 45; s->sfx |= SFX_HURT;
    s->wpn_t = 0;
}

static void kill_enemy(scene *s, enemy *e)
{
    e->on = 0;
    switch (e->type) {
    case E_FIGHTER: s->score += 100; add_boom(s, (int)(e->x + 8), (int)(e->y + 8), 10); s->sfx |= SFX_BOOM; break;
    case E_RED:
        s->score += 200; add_boom(s, (int)(e->x + 8), (int)(e->y + 8), 10); s->sfx |= SFX_BOOM;
        if (++s->red_killed == 5) { add_item(s, e->x, e->y, s->stage & 1 ? I_POW : I_WPN); s->score += 1000; }
        break;
    case E_BOMBER:
        s->score += 1500; s->sfx |= SFX_BIG;
        for (int i = 0; i < 4; i++) add_boom(s, (int)(e->x + rndi(s, 40)), (int)(e->y + rndi(s, 28)), 16);
        add_item(s, e->x + 12, e->y + 8, rndi(s, 2) ? I_POW : I_WPN);
        break;
    case E_BOSS:
        s->score += 20000; s->sfx |= SFX_BIG;
        for (int i = 0; i < 10; i++) add_boom(s, (int)(e->x + rndi(s, 96)), (int)(e->y + rndi(s, 56)), 22);
        stage_end(s, 100);
        break;
    }
    if (e->group && s->red_left) s->red_left--;
}

static void update_enemy(scene *s, enemy *e)
{
    if (IS_SHIP(e->type)) { ship_update(s, e); return; }
    e->t++;
    if (e->flash) e->flash--;
    float cx, cy, w, h; enemy_box(e, &w, &h);
    cx = e->x + w / 2; cy = e->y + h / 2;
    float esp = 2.2f + 0.3f * s->stage;

    switch (e->pat) {
    case P_DIVE:
        e->y += 2.6f + 0.2f * s->stage;
        if (e->t == 25 && rndi(s, 3) == 0) aimed(s, cx, cy, esp, 0);
        break;
    case P_SINE:
        e->y += 1.8f;
        e->x = e->x0 + 50 * sinf(e->t * 0.07f);
        if (--e->cd <= 0) { aimed(s, cx, cy, esp, 0); e->cd = 80; }
        break;
    case P_SIDE:
        e->x += e->vx; e->y += 0.4f;
        if (--e->cd <= 0) { aimed(s, cx, cy, esp, 0); e->cd = 70; }
        break;
    case P_SWOOP:
        if (e->type == E_RED) {                 /* 從左上弧線劃過再往上離開 */
            e->x += e->vx; e->y += e->vy; e->vy -= 0.025f;
        } else {
            e->y += e->vy = e->t < 40 ? 3.0f : e->vy - 0.18f;
            e->x += (e->x0 < 160 ? 1.0f : -1.0f) * (e->t > 40 ? 2.2f : 0);
            if (e->t == 40) aimed(s, cx, cy, esp, 0);
        }
        break;
    case P_HOVER:                               /* 轟炸機：下降、橫移、齊射，然後離開 */
        if (e->t < 60) e->y += 1.2f;
        else if (e->t < 420) e->x = e->x0 + 70 * sinf((e->t - 60) * 0.02f);
        else e->y += 1.0f;
        if (e->t > 40 && --e->cd <= 0) {
            for (int k = -1; k <= 1; k++) aimed(s, cx, cy + 10, esp, k * 0.25f);
            e->cd = 60 - 4 * s->stage;
        }
        break;
    }

    if (e->type == E_BOSS) {
        if (e->y < 24) e->y += 0.6f;
        else e->x = 112 + 90 * sinf(e->t * 0.012f);
        if (e->t > 80 && --e->cd <= 0) {
            int phase = (e->t / 240) % 3;
            if (phase == 0) for (int k = -3; k <= 3; k++) eshot(s, cx, cy + 20, k * 0.7f, 2.4f);
            else if (phase == 1) for (int k = -1; k <= 1; k++) aimed(s, cx + k * 34, cy + 10, esp + 0.5f, 0);
            else for (int k = 0; k < 10; k++) {
                float a = k * 0.628f + e->t * 0.05f;
                eshot(s, cx, cy, cosf(a) * 2.0f, sinf(a) * 2.0f);
            }
            e->cd = phase == 1 ? 25 : 55;
        }
    }

    if (e->y > 260 || e->y < -80 || e->x < -80 || e->x > 400) {
        if (e->group && s->red_left) s->red_left--;
        e->on = 0;
    }
}

/* 關卡進度：空戰 → (偶數關) 發現敵艦 → 下降 → 船艦段 → 魔王 */
static void stage_script(scene *s)
{
    if (s->boss_on) return;

    if (!is_fleet_stage(s->stage)) {
        if (s->t < STAGE_LEN) { spawn_waves(s, s->t, 0); return; }
        if (s->boss_warn == 0) { s->boss_warn = 1; s->sfx |= SFX_ALARM; }
        if (++s->boss_warn > 90) { spawn(s, E_BOSS, 112, -60, P_NONE); s->boss_on = 1; }
        return;
    }

    if (!s->fleet) {
        if (s->t < FLEET_T) { spawn_waves(s, s->t, 0); return; }
        if (s->alert_t == 0) s->sfx |= SFX_ALARM;
        if (++s->alert_t > 90) {                    /* 「ENEMY FLEET!」之後開始下降 */
            s->state = ST_DESCEND; s->desc_t = 0; s->sfx |= SFX_DIVE;
            for (int i = 0; i < MAX_ESHOT; i++) s->es[i].on = 0;
        }
        return;
    }

    s->fleet_t++;
    if (s->fleet_t < FLEET_LEN) {
        ships_spawn(s);
        spawn_waves(s, s->fleet_t, 1);
        return;
    }
    if (s->boss_warn == 0) { s->boss_warn = 1; s->sfx |= SFX_ALARM; }
    if (++s->boss_warn > 90) {
        ships_boss(s);
        s->boss_on = 1;
        s->scroll_spd = 0.25f;                      /* 魔王艦慢慢捲過，只有一次機會 */
    }
}

static void player_move(scene *s, uint8_t held)
{
    float dx = 0, dy = 0;
    if (held & RD_BTN_LEFT) dx -= PSPEED;
    if (held & RD_BTN_RIGHT) dx += PSPEED;
    if (held & RD_BTN_UP) dy -= PSPEED;
    if (held & RD_BTN_DOWN) dy += PSPEED;
    s->px += dx; s->py += dy;
    if (s->px < 0) s->px = 0;
    if (s->px > 296) s->px = 296;
    if (s->py < TOPBAR + 4) s->py = TOPBAR + 4;
    if (s->py > 222) s->py = 222;
}

static void update_objects(scene *s)
{
    /* 子彈：先打砲塔（在船上面），再打其他 */
    for (int i = 0; i < MAX_PSHOT; i++) {
        shot *b = &s->ps[i];
        if (!b->on) continue;
        b->x += b->vx; b->y += b->vy;
        if (b->y < -8 || b->x < -4 || b->x > 324) { b->on = 0; continue; }
        for (int pass = 0; pass < 2 && b->on; pass++)
            for (int j = 0; j < MAX_ENEMY; j++) {
                enemy *e = &s->en[j];
                if (!e->on || (e->type == E_TURRET) != (pass == 0)) continue;
                float x, y, w, h;
                if (!hitbox(s, e, &x, &y, &w, &h)) continue;
                if (!overlap(b->x, b->y, 2, 6, x, y, w, h)) continue;
                b->on = 0;
                if (IS_SHIP(e->type)) ship_hit(s, e);
                else {
                    e->flash = 3;
                    if (--e->hp <= 0) kill_enemy(s, e); else s->sfx |= SFX_HIT;
                }
                break;
            }
    }
    int vuln = !s->dead_t && s->loop_t == 0 && s->state == ST_PLAY;
    for (int i = 0; i < MAX_ESHOT; i++) {
        shot *b = &s->es[i];
        if (!b->on) continue;
        b->x += b->vx; b->y += b->vy;
        if (b->y < -8 || b->y > 248 || b->x < -8 || b->x > 328) { b->on = 0; continue; }
        if (vuln && overlap(b->x - 2, b->y - 2, 4, 4, s->px + 9, s->py + 3, 6, 9)) {
            b->on = 0; hurt(s, 120);
        }
    }

    for (int i = 0; i < MAX_ENEMY; i++) {
        enemy *e = &s->en[i];
        if (!e->on) continue;
        update_enemy(s, e);
        if (!e->on || IS_SHIP(e->type)) continue;  /* 船在海面，撞不到 */
        float w, h; enemy_box(e, &w, &h);
        if (vuln && overlap(s->px + 4, s->py + 4, 16, 6, e->x, e->y, w, h)) {
            hurt(s, 200);
            if (e->type <= E_RED) kill_enemy(s, e);
        }
    }

    for (int i = 0; i < MAX_ITEM; i++) {
        item *it = &s->it[i];
        if (!it->on) continue;
        it->t++; it->y += 0.8f; it->x += sinf(it->t * 0.08f) * 0.8f;
        if (it->y > 250) { it->on = 0; continue; }
        if (!s->dead_t && overlap(s->px, s->py, 24, 16, it->x, it->y, 14, 12)) {
            it->on = 0; s->sfx |= SFX_ITEM; s->score += 500;
            if (it->kind == I_POW) { s->energy += 300; if (s->energy > ENERGY_MAX) s->energy = ENERGY_MAX; }
            else s->wpn_t = 600;
        }
    }

    for (int i = 0; i < MAX_BOOM; i++) if (s->bm[i].t > 0) s->bm[i].t--;
    if (s->score > s->hi) s->hi = s->score;
}

static void update_play(scene *s, uint8_t held, uint8_t fired)
{
    if (fired & RD_BTN_START) s->paused ^= 1;
    if (s->paused) return;

    s->t++;
    scroll_bg(s);

    if (s->dead_t) {
        if (++s->dead_t > 90) { s->state = ST_OVER; s->state_t = 0; s->sfx |= SFX_OVER;
                                if (s->score > s->hi) s->hi = s->score; }
    } else {
        player_move(s, held);

        if ((fired & RD_BTN_B) && s->rolls > 0 && s->loop_t == 0) {
            s->rolls--; s->loop_t = 40; s->sfx |= SFX_LOOP;
        }
        if (s->loop_t) s->loop_t--;
        if (s->inv_t) s->inv_t--;
        if (s->wpn_t) s->wpn_t--;

        if (s->fire_cd) s->fire_cd--;
        if ((held & RD_BTN_A) && s->fire_cd == 0 && s->loop_t == 0) {
            int n = 0;
            for (int i = 0; i < MAX_PSHOT && n < (s->wpn_t ? 3 : 2); i++) {
                if (s->ps[i].on) continue;
                if (s->wpn_t) s->ps[i] = (shot){ s->px + 11, s->py - 4, (float)(n - 1) * 1.6f, -8, 1 };
                else          s->ps[i] = (shot){ s->px + 5 + n * 12, s->py - 2, 0, -8, 1 };
                n++;
            }
            s->fire_cd = 4; s->sfx |= SFX_SHOT;
        }

        /* 能量隨時間消耗（1943 的燃料） */
        if (s->frame % 3 == 0) s->energy--;
        if (s->energy <= 0) {
            s->energy = 0; s->dead_t = 1; s->sfx |= SFX_BIG;
            add_boom(s, (int)s->px + 12, (int)s->py + 8, 24);
        }
    }

    stage_script(s);
    if (s->state == ST_PLAY) update_objects(s);
}

/* 下降過場：主角越縮越小、影子靠近，白雲蓋住畫面時切成低空背景，雲散開回到正常大小 */
#define DESC_SWITCH 70
static void update_descend(scene *s)
{
    int t = ++s->desc_t;
    if (t < DESC_SWITCH) {
        s->scroll_spd = 1.0f + t / 70.0f;
        s->px += (148 - s->px) * 0.06f;
        s->py += (170 - s->py) * 0.06f;
    } else if (t == DESC_SWITCH) {
        s->fleet = 1;
        regen_map(s);
        memset(s->en, 0, sizeof s->en);             /* 高空的敵機留在上面 */
        s->red_left = s->red_spawn = 0;
        s->scroll_spd = 1.5f;
    } else {
        s->scroll_spd = 1.5f - 0.5f * (t - DESC_SWITCH) / (DESCEND_LEN - DESC_SWITCH);
    }
    scroll_bg(s);
    for (int i = 0; i < MAX_PSHOT; i++) if (s->ps[i].on) { s->ps[i].y += s->ps[i].vy; if (s->ps[i].y < -8) s->ps[i].on = 0; }
    for (int i = 0; i < MAX_BOOM; i++) if (s->bm[i].t > 0) s->bm[i].t--;
    if (t >= DESCEND_LEN) {
        s->state = ST_PLAY; s->scroll_spd = 1.0f; s->fleet_t = 0;
        s->boss_warn = 0; s->inv_t = 30; s->alert_t = 0;
    }
}

void scene_update(scene *s, uint8_t held, uint8_t fired)
{
    s->frame++;
    s->sfx = 0;
    switch (s->state) {
    case ST_TITLE:
        scroll_bg(s);
        if (fired & (RD_BTN_A | RD_BTN_START)) new_game(s);
        break;
    case ST_PLAY:
        update_play(s, held, fired);
        break;
    case ST_DESCEND:
        update_descend(s);
        break;
    case ST_CLEAR:
        s->scroll_spd = 1.0f;
        scroll_bg(s);
        for (int i = 0; i < MAX_BOOM; i++) if (s->bm[i].t > 0) s->bm[i].t--;
        if (++s->state_t == 1)
            s->score += (uint32_t)s->energy * 10 * (uint32_t)s->rate / 100 + (uint32_t)s->rate * 100;
        if (s->state_t > 150) {
            s->stage++; s->energy += 400;
            if (s->energy > ENERGY_MAX) s->energy = ENERGY_MAX;
            start_stage(s); s->state = ST_PLAY;
        }
        break;
    case ST_OVER:
        scroll_bg(s);
        if (++s->state_t > 45 && (fired & (RD_BTN_A | RD_BTN_START))) s->state = ST_TITLE;
        break;
    }
}

/* ---------------------------------------------------------------- 畫面 */
/* 大飛機（轟炸機 / 魔王）用矩形組出來，機頭朝下。sc = 放大倍數 */
static void draw_big(const rd_strip *st, int x, int y, int sc, uint16_t body, uint16_t wing, int flash)
{
    if (flash) body = wing = RD_WHITE;
    int w = 20 * sc;                              /* 半寬 */
    uint16_t dark = RD_RGB565(40, 40, 40);
    int cx = x + w;
    rd_rect(st, cx - 7 * sc, y, 14 * sc, 3 * sc, wing);               /* 尾翼 */
    rd_rect(st, cx - 2 * sc, y, 4 * sc, 26 * sc, body);               /* 機身 */
    rd_rect(st, x, y + 9 * sc, 2 * w, 5 * sc, wing);                  /* 主翼 */
    for (int k = -1; k <= 1; k += 2) {
        rd_rect(st, cx + k * 8 * sc - sc, y + 8 * sc, 3 * sc, 9 * sc, body);   /* 內側引擎 */
        rd_rect(st, cx + k * 15 * sc - sc, y + 8 * sc, 3 * sc, 8 * sc, body);  /* 外側引擎 */
        rd_rect(st, cx + k * 8 * sc - sc, y + 17 * sc, 3 * sc, sc, dark);
        rd_rect(st, cx + k * 15 * sc - sc, y + 16 * sc, 3 * sc, sc, dark);
    }
    rd_rect(st, cx - sc, y + 22 * sc, 2 * sc, 3 * sc, RD_RGB565(120, 200, 255));  /* 座艙 */
}

/* 縮放貼圖（最近鄰）。(cx,cy) 是中心，scale256 = 256 為原尺寸。過場用。 */
static void blit_scaled(const rd_strip *st, int cx, int cy, int w, int h,
                        const uint16_t *src, int scale256)
{
    int dw = w * scale256 / 256, dh = h * scale256 / 256;
    if (dw < 1 || dh < 1) return;
    int x0 = cx - dw / 2, y0 = cy - dh / 2;
    for (int dy = 0; dy < dh; dy++) {
        int y = y0 + dy;
        if (y < st->y0 || y >= st->y0 + st->h) continue;
        const uint16_t *row = src + (dy * h / dh) * w;
        uint16_t *out = st->px + (y - st->y0) * RD_GFX_W;
        for (int dx = 0; dx < dw; dx++) {
            int x = x0 + dx;
            if (x < 0 || x >= RD_GFX_W) continue;
            uint16_t c = row[dx * w / dw];
            if (c != KEY) out[x] = c;
        }
    }
}

static void center_text(const rd_strip *st, int y, const char *t, uint16_t c, int scale)
{
    int w = (int)strlen(t) * 8 * scale;
    rd_text(st, (RD_GFX_W - w) / 2, y, t, c, scale == 1 ? C_HUD : RD_NOKEY, scale);
}

static void draw_player(const rd_strip *st, const scene *s)
{
    int x = (int)s->px, y = (int)s->py;
    int sx = s->fleet ? SHADOW_LOW_X : SHADOW_AIR_X, sy = s->fleet ? SHADOW_LOW_Y : SHADOW_AIR_Y;
    int scale = 256;

    if (s->state == ST_DESCEND) {
        int t = s->desc_t;
        if (t < DESC_SWITCH) {
            scale = 256 - 154 * t / DESC_SWITCH;    /* 100% → 40% */
            sx = SHADOW_AIR_X + (SHADOW_LOW_X - SHADOW_AIR_X) * t / DESC_SWITCH;
            sy = SHADOW_AIR_Y + (SHADOW_LOW_Y - SHADOW_AIR_Y) * t / DESC_SWITCH;
        }
        blit_scaled(st, x + 12 + sx * scale / 256, y + 8 + sy * scale / 256, 24, 16, spr_shadow, scale * 3 / 4);
        blit_scaled(st, x + 12, y + 8, 24, 16, spr_player, scale);
        return;
    }

    if (s->dead_t) return;
    if (s->loop_t) {                                /* 翻滾：拉高（變大、影子遠）再落回 */
        int ph = s->loop_t, up = ph > 20 ? 40 - ph : ph;       /* 0..20 */
        blit_scaled(st, x + 12 + sx + up, y + 8 + sy + up, 24, 16, spr_shadow, 192);
        blit_scaled(st, x + 12, y + 8 - up, 24, 16, spr_player, 256 + up * 5);
        return;
    }
    blit_scaled(st, x + 12 + sx, y + 8 + sy, 24, 16, spr_shadow, 192);
    if (!(s->inv_t && (s->frame & 2)))
        rd_blit(st, x, y, 24, 16, spr_player, KEY);
}

void scene_render(int y0, int h, uint16_t *buf, void *ctx)
{
    const scene *s = ctx;
    rd_strip st = { buf, y0, h };
    char line[40];

    rd_fill(&st, C_SEA);
    tmap.tiles = tiles; tmap.map = s->map;
    tmap.tw = tmap.th = 16; tmap.mw = MAP_W; tmap.mh = MAP_H;
    tmap.x = 0; tmap.y = (int16_t)(s->bgy - 16);
    rd_draw_tilemap(&st, &tmap);

    /* 海面：船（船身先、砲塔後） */
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < MAX_ENEMY; i++) {
            const enemy *e = &s->en[i];
            if (e->on && IS_SHIP(e->type) && (e->type == E_TURRET) == (pass == 1)) ship_draw(&st, s, e);
        }

    /* 道具 */
    for (int i = 0; i < MAX_ITEM; i++) {
        const item *it = &s->it[i];
        if (!it->on) continue;
        int x = (int)it->x, y = (int)it->y;
        uint16_t c = (s->frame / 4) & 1 ? C_RED : C_ORG;
        rd_rect(&st, x, y, 26, 12, c);
        rd_text(&st, x + 1, y + 2, it->kind == I_POW ? "POW" : "3WY", RD_WHITE, RD_NOKEY, 1);
    }

    /* 敵機 */
    for (int i = 0; i < MAX_ENEMY; i++) {
        const enemy *e = &s->en[i];
        if (!e->on || IS_SHIP(e->type)) continue;
        int x = (int)e->x, y = (int)e->y;
        switch (e->type) {
        case E_FIGHTER: rd_blit(&st, x, y, 16, 16, e->flash ? spr_red : spr_fighter, KEY); break;
        case E_RED:     rd_blit(&st, x, y, 16, 16, spr_red, KEY); break;
        case E_BOMBER:  draw_big(&st, x, y, 1, RD_RGB565(90, 110, 70), RD_RGB565(70, 90, 56), e->flash); break;
        case E_BOSS:    draw_big(&st, x + 8, y, 2, RD_RGB565(70, 74, 90), RD_RGB565(54, 58, 74), e->flash); break;
        }
    }

    /* 子彈 */
    for (int i = 0; i < MAX_PSHOT; i++)
        if (s->ps[i].on) rd_rect(&st, (int)s->ps[i].x, (int)s->ps[i].y, 2, 6, C_SHOT);
    for (int i = 0; i < MAX_ESHOT; i++)
        if (s->es[i].on) {
            int x = (int)s->es[i].x, y = (int)s->es[i].y;
            rd_rect(&st, x - 2, y - 1, 4, 2, C_ESHOT);
            rd_rect(&st, x - 1, y - 2, 2, 4, C_ESHOT);
        }

    /* 爆炸 */
    for (int i = 0; i < MAX_BOOM; i++) {
        const boom *b = &s->bm[i];
        if (b->t <= 0) continue;
        int r = b->size * (17 - b->t) / 16;
        uint16_t c = b->t > 10 ? RD_WHITE : b->t > 5 ? C_YEL : C_ORG;
        rd_rect(&st, b->x - r, b->y - r / 2, 2 * r, r, c);
        rd_rect(&st, b->x - r / 2, b->y - r, r, 2 * r, c);
        if (b->t < 8) rd_rect(&st, b->x - r / 3, b->y - r / 3, 2 * r / 3, 2 * r / 3, C_RED);
    }

    if (s->state == ST_PLAY || s->state == ST_DESCEND) draw_player(&st, s);

    /* 雲：只有高空才有 */
    if (!s->fleet)
        for (int i = 0; i < MAX_CLOUD; i++) {
            int x = (int)s->cl[i].x, y = (int)s->cl[i].y, w = s->cl[i].w;
            rd_rect(&st, x, y + 6, w, 10, C_CLOUD);
            rd_rect(&st, x + w / 5, y, w / 2, 8, C_CLOUD);
            rd_rect(&st, x + w / 2, y + 2, w / 3, 6, C_CLOUD);
        }

    /* 下降過場的雲層：從上往下蓋滿，切換後從上往下散開 */
    if (s->state == ST_DESCEND) {
        int t = s->desc_t;
        if (t >= DESC_SWITCH - 16 && t < DESC_SWITCH) {
            int edge = (t - (DESC_SWITCH - 16)) * 256 / 16;
            rd_rect(&st, 0, 0, RD_GFX_W, edge, C_CLOUD);
            for (int k = 0; k < 8; k++) rd_rect(&st, k * 40 + 4, edge, 32, 8, C_CLOUD);
        } else if (t >= DESC_SWITCH) {
            int top = (t - DESC_SWITCH) * 256 / (DESCEND_LEN - DESC_SWITCH);
            rd_rect(&st, 0, top, RD_GFX_W, RD_GFX_H - top, C_CLOUD);
            for (int k = 0; k < 8; k++) rd_rect(&st, k * 40 + 4, top - 8, 32, 8, C_CLOUD);
        }
    }

    /* HUD */
    rd_rect(&st, 0, 0, RD_GFX_W, TOPBAR, C_HUD);
    snprintf(line, sizeof line, "%07lu", (unsigned long)s->score);
    rd_text(&st, 2, 2, line, C_TEXT, RD_NOKEY, 1);
    snprintf(line, sizeof line, "HI %07lu", (unsigned long)s->hi);
    rd_text(&st, 66, 2, line, C_YEL, RD_NOKEY, 1);
    if (s->state != ST_TITLE) {
        int e = s->energy * 100 / ENERGY_MAX;
        uint16_t ec = e > 40 ? RD_RGB565(80, 220, 90) : e > 20 ? C_YEL : C_RED;
        rd_text(&st, 154, 2, "E", C_TEXT, RD_NOKEY, 1);
        rd_rect(&st, 164, 3, 102, 6, RD_RGB565(60, 60, 70));
        rd_rect(&st, 165, 4, e, 4, ec);
        snprintf(line, sizeof line, "R%d S%d", s->rolls, s->stage);
        rd_text(&st, 272, 2, line, C_TEXT, RD_NOKEY, 1);
    }

    switch (s->state) {
    case ST_TITLE:
        center_text(&st, 70, "SKY ACE", C_YEL, 4);
        center_text(&st, 112, "PACIFIC 1943", C_TEXT, 2);
        if ((s->frame / 15) & 1) center_text(&st, 160, "PRESS A TO START", RD_WHITE, 1);
        center_text(&st, 190, "A:FIRE  B:LOOP  START:PAUSE", C_TEXT, 1);
        center_text(&st, 204, "COLLECT POW TO REFUEL", C_TEXT, 1);
        break;
    case ST_PLAY:
        if (s->paused) center_text(&st, 112, "PAUSE", RD_WHITE, 2);
        else if (s->alert_t && ((s->frame / 8) & 1))
            center_text(&st, 100, "ENEMY FLEET!", C_RED, 3);
        else if (s->boss_warn && !s->boss_on && ((s->frame / 8) & 1))
            center_text(&st, 100, "WARNING", C_RED, 3);
        else if (s->t < 60) {
            snprintf(line, sizeof line, "STAGE %d", s->stage);
            center_text(&st, 104, line, RD_WHITE, 2);
        } else if (s->fleet && s->fleet_t < 60)
            center_text(&st, 104, "ATTACK THE FLEET", RD_WHITE, 2);
        if (!s->dead_t && s->energy < 200 && ((s->frame / 10) & 1))
            center_text(&st, 222, "LOW ENERGY", C_RED, 1);
        break;
    case ST_DESCEND:
        if (s->desc_t < DESC_SWITCH - 16) center_text(&st, 60, "DESCEND", C_YEL, 2);
        break;
    case ST_CLEAR:
        center_text(&st, 80, s->rate == 100 ? "MISSION COMPLETE" : "TARGET ESCAPED", C_YEL, 2);
        snprintf(line, sizeof line, "DESTROY RATE %d%%", s->rate);
        center_text(&st, 112, line, s->rate == 100 ? RD_WHITE : C_ORG, 1);
        snprintf(line, sizeof line, "BONUS %lu",
                 (unsigned long)((uint32_t)s->energy * 10 * (uint32_t)s->rate / 100 + (uint32_t)s->rate * 100));
        center_text(&st, 128, line, RD_WHITE, 1);
        break;
    case ST_OVER:
        center_text(&st, 100, "GAME OVER", C_RED, 3);
        if (s->state_t > 45) center_text(&st, 140, "PRESS A", RD_WHITE, 1);
        break;
    }
}
