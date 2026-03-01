#include "entity_wall.h"

#include <stddef.h>

static void wall_render(const Entity *e)
{
    DrawRectangleRec(e->wall.rect, DARKGRAY);
}

static void wall_get_collision_shape(const Entity *e, CollisionShape *out)
{
    Rectangle r = e->wall.rect;
    *out = (CollisionShape){0};
    out->count = 1;
    out->prims[0].kind = COLLIDER_RECT;
    out->prims[0].offset = (Vector2){0, 0};
    out->prims[0].angle_offset = 0;
    out->prims[0].rect.half_w = r.width / 2;
    out->prims[0].rect.half_h = r.height / 2;
}

static const EntityVTable wall_vtable = {
    .update = NULL,
    .render = wall_render,
    .get_collision_shape = wall_get_collision_shape,
};

Entity entity_wall_init(Rectangle rect)
{
    Entity e = {0};
    e.kind = ENTITY_WALL;
    e.vtable = &wall_vtable;
    e.active = true;
    e.position = (Vector2){rect.x + rect.width / 2,
                           rect.y + rect.height / 2};
    e.wall.rect = rect;
    return e;
}
