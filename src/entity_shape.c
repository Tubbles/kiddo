#include "entity_shape.h"
#include "collision.h"
#include "player.h"
#include "render.h"

#include <math.h>

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

static void shape_get_collision_shape(const Entity *e, CollisionShape *out)
{
    float sz = SHAPE_BASE_SIZE * e->scale;
    *out = (CollisionShape){0};
    out->count = 1;
    out->prims[0].offset = (Vector2){0, 0};

    switch (e->shape.shape) {
    case SHAPE_CIRCLE:
        out->prims[0].kind = COLLIDER_CIRCLE;
        out->prims[0].circle.radius = sz;
        break;
    case SHAPE_TRIANGLE: {
        out->prims[0].kind = COLLIDER_TRIANGLE;
        float rot = e->rotation * DEG2RAD;
        for (int i = 0; i < 3; i++) {
            float a = rot + (float)i * 2.0f * PI / 3.0f - PI / 2.0f;
            out->prims[0].triangle.verts[i] = (Vector2){
                cosf(a) * sz, sinf(a) * sz};
        }
        break;
    }
    case SHAPE_STAR:
        out->prims[0].kind = COLLIDER_CIRCLE;
        out->prims[0].circle.radius = sz * 0.75f;
        break;
    default: /* SHAPE_SQUARE */
        out->prims[0].kind = COLLIDER_RECT;
        out->prims[0].angle_offset = 0;
        out->prims[0].rect.half_w = sz;
        out->prims[0].rect.half_h = sz;
        break;
    }
}

static const EntityVTable shape_vtable = {
    .update = shape_update,
    .render = shape_render,
    .get_collision_shape = shape_get_collision_shape,
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
