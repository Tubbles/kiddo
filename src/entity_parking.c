#include "entity_parking.h"

#include <stddef.h>

static void parking_render(const Entity *e)
{
    DrawRectangleRec(e->parking.rect, GRAY);
    Vector2 center = {
        e->parking.rect.x + e->parking.rect.width / 2,
        e->parking.rect.y + e->parking.rect.height / 2,
    };
    Vector2 sz = MeasureTextEx(*e->parking.font, "P", 80.0f, 1);
    DrawTextEx(*e->parking.font, "P",
               (Vector2){center.x - sz.x / 2, center.y - sz.y / 2},
               80.0f, 1, WHITE);
}

static void parking_get_collision_shape(const Entity *e, CollisionShape *out)
{
    Rectangle r = e->parking.rect;
    *out = (CollisionShape){0};
    out->count = 1;
    out->prims[0].kind = COLLIDER_RECT;
    out->prims[0].offset = (Vector2){0, 0};
    out->prims[0].angle_offset = 0;
    out->prims[0].rect.half_w = r.width / 2;
    out->prims[0].rect.half_h = r.height / 2;
}

static const EntityVTable parking_vtable = {
    .update = NULL,
    .render = parking_render,
    .get_collision_shape = parking_get_collision_shape,
};

Entity entity_parking_init(Rectangle rect, Font *font)
{
    Entity e = {0};
    e.kind = ENTITY_PARKING;
    e.vtable = &parking_vtable;
    e.active = true;
    e.position = (Vector2){rect.x + rect.width / 2,
                           rect.y + rect.height / 2};
    e.parking.rect = rect;
    e.parking.font = font;
    return e;
}
