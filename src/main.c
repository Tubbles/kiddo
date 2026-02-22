#include "raylib.h"
#include "render.h"
#include "shape.h"

int main(void)
{
    InitWindow(800, 600, "Kiddo");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        render_background();

        render_shape(SHAPE_CIRCLE, (Vector2){200, 200}, 0.0f, 1.5f, RED);
        render_shape(SHAPE_SQUARE, (Vector2){400, 200}, 0.3f, 1.5f, BLUE);
        render_shape(SHAPE_TRIANGLE, (Vector2){600, 200}, 0.0f, 1.5f, GREEN);
        render_shape(SHAPE_STAR, (Vector2){400, 400}, 0.0f, 1.5f, GOLD);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
