/* 船艦：巡邏艇、驅逐艦、運輸船、魔王戰艦 / 航空母艦，以及船上的砲塔。
 *
 * 船浮在海面上，跟著背景一起捲動（scroll_spd）再加上自己的航速。機頭朝下。
 * 砲塔是獨立的 enemy（type = E_TURRET），用 parent + ox/oy 掛在船上，位置每次
 * 從船算出來，所以永遠跟船貼齊。船沉了砲塔一起消失。
 */
#include <math.h>
#include "internal.h"

#define C_HULL    RD_RGB565(128, 134, 144)
#define C_HULL_D  RD_RGB565(96, 102, 112)
#define C_DECK    RD_RGB565(176, 150, 104)
#define C_FDECK   RD_RGB565(84, 88, 96)
#define C_SUPER   RD_RGB565(186, 190, 198)
#define C_RUST    RD_RGB565(120, 96, 80)
#define C_SINK    RD_RGB565(50, 60, 80)
#define C_WAKE    RD_RGB565(210, 230, 250)
#define C_WAKE2   RD_RGB565(110, 150, 200)
#define C_GUN     RD_RGB565(70, 74, 82)
#define C_GUN_L   RD_RGB565(150, 154, 162)
#define C_WIN     RD_RGB565(120, 200, 255)
#define C_DARK    RD_RGB565(30, 30, 34)

/* 船的尺寸：PT DD TR BB CV */
static const uint8_t SHIP_W[] = { 12, 20, 24, 56, 56 };
static const uint8_t SHIP_H[] = { 28, 64, 56, 120, 120 };
#define SW(t) SHIP_W[(t) - E_PT]
#define SH(t) SHIP_H[(t) - E_PT]

static void turret_pos(const scene *s, const enemy *e, float *x, float *y)
{
    const enemy *p = &s->en[e->parent];
    *x = p->x + e->ox;
    *y = p->y + e->oy;
}

static void add_turret(scene *s, enemy *ship, int ox, int oy, int gun)
{
    enemy *t = spawn(s, E_TURRET, 0, 0, P_NONE);
    if (!t) return;
    t->parent = (int8_t)(ship - s->en);
    t->ox = (int8_t)ox; t->oy = (int8_t)oy; t->gun = (uint8_t)gun;
    t->hp = gun == G_MAIN ? 10 + s->stage : gun == G_SEC ? 6 : 4;
    t->cd = 40 + rndi(s, 60);
    if (IS_BOSSHIP(ship->type)) s->bt_total++;
}

static enemy *new_ship(scene *s, int type, float x)
{
    enemy *e = spawn(s, type, x, (float)-SH(type) - 8, P_NONE);
    if (!e) return 0;
    switch (type) {
    case E_PT: e->hp = 3; e->vy = 0.9f; e->vx = x < 150 ? 0.5f : -0.5f; e->cd = 40; break;
    case E_DD: e->hp = 22 + 2 * s->stage; e->vy = 0.3f; break;
    case E_TR: e->hp = 14; e->vy = 0.15f; break;
    default:   e->hp = 70 + 10 * s->stage; e->vy = 0; break;
    }
    return e;
}

void ships_spawn(scene *s)
{
    if (s->fleet_t % 100 != 10) return;
    int k = rndi(s, 20);
    if (k < 8) {                                    /* 兩艘巡邏艇 */
        int x = 20 + rndi(s, 260);
        new_ship(s, E_PT, (float)x);
        enemy *e = new_ship(s, E_PT, (float)(x + 24 > 300 ? x - 24 : x + 24));
        if (e) e->y -= 30;
    } else if (k < 15) {                            /* 驅逐艦：前後兩座砲 */
        enemy *e = new_ship(s, E_DD, (float)(20 + rndi(s, 260)));
        if (e) { add_turret(s, e, 5, 6, G_SEC); add_turret(s, e, 5, 44, G_SEC); }
    } else {
        new_ship(s, E_TR, (float)(20 + rndi(s, 256)));
    }
}

void ships_boss(scene *s)
{
    s->bt_total = s->bt_killed = 0;
    if (s->stage % 4 == 2) {                        /* 戰艦：前後各兩座主砲、兩舷副砲 */
        enemy *e = new_ship(s, E_BB, 132);
        if (!e) return;
        add_turret(s, e, 23, 6, G_MAIN);
        add_turret(s, e, 23, 22, G_MAIN);
        add_turret(s, e, 23, 86, G_MAIN);
        add_turret(s, e, 23, 102, G_MAIN);
        add_turret(s, e, 4, 56, G_SEC);
        add_turret(s, e, 42, 56, G_SEC);
    } else {                                        /* 航空母艦：四角機槍座、會放艦載機 */
        enemy *e = new_ship(s, E_CV, 132);
        if (!e) return;
        add_turret(s, e, 1, 10, G_AA);
        add_turret(s, e, 45, 10, G_AA);
        add_turret(s, e, 1, 96, G_AA);
        add_turret(s, e, 45, 96, G_AA);
    }
}

static void ship_destroy(scene *s, enemy *e)
{
    int w = SW(e->type), h = SH(e->type);
    s->sfx |= SFX_BIG;
    switch (e->type) {
    case E_PT: s->score += 300; e->sink = 20; s->sfx = (uint16_t)((s->sfx & ~SFX_BIG) | SFX_BOOM); break;
    case E_DD: s->score += 1500; e->sink = 48; add_item(s, e->x, e->y + 20, I_POW); break;
    case E_TR: s->score += 2000; e->sink = 48; break;
    default:
        s->score += 30000; e->sink = 100;
        for (int i = 0; i < MAX_ESHOT; i++) s->es[i].on = 0;
        break;
    }
    for (int i = 0; i < (h > 60 ? 6 : 3); i++)
        add_boom(s, (int)e->x + rndi(s, w), (int)e->y + rndi(s, h), w > 30 ? 22 : 14);
}

static void turret_destroy(scene *s, enemy *e, int score)
{
    float x, y; turret_pos(s, e, &x, &y);
    e->hp = 0;                                      /* 留著當彈坑，跟船一起動 */
    add_boom(s, (int)x + 5, (int)y + 5, 12);
    s->sfx |= SFX_BOOM;
    if (score) {
        s->score += e->gun == G_MAIN ? 800 : 400;
        if (IS_BOSSHIP(s->en[e->parent].type)) s->bt_killed++;
    }
}

void ship_hit(scene *s, enemy *e)
{
    if (e->type == E_TURRET) {
        e->flash = 3;
        if (--e->hp <= 0) turret_destroy(s, e, 1); else s->sfx |= SFX_HIT;
        return;
    }
    if (IS_BOSSHIP(e->type) && s->bt_killed * 2 < s->bt_total) return;   /* 砲塔打掉一半前打不動 */
    e->flash = 3;
    if (--e->hp <= 0) ship_destroy(s, e); else s->sfx |= SFX_HIT;
}

int ship_hitbox(const scene *s, const enemy *e, float *x, float *y, float *w, float *h)
{
    if (e->type == E_TURRET) {
        if (e->hp <= 0) return 0;
        turret_pos(s, e, x, y); *w = *h = 10;
        return 1;
    }
    if (e->sink) return 0;
    switch (e->type) {
    case E_BB: *x = e->x + 20; *y = e->y + 46; *w = 16; *h = 26; break;   /* 艦橋 */
    case E_CV: *x = e->x + 40; *y = e->y + 42; *w = 12; *h = 30; break;   /* 艦島 */
    default:   *x = e->x; *y = e->y; *w = SW(e->type); *h = SH(e->type); break;
    }
    return 1;
}

static void drop_children(scene *s, const enemy *ship)
{
    int idx = (int)(ship - s->en);
    for (int i = 0; i < MAX_ENEMY; i++)
        if (s->en[i].on && s->en[i].type == E_TURRET && s->en[i].parent == idx) s->en[i].on = 0;
}

void ship_update(scene *s, enemy *e)
{
    float esp = 2.0f + 0.25f * s->stage;
    e->t++;
    if (e->flash) e->flash--;

    if (e->type == E_TURRET) {
        const enemy *p = &s->en[e->parent];
        if (!p->on || p->sink || p->type < E_PT) { e->on = 0; return; }
        if (e->hp <= 0) return;
        float x, y; turret_pos(s, e, &x, &y);
        if (y < 8 || y > 200) return;               /* 還沒進畫面不開火 */
        if (--e->cd <= 0) {
            float cx = x + 5, cy = y + 5;
            if (e->gun == G_MAIN) {
                for (int k = -1; k <= 1; k++) aimed(s, cx, cy, esp - 0.4f, k * 0.22f);
                e->cd = 110 - 6 * s->stage;
            } else {
                aimed(s, cx, cy, esp, 0);
                e->cd = e->gun == G_SEC ? 60 : 50;
            }
            if (e->cd < 30) e->cd = 30;
        }
        return;
    }

    int w = SW(e->type), h = SH(e->type);
    e->y += s->scroll_spd + e->vy;
    e->x += e->vx;

    if (e->sink) {
        if (e->sink % 10 == 0) add_boom(s, (int)e->x + rndi(s, w), (int)e->y + rndi(s, h), w > 30 ? 20 : 12);
        if (--e->sink == 0) {
            e->on = 0; drop_children(s, e);
            if (IS_BOSSHIP(e->type)) stage_end(s, 100);
        }
        return;
    }

    switch (e->type) {
    case E_PT:
        if (--e->cd <= 0 && e->y > 0 && e->y < 180) { aimed(s, e->x + 6, e->y + 26, esp, 0); e->cd = 70; }
        break;
    case E_BB:
    case E_CV:
        e->x = e->x0 + 30 * sinf(e->t * 0.008f);
        if (e->type == E_CV && e->y > -60 && e->y < 120 && e->t % (110 - 6 * s->stage) == 0) {
            enemy *f = spawn(s, E_FIGHTER, e->x + 20, e->y + 104, P_SWOOP);    /* 從艦艏起飛 */
            if (f) f->vy = 3.0f;
        }
        break;
    }

    if (e->y > RD_GFX_H + 4) {                      /* 開出畫面 */
        e->on = 0; drop_children(s, e);
        if (IS_BOSSHIP(e->type))
            stage_end(s, s->bt_total ? s->bt_killed * 70 / s->bt_total : 0);
    }
}

/* ---------------------------------------------------------------- 畫面 */
static void hull(const rd_strip *st, int x, int y, int w, int h, uint16_t c)
{
    rd_rect(st, x + 2, y, w - 4, 4, c);                     /* 艦艉 */
    rd_rect(st, x, y + 4, w, h - 16, c);
    rd_rect(st, x + 1, y + h - 12, w - 2, 4, c);
    rd_rect(st, x + w / 6, y + h - 8, w - w / 3, 4, c);
    rd_rect(st, x + w / 2 - w / 6, y + h - 4, w / 3 > 2 ? w / 3 : 2, 4, c);   /* 艦艏 */
}

static void wake(const rd_strip *st, const scene *s, int x, int y, int w, int h)
{
    int f = (int)(s->frame / 3) & 1;
    rd_rect(st, x + 2, y - 5, w - 4, 5, C_WAKE);
    rd_rect(st, x + 3 - f, y - 12, w - 6 + 2 * f, 7, C_WAKE2);
    rd_rect(st, x + w / 4, y - 20, w / 2, 8, C_WAKE2);
    rd_rect(st, x - 2, y + h - 10, 2, 6, C_WAKE);           /* 艦艏浪花 */
    rd_rect(st, x + w, y + h - 10, 2, 6, C_WAKE);
}

static void draw_turret(const rd_strip *st, const scene *s, const enemy *e)
{
    float x, y; turret_pos(s, e, &x, &y);
    int cx = (int)x + 5, cy = (int)y + 5;
    if (e->hp <= 0) {
        rd_rect(st, cx - 4, cy - 4, 8, 8, C_DARK);
        rd_rect(st, cx - 2, cy - 2, 4, 4, RD_RGB565(110, 40, 20));
        return;
    }
    uint16_t base = e->flash ? RD_WHITE : C_GUN;
    float dx = s->px + 12 - cx, dy = s->py + 8 - cy, d = sqrtf(dx * dx + dy * dy);
    if (d < 1) d = 1;
    dx /= d; dy /= d;
    int len = e->gun == G_MAIN ? 10 : 7;
    for (int r = 3; r <= len; r += 2) {
        if (e->gun == G_MAIN) {
            rd_rect(st, cx + (int)(dx * r - dy * 2) - 1, cy + (int)(dy * r + dx * 2) - 1, 2, 2, C_DARK);
            rd_rect(st, cx + (int)(dx * r + dy * 2) - 1, cy + (int)(dy * r - dx * 2) - 1, 2, 2, C_DARK);
        } else {
            rd_rect(st, cx + (int)(dx * r) - 1, cy + (int)(dy * r) - 1, 2, 2, C_DARK);
        }
    }
    int r = e->gun == G_MAIN ? 5 : 4;
    rd_rect(st, cx - r, cy - r + 1, 2 * r, 2 * r - 2, base);
    rd_rect(st, cx - r + 1, cy - r, 2 * r - 2, 2 * r, base);
    rd_rect(st, cx - 2, cy - 2, 4, 3, e->flash ? RD_WHITE : C_GUN_L);
}

void ship_draw(const rd_strip *st, const scene *s, const enemy *e)
{
    if (e->type == E_TURRET) { draw_turret(st, s, e); return; }

    int x = (int)e->x, y = (int)e->y, w = SW(e->type), h = SH(e->type);
    if (y > st->y0 + st->h || y + h < st->y0 - 24) return;

    if (e->sink) {                                  /* 沉沒：變暗、往中間縮 */
        int full = IS_BOSSHIP(e->type) ? 100 : e->type == E_PT ? 20 : 48;
        int shrink = (full - e->sink) * w / (3 * full);
        hull(st, x + shrink, y + shrink, w - 2 * shrink, h - 2 * shrink, C_SINK);
        rd_rect(st, x + w / 2 - 2, y + h / 3, 4, h / 3, RD_RGB565(30, 40, 60));
        return;
    }

    wake(st, s, x, y, w, h);
    uint16_t hc = e->flash ? RD_WHITE : C_HULL;

    switch (e->type) {
    case E_PT:
        hull(st, x, y, w, h, hc);
        rd_rect(st, x + 3, y + 8, 6, 8, C_SUPER);
        rd_rect(st, x + 4, y + 13, 4, 2, C_WIN);
        rd_rect(st, x + 5, y + 20, 2, 4, C_DARK);
        break;
    case E_DD:
        hull(st, x, y, w, h, hc);
        rd_rect(st, x + 2, y + 4, w - 4, h - 18, C_HULL_D);
        rd_rect(st, x + 5, y + 20, 10, 18, C_SUPER);
        rd_rect(st, x + 7, y + 24, 6, 6, C_DARK);           /* 煙囪 */
        rd_rect(st, x + 6, y + 34, 8, 2, C_WIN);
        break;
    case E_TR:
        hull(st, x, y, w, h, e->flash ? RD_WHITE : C_RUST);
        rd_rect(st, x + 4, y + 4, 16, 10, C_SUPER);         /* 船艉艙房 */
        rd_rect(st, x + 6, y + 10, 12, 2, C_WIN);
        for (int k = 0; k < 3; k++) rd_rect(st, x + 5, y + 18 + k * 10, 14, 7, RD_RGB565(90, 70, 56));
        break;
    case E_BB:
        hull(st, x, y, w, h, hc);
        rd_rect(st, x + 3, y + 4, w - 6, h - 18, C_DECK);
        rd_rect(st, x + 16, y + 40, 24, 38, C_SUPER);       /* 上層結構 */
        rd_rect(st, x + 24, y + 36, 8, 8, C_DARK);          /* 煙囪 */
        {
            uint16_t bc = e->flash ? RD_WHITE : (s->bt_killed * 2 < s->bt_total ? C_HULL_D : C_ORG);
            rd_rect(st, x + 20, y + 48, 16, 22, bc);        /* 艦橋：可攻擊時變橘 */
            rd_rect(st, x + 22, y + 64, 12, 3, C_WIN);
        }
        break;
    case E_CV:
        hull(st, x, y, w, h, hc);
        rd_rect(st, x + 3, y + 4, 44, h - 14, C_FDECK);     /* 飛行甲板 */
        for (int k = 0; k < 7; k++) rd_rect(st, x + 24, y + 10 + k * 15, 2, 8, RD_WHITE);
        rd_rect(st, x + 4, y + 4, 2, h - 16, RD_RGB565(200, 200, 200));
        for (int k = 0; k < 2; k++) {                       /* 停在甲板後段的艦載機 */
            int px = x + 10 + k * 18, py = y + 20;
            rd_rect(st, px, py + 3, 10, 2, RD_RGB565(90, 130, 70));
            rd_rect(st, px + 4, py, 2, 9, RD_RGB565(90, 130, 70));
        }
        {
            uint16_t ic = e->flash ? RD_WHITE : (s->bt_killed * 2 < s->bt_total ? C_SUPER : C_ORG);
            rd_rect(st, x + 42, y + 42, 10, 30, ic);        /* 艦島：可攻擊時變橘 */
            rd_rect(st, x + 44, y + 50, 6, 6, C_DARK);
            rd_rect(st, x + 43, y + 66, 8, 2, C_WIN);
        }
        break;
    }
}
