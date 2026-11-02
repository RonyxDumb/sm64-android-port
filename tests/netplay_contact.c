#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "netplay_contact.h"
static struct LanPlayerState player(float x) {
    struct LanPlayerState p = {0};
    p.x = x; p.level = 9; p.area = 1; p.act = 1; p.contact = LAN_PLAYER_SOLID;
    return p;
}
static void push_test(int mover) {
    struct LanPlayerState a = player(0), b = player(90);
    float dx, dz, initial_a = a.x, initial_b = b.x;
    for (int frame = 0; frame < 60; ++frame) {
        float old_a = a.x, old_b = b.x;
        a.vx = mover == 0 ? 8 : 0; b.vx = mover == 1 ? -8 : 0;
        a.x += a.vx; b.x += b.vx;
        struct LanPlayerState view_a = a, view_b = b;
        if (lan_contact_solve(&view_a, &view_b, old_a, 0, 1, &dx, &dz)) a.x += dx;
        if (lan_contact_solve(&view_b, &view_a, old_b, 0, 0, &dx, &dz)) b.x += dx;
        assert(a.x <= b.x && isfinite(a.x) && isfinite(b.x));
    }
    if (!mover) assert(b.x > initial_b + 200 && a.x > initial_a + 200);
    else assert(a.x < initial_a - 200 && b.x < initial_b - 200);
    assert(fabsf(b.x - a.x - 78) < .3f);
}
static void delayed_push(void) {
    struct LanPlayerState a = player(0), b = player(100), past_a[4], past_b[4];
    float dx, dz;
    for (int i = 0; i < 4; ++i) { past_a[i] = a; past_b[i] = b; }
    for (int frame = 0; frame < 90; ++frame) {
        float start_a = a.x, start_b = b.x;
        a.vx = 32; a.x += 32;
        struct LanPlayerState pa = past_a[frame % 4], pb = past_b[frame % 4];
        if (lan_contact_solve(&a, &pb, start_a, 0, 1, &dx, &dz)) a.x += dx;
        if (lan_contact_solve(&b, &pa, start_b, 0, 0, &dx, &dz)) b.x += dx;
        past_a[frame % 4] = a; past_b[frame % 4] = b;
        assert(a.x < b.x && isfinite(a.x) && isfinite(b.x));
    }
    assert(a.x > 200 && b.x > 300);
}
int main(void) {
    float dx, dz;
    struct LanPlayerState a = player(0), b = player(60);
    b.act = 6;
    assert(lan_contact_solve(&a, &b, a.x, 0, 1, &dx, &dz) && dx < 0); // different acts collide
    b.contact = 0; assert(!lan_contact_solve(&a, &b, 0, 0, 1, &dx, &dz));
    b.contact = 1; b.area = 2; assert(!lan_contact_solve(&a, &b, 0, 0, 1, &dx, &dz));
    b.area = 1; b.y = 180; assert(!lan_contact_solve(&a, &b, 0, 0, 1, &dx, &dz));
    b.y = 0; b.x = 0;
    assert(lan_contact_solve(&a, &b, 0, 0, 1, &dx, &dz) && dx > 0);
    assert(lan_contact_solve(&a, &b, 0, 0, 0, &dx, &dz) && dx < 0);
    b.x = 100; a.x = 200; // dash through a stationary peer
    assert(lan_contact_solve(&a, &b, 1, 0, 1, &dx, &dz) && a.x + dx <= b.x - 77.9f);
    a.x = 110; // crossing while the previous frame already overlaps
    assert(lan_contact_solve(&a, &b, 70, 0, 1, &dx, &dz) && a.x + dx <= b.x - 77.9f);
    a = player(0); b = player(60); a.vx = 32; b.contact |= LAN_PLAYER_BLOCKED;
    assert(lan_contact_solve(&a, &b, 0, 0, 1, &dx, &dz) && fabsf(dx + 18) < .01f);
    b.vx = NAN; assert(!lan_contact_solve(&a, &b, 0, 0, 1, &dx, &dz));
    push_test(0); push_test(1); delayed_push();
    puts("contact: mutual pushing, different acts, blocked peer, sweeps, height, intangible and area PASS");
}
