#include "raylib.h"
#include "input.h"
#include "player.h"
#include "render.h"

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Kiddo");
    SetTargetFPS(60);

    PlayerState player = player_init(0,
        (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f},
        SHAPE_CIRCLE, RED);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (IsGamepadAvailable(0)) {
            InputState input = input_read(0);
            player = player_update(player, input, dt);
        }

        BeginDrawing();
        render_background();
        render_shape(player.shape, player.position, player.rotation,
                     player.scale, player.color);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
