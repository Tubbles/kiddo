#ifndef PLAYER_H
#define PLAYER_H

#include "input.h"
#include "shape.h"
#include <stdbool.h>

#define MAX_PLAYERS 4
#define PLAYER_SPEED 300.0f
#define PLAYER_ROT_SPEED 3.0f
#define PLAYER_SCALE_MIN 0.5f
#define PLAYER_SCALE_MAX 3.0f
#define PLAYER_SCALE_SPEED 2.0f
#define SCREEN_WIDTH_DEFAULT 800
#define SCREEN_HEIGHT_DEFAULT 600

extern int screen_width;
extern int screen_height;

typedef struct {
    bool active;
    ShapeKind shape;
    Vector2 position;
    float rotation;
    float scale;
    Color color;
    int gamepad_id;
} PlayerState;

PlayerState player_init(int gamepad_id, Vector2 start_pos, ShapeKind shape, Color color);
PlayerState player_update(PlayerState state, InputState input, float dt);

#endif
