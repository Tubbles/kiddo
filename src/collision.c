#include "collision.h"
#include "shape.h"

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
