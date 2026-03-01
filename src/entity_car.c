#include "entity_car.h"
#include "collision.h"
#include "player.h"

#include <math.h>

static float angle_from_stick(Vector2 stick, float current_angle)
{
    if (fabsf(stick.x) < 0.2f && fabsf(stick.y) < 0.2f)
        return current_angle;
    return atan2f(stick.y, stick.x) * RAD2DEG + 90.0f;
}

static void car_update(Entity *e, InputState input, float dt)
{
    PlayerState ps = {
        .active = e->active,
        .shape = SHAPE_CIRCLE,
        .position = e->position,
        .rotation = 0.0f,
        .scale = e->scale,
        .color = e->color,
        .gamepad_id = e->gamepad_id,
    };
    ps = player_update(ps, input, dt);
    e->position = ps.position;
    e->scale = ps.scale;
    e->color = ps.color;

    e->car.facing_angle = angle_from_stick(input.left_stick,
                                            e->car.facing_angle);

    /* Cycle car texture on button press */
    if (input.buttons[1] && e->car.tex_count > 0)
        e->car.tex_index = (e->car.tex_index + 1) % e->car.tex_count;
}

static void car_render(const Entity *e)
{
    Texture2D *tex = &e->car.textures[e->car.tex_index];
    float car_scale = CAR_SIZE / (float)tex->width;
    Rectangle src = {0, 0, (float)tex->width, (float)tex->height};
    float dw = (float)tex->width * car_scale;
    float dh = (float)tex->height * car_scale;
    Rectangle dst = {e->position.x, e->position.y, dw, dh};
    Vector2 origin = {dw / 2, dh / 2};
    DrawTexturePro(*tex, src, dst, origin, e->car.facing_angle, e->color);
}

static void car_get_collision_shape(const Entity *e, CollisionShape *out)
{
    *out = (CollisionShape){0};
    out->count = 1;
    out->prims[0].kind = COLLIDER_RECT;
    out->prims[0].offset = (Vector2){0, 0};
    out->prims[0].angle_offset = e->car.facing_angle;
    out->prims[0].rect.half_w = CAR_HALF_W;
    out->prims[0].rect.half_h = CAR_HALF_H;
}

static const EntityVTable car_vtable = {
    .update = car_update,
    .render = car_render,
    .get_collision_shape = car_get_collision_shape,
};

Entity entity_car_init(int gamepad_id, Vector2 pos, Color color,
                       Texture2D *textures, int tex_count)
{
    Entity e = {0};
    e.kind = ENTITY_CAR;
    e.vtable = &car_vtable;
    e.active = true;
    e.position = pos;
    e.rotation = 0.0f;
    e.scale = 1.0f;
    e.color = color;
    e.gamepad_id = gamepad_id;
    e.car.facing_angle = 0.0f;
    e.car.textures = textures;
    e.car.tex_count = tex_count;
    e.car.tex_index = 0;
    return e;
}
