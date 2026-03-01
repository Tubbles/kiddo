#include "collision.h"
#include "shape.h"

#include <math.h>

#define ARENA_PAD 20.0f
#define ARENA_RADIUS 80.0f

int collision_detect(const PlayerState *players, int count,
                     CollisionEvent *events, int max_events)
{
    int found = 0;

    for (int i = 0; i < count && found < max_events; i++) {
        if (!players[i].active)
            continue;

        for (int j = i + 1; j < count && found < max_events; j++) {
            if (!players[j].active)
                continue;

            if (shape_overlap(players[i].shape, players[i].position,
                              players[i].scale, players[j].shape,
                              players[j].position, players[j].scale)) {
                Vector2 contact = {
                    (players[i].position.x + players[j].position.x) * 0.5f,
                    (players[i].position.y + players[j].position.y) * 0.5f,
                };
                events[found++] = (CollisionEvent){
                    .player_a = i,
                    .player_b = j,
                    .contact = contact,
                };
            }
        }
    }

    return found;
}

void project_corners(const Vector2 *corners, int count,
                     Vector2 axis, float *mn, float *mx)
{
    *mn = *mx = corners[0].x * axis.x + corners[0].y * axis.y;
    for (int i = 1; i < count; i++) {
        float p = corners[i].x * axis.x + corners[i].y * axis.y;
        if (p < *mn) *mn = p;
        if (p > *mx) *mx = p;
    }
}

void obb_corners(Vector2 center, float angle_deg,
                 float half_w, float half_h, Vector2 *out)
{
    float rad = angle_deg * DEG2RAD;
    float c = cosf(rad), s = sinf(rad);
    Vector2 ax = {c, s};
    Vector2 ay = {-s, c};
    out[0] = (Vector2){center.x - half_w*ax.x - half_h*ay.x,
                       center.y - half_w*ax.y - half_h*ay.y};
    out[1] = (Vector2){center.x + half_w*ax.x - half_h*ay.x,
                       center.y + half_w*ax.y - half_h*ay.y};
    out[2] = (Vector2){center.x + half_w*ax.x + half_h*ay.x,
                       center.y + half_w*ax.y + half_h*ay.y};
    out[3] = (Vector2){center.x - half_w*ax.x + half_h*ay.x,
                       center.y - half_w*ax.y + half_h*ay.y};
}

void resolve_wall_collision_ex(PlayerState *player, float angle_deg,
                               float half_w, float half_h,
                               Rectangle wall)
{
    float rad = angle_deg * DEG2RAD;
    float c = cosf(rad), s = sinf(rad);
    Vector2 car_ax = {c, s};
    Vector2 car_ay = {-s, c};

    Vector2 car_corners[4];
    obb_corners(player->position, angle_deg, half_w, half_h, car_corners);

    Vector2 wall_corners[4] = {
        {wall.x, wall.y},
        {wall.x + wall.width, wall.y},
        {wall.x + wall.width, wall.y + wall.height},
        {wall.x, wall.y + wall.height},
    };

    /* SAT: 2 car axes + 2 wall axes (x, y) */
    Vector2 axes[4] = {car_ax, car_ay, {1, 0}, {0, 1}};

    float min_overlap = 1e9f;
    Vector2 push_axis = {0};

    for (int a = 0; a < 4; a++) {
        float c_mn, c_mx, w_mn, w_mx;
        project_corners(car_corners, 4, axes[a], &c_mn, &c_mx);
        project_corners(wall_corners, 4, axes[a], &w_mn, &w_mx);

        float overlap = fminf(c_mx - w_mn, w_mx - c_mn);
        if (overlap <= 0) return; /* separated */

        if (overlap < min_overlap) {
            min_overlap = overlap;
            float car_mid = (c_mn + c_mx) * 0.5f;
            float wall_mid = (w_mn + w_mx) * 0.5f;
            float sign = (car_mid > wall_mid) ? 1.0f : -1.0f;
            push_axis = (Vector2){axes[a].x * sign, axes[a].y * sign};
        }
    }

    player->position.x += push_axis.x * min_overlap;
    player->position.y += push_axis.y * min_overlap;
}

void resolve_arena_collision(PlayerState *player, float angle_deg,
                             float half_w, float half_h)
{
    float thick = 200.0f;
    float r = ARENA_RADIUS;
    float w = (float)screen_width;
    float h = (float)screen_height;

    /* 4 edge walls just outside the arena */
    Rectangle edge_walls[4] = {
        {ARENA_PAD - thick, ARENA_PAD,         thick, h - 2 * ARENA_PAD}, /* left */
        {w - ARENA_PAD,     ARENA_PAD,         thick, h - 2 * ARENA_PAD}, /* right */
        {ARENA_PAD,         ARENA_PAD - thick,  w - 2 * ARENA_PAD, thick}, /* top */
        {ARENA_PAD,         h - ARENA_PAD,      w - 2 * ARENA_PAD, thick}, /* bottom */
    };
    for (int i = 0; i < 4; i++)
        resolve_wall_collision_ex(player, angle_deg, half_w, half_h,
                                  edge_walls[i]);

    /* Corner arcs — push OBB corners inside the arc */
    Vector2 corner_centers[4] = {
        {ARENA_PAD + r,     ARENA_PAD + r},
        {w - ARENA_PAD - r, ARENA_PAD + r},
        {ARENA_PAD + r,     h - ARENA_PAD - r},
        {w - ARENA_PAD - r, h - ARENA_PAD - r},
    };
    for (int c = 0; c < 4; c++) {
        Vector2 ob[4];
        obb_corners(player->position, angle_deg, half_w, half_h, ob);
        for (int j = 0; j < 4; j++) {
            float dx = ob[j].x - corner_centers[c].x;
            float dy = ob[j].y - corner_centers[c].y;
            bool in_corner = false;
            if (c == 0) in_corner = dx < 0 && dy < 0;
            if (c == 1) in_corner = dx > 0 && dy < 0;
            if (c == 2) in_corner = dx < 0 && dy > 0;
            if (c == 3) in_corner = dx > 0 && dy > 0;
            if (!in_corner) continue;

            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > r && dist > 0.001f) {
                float push = dist - r;
                float nx = dx / dist;
                float ny = dy / dist;
                player->position.x -= nx * push;
                player->position.y -= ny * push;
            }
        }
    }
}
