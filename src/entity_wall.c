#include "entity_wall.h"

#include <stddef.h>

static void wall_render(const Entity *e)
{
    DrawRectangleRec(e->wall.rect, DARKGRAY);
}

static void wall_get_obb(const Entity *e, Vector2 *corners)
{
    Rectangle r = e->wall.rect;
    corners[0] = (Vector2){r.x, r.y};
    corners[1] = (Vector2){r.x + r.width, r.y};
    corners[2] = (Vector2){r.x + r.width, r.y + r.height};
    corners[3] = (Vector2){r.x, r.y + r.height};
}

static const EntityVTable wall_vtable = {
    .update = NULL,
    .render = wall_render,
    .get_obb = wall_get_obb,
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
