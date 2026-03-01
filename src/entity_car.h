#ifndef ENTITY_CAR_H
#define ENTITY_CAR_H

#include "entity.h"

#define CAR_SIZE 100.0f
#define CAR_HALF_W 17.0f
#define CAR_HALF_H 43.0f

Entity entity_car_init(int gamepad_id, Vector2 pos, Color color,
                       Texture2D *textures, int tex_count);

#endif
