#ifndef ENTITY_H
#define ENTITY_H

#include "input.h"
#include "shape.h"
#include <stdbool.h>

typedef enum {
    ENTITY_SHAPE,
    ENTITY_CAR,
    ENTITY_WALL,
    ENTITY_PARKING,
} EntityKind;

typedef struct Entity Entity;

typedef struct {
    void (*update)(Entity *e, InputState input, float dt);
    void (*render)(const Entity *e);
    void (*get_obb)(const Entity *e, Vector2 *corners);
} EntityVTable;

struct Entity {
    EntityKind kind;
    const EntityVTable *vtable;
    bool active;
    Vector2 position;
    float rotation;  /* degrees */
    float scale;
    Color color;
    int gamepad_id;
    union {
        struct { ShapeKind shape; } shape;
        struct { float facing_angle; Texture2D *tex; } car;
        struct { Rectangle rect; } wall;
        struct { Rectangle rect; Font *font; } parking;
    };
};

#define MAX_ENTITIES 32

#endif
