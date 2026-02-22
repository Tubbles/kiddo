#include "raylib.h"
#include "audio.h"
#include "collision.h"
#include "input.h"
#include "particle.h"
#include "player.h"
#include "render.h"

static const Color PLAYER_COLORS[MAX_PLAYERS] = {
    {230, 41, 55, 255},   /* RED */
    {0, 121, 241, 255},   /* BLUE */
    {0, 228, 48, 255},    /* GREEN */
    {253, 249, 0, 255},   /* YELLOW */
};

static const ShapeKind PLAYER_SHAPES[MAX_PLAYERS] = {
    SHAPE_CIRCLE,
    SHAPE_SQUARE,
    SHAPE_TRIANGLE,
    SHAPE_STAR,
};

static Vector2 player_start_pos(int index)
{
    float quarter_w = SCREEN_WIDTH / 4.0f;
    float half_h = SCREEN_HEIGHT / 2.0f;
    return (Vector2){quarter_w * (index * 2 + 1) * 0.5f, half_h};
}

static bool any_button_pressed(const InputState *input)
{
    for (int i = 0; i < 4; i++) {
        if (input->buttons[i])
            return true;
    }
    return false;
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Kiddo");
    SetTargetFPS(60);
    audio_init();

    PlayerState players[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i] = player_init(i, player_start_pos(i),
                                 PLAYER_SHAPES[i], PLAYER_COLORS[i]);
        players[i].active = false;
    }

    ParticlePool particles;
    particles_init(&particles);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (IsGamepadAvailable(i)) {
                if (!players[i].active) {
                    players[i] = player_init(i, player_start_pos(i),
                                             PLAYER_SHAPES[i], PLAYER_COLORS[i]);
                }
                InputState input = input_read(i);
                players[i] = player_update(players[i], input, dt);

                if (any_button_pressed(&input)) {
                    particles_spawn(&particles, players[i].position,
                                    players[i].color, 15);
                    audio_play(SOUND_BUTTON);
                }
            } else {
                players[i].active = false;
            }
        }

        CollisionEvent collisions[MAX_COLLISIONS];
        int collision_count = collision_detect(players, MAX_PLAYERS,
                                               collisions, MAX_COLLISIONS);
        for (int i = 0; i < collision_count; i++) {
            particles_spawn(&particles, collisions[i].contact, MAGENTA, 20);
            audio_play(SOUND_COLLISION);
        }

        particles_update(&particles, dt);

        BeginDrawing();
        render_background();

        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (players[i].active)
                render_shape(players[i].shape, players[i].position,
                             players[i].rotation, players[i].scale,
                             players[i].color);
        }

        render_particles(particles.items, particles.count);
        EndDrawing();
    }

    particles_free(&particles);
    audio_shutdown();
    CloseWindow();
    return 0;
}
