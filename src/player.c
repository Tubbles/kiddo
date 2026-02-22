#include "player.h"

static float clampf(float val, float min, float max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

static ShapeKind next_shape(ShapeKind current)
{
    return (ShapeKind)((current + 1) % SHAPE_COUNT);
}

static Color next_color(Color current)
{
    Color colors[] = {RED, BLUE, GREEN, GOLD, PURPLE, ORANGE, PINK, SKYBLUE};
    int count = sizeof(colors) / sizeof(colors[0]);
    for (int i = 0; i < count; i++) {
        if (current.r == colors[i].r && current.g == colors[i].g &&
            current.b == colors[i].b)
            return colors[(i + 1) % count];
    }
    return colors[0];
}

PlayerState player_init(int gamepad_id, Vector2 start_pos, ShapeKind shape, Color color)
{
    return (PlayerState){
        .active = true,
        .shape = shape,
        .position = start_pos,
        .rotation = 0.0f,
        .scale = 1.0f,
        .color = color,
        .gamepad_id = gamepad_id,
    };
}

PlayerState player_update(PlayerState state, InputState input, float dt)
{
    state.position.x += input.left_stick.x * PLAYER_SPEED * dt;
    state.position.y += input.left_stick.y * PLAYER_SPEED * dt;

    state.rotation += input.right_stick.x * PLAYER_ROT_SPEED * dt;

    float scale_input = input.right_trigger - input.left_trigger;
    state.scale += scale_input * PLAYER_SCALE_SPEED * dt;
    state.scale = clampf(state.scale, PLAYER_SCALE_MIN, PLAYER_SCALE_MAX);

    float margin = SHAPE_BASE_SIZE * state.scale;
    state.position.x = clampf(state.position.x, margin, SCREEN_WIDTH - margin);
    state.position.y = clampf(state.position.y, margin, SCREEN_HEIGHT - margin);

    if (input.buttons[0])
        state.color = next_color(state.color);
    if (input.buttons[1])
        state.shape = next_shape(state.shape);

    return state;
}
