#ifndef COLLISION_H
#define COLLISION_H

#include "player.h"

#define MAX_COLLISIONS (MAX_PLAYERS * (MAX_PLAYERS - 1) / 2)

typedef struct {
    int player_a;
    int player_b;
    Vector2 contact;
} CollisionEvent;

int collision_detect(const PlayerState *players, int count,
                     CollisionEvent *events, int max_events);

void obb_corners(Vector2 center, float angle_deg,
                 float half_w, float half_h, Vector2 *out);

void project_corners(const Vector2 *corners, int count,
                     Vector2 axis, float *mn, float *mx);

void resolve_wall_collision_ex(PlayerState *player, float angle_deg,
                               float half_w, float half_h,
                               Rectangle wall);

void resolve_arena_collision(PlayerState *player, float angle_deg,
                             float half_w, float half_h);

#endif
