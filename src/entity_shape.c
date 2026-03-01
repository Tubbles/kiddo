#include "entity_shape.h"
#include "collision.h"
#include "player.h"
#include "render.h"

static void shape_update(Entity *e, InputState input, float dt)
{
    PlayerState ps = {
        .active = e->active,
        .shape = e->shape.shape,
        .position = e->position,
        .rotation = e->rotation * DEG2RAD,
        .scale = e->scale,
        .color = e->color,
        .gamepad_id = e->gamepad_id,
    };
    ps = player_update(ps, input, dt);
    e->position = ps.position;
    e->rotation = ps.rotation * RAD2DEG;
    e->scale = ps.scale;
    e->color = ps.color;
    e->shape.shape = ps.shape;
}

static void shape_render(const Entity *e)
{
    render_shape(e->shape.shape, e->position,
                 e->rotation * DEG2RAD, e->scale, e->color);
}

static void shape_get_obb(const Entity *e, Vector2 *corners)
{
    float sz = SHAPE_BASE_SIZE * e->scale;
    obb_corners(e->position, e->rotation, sz, sz, corners);
}

static const EntityVTable shape_vtable = {
    .update = shape_update,
    .render = shape_render,
    .get_obb = shape_get_obb,
};

Entity entity_shape_init(int gamepad_id, Vector2 pos, ShapeKind shape, Color color)
{
    Entity e = {0};
    e.kind = ENTITY_SHAPE;
    e.vtable = &shape_vtable;
    e.active = true;
    e.position = pos;
    e.rotation = 0.0f;
    e.scale = 1.0f;
    e.color = color;
    e.gamepad_id = gamepad_id;
    e.shape.shape = shape;
    return e;
}
