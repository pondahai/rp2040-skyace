/* scene.c 與 ships.c 共用的內部定義（不給 main.c 用）。 */
#ifndef INTERNAL_H
#define INTERNAL_H

#include "scene.h"

enum { E_FIGHTER, E_RED, E_BOMBER, E_BOSS,              /* 空中 */
       E_PT, E_DD, E_TR, E_BB, E_CV, E_TURRET };         /* 海上 */
enum { P_DIVE, P_SINE, P_SWOOP, P_SIDE, P_HOVER, P_NONE };
enum { I_POW, I_WPN };
enum { G_MAIN, G_SEC, G_AA };

#define IS_SHIP(t)  ((t) >= E_PT)
#define IS_BOSSHIP(t) ((t) == E_BB || (t) == E_CV)

#define C_HUD     RD_RGB565(16, 16, 28)
#define C_TEXT    RD_RGB565(230, 230, 240)
#define C_YEL     RD_RGB565(255, 220, 40)
#define C_RED     RD_RGB565(230, 50, 40)
#define C_ORG     RD_RGB565(255, 140, 20)

int    rndi(scene *s, int n);
enemy *spawn(scene *s, int type, float x, float y, int pat);
void   eshot(scene *s, float x, float y, float vx, float vy);
void   aimed(scene *s, float x, float y, float spd, float spread);
void   add_boom(scene *s, int x, int y, int size);
void   add_item(scene *s, float x, float y, int kind);
void   stage_end(scene *s, int rate);

/* ships.c */
void ships_spawn(scene *s);                     /* 船艦段：定期出小船 */
void ships_boss(scene *s);                      /* 魔王艦（依關卡輪流） */
void ship_update(scene *s, enemy *e);
int  ship_hitbox(const scene *s, const enemy *e, float *x, float *y, float *w, float *h);
void ship_hit(scene *s, enemy *e);              /* 被子彈打中一發 */
void ship_draw(const rd_strip *st, const scene *s, const enemy *e);

#endif
