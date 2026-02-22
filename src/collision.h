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

#endif
