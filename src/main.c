#include "raylib.h"
#include "audio.h"
#include "collision.h"
#include "input.h"
#include "particle.h"
#include "player.h"
#include "render.h"

#include "zlog.h"

#include <stdio.h>

int screen_width = SCREEN_WIDTH_DEFAULT;
int screen_height = SCREEN_HEIGHT_DEFAULT;

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
    float quarter_w = screen_width / 4.0f;
    float half_h = screen_height / 2.0f;
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

static InputState merge_input(InputState a, InputState b)
{
    InputState out = a;
    if (b.left_stick.x != 0.0f) out.left_stick.x = b.left_stick.x;
    if (b.left_stick.y != 0.0f) out.left_stick.y = b.left_stick.y;
    if (b.right_stick.x != 0.0f) out.right_stick.x = b.right_stick.x;
    if (b.right_stick.y != 0.0f) out.right_stick.y = b.right_stick.y;
    for (int i = 0; i < 4; i++)
        out.buttons[i] = a.buttons[i] || b.buttons[i];
    if (b.left_trigger > a.left_trigger) out.left_trigger = b.left_trigger;
    if (b.right_trigger > a.right_trigger) out.right_trigger = b.right_trigger;
    return out;
}

int main(void)
{
    dzlog_init("assets/zlog.conf", "kiddo");

    InitWindow(SCREEN_WIDTH_DEFAULT, SCREEN_HEIGHT_DEFAULT, "Kiddo");
    HideCursor();

    int monitor = GetCurrentMonitor();
    int mon_w = GetMonitorWidth(monitor);
    int mon_h = GetMonitorHeight(monitor);
    dzlog_info("monitor=%d resolution=%dx%d", monitor, mon_w, mon_h);
    if (mon_w > 0 && mon_h > 0) {
        screen_width = mon_w;
        screen_height = mon_h;
        SetWindowSize(screen_width, screen_height);
    }
    ToggleBorderlessWindowed();
    dzlog_info("screen_width=%d screen_height=%d", screen_width, screen_height);

    input_load_mappings("assets/gamecontrollerdb.txt");
    SetTargetFPS(60);
    audio_init();

    PlayerState players[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i] = player_init(i, player_start_pos(i),
                                 PLAYER_SHAPES[i], PLAYER_COLORS[i]);
        players[i].active = false;
    }

    /* Player 0 always active (keyboard fallback) */
    players[0].active = true;

    ParticlePool particles;
    particles_init(&particles);

    int frame = 0;
    float elapsed = 0.0f;
    int prev_gamepads = -1;

    dzlog_info("entering game loop");

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        frame++;
        elapsed += dt;

        /* Heartbeat every ~5 seconds */
        if (frame % 300 == 0) {
            int active = 0;
            for (int i = 0; i < MAX_PLAYERS; i++)
                if (players[i].active) active++;
            dzlog_debug("frame=%d t=%.1fs dt=%.4f fps=%d "
                    "players=%d particles=%d",
                    frame, elapsed, dt, GetFPS(), active, particles.count);
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (IsGamepadAvailable(i)) {
                    InputState dbg = input_read(i);
                    dzlog_debug("gp%d stick=(%.2f,%.2f) "
                            "rstick=(%.2f,%.2f) lt=%.2f rt=%.2f "
                            "btn=%d%d%d%d pos=(%.0f,%.0f)", i,
                            dbg.left_stick.x, dbg.left_stick.y,
                            dbg.right_stick.x, dbg.right_stick.y,
                            dbg.left_trigger, dbg.right_trigger,
                            dbg.buttons[0], dbg.buttons[1],
                            dbg.buttons[2], dbg.buttons[3],
                            players[i].position.x, players[i].position.y);
                    char btns[19] = {0};
                    for (int b = 0; b < 18; b++)
                        btns[b] = IsGamepadButtonDown(i, b) ? '1' : '0';
                    dzlog_debug("gp%d raw_buttons=%s axes=%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
                            i, btns,
                            GetGamepadAxisMovement(i, 0),
                            GetGamepadAxisMovement(i, 1),
                            GetGamepadAxisMovement(i, 2),
                            GetGamepadAxisMovement(i, 3),
                            GetGamepadAxisMovement(i, 4),
                            GetGamepadAxisMovement(i, 5));
                }
            }
        }

        /* Log gamepad connect/disconnect */
        int gamepads = input_count_gamepads();
        if (gamepads != prev_gamepads) {
            dzlog_info("gamepads %d -> %d (frame=%d)",
                    prev_gamepads, gamepads, frame);
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (IsGamepadAvailable(i))
                    dzlog_info("gamepad %d: %s", i, GetGamepadName(i));
            }
            prev_gamepads = gamepads;
        }

        /* Exit on Start+Select from any gamepad */
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (input_exit_requested(i))
                goto quit;
        }

        for (int i = 0; i < MAX_PLAYERS; i++) {
            bool gamepad_connected = IsGamepadAvailable(i);

            if (i == 0) {
                /* Player 0: keyboard always, gamepad merged if available */
                InputState kb = input_read_keyboard();
                InputState input = gamepad_connected
                    ? merge_input(input_read(0), kb)
                    : kb;
                players[0] = player_update(players[0], input, dt);

                if (any_button_pressed(&input)) {
                    particles_spawn(&particles, players[0].position,
                                    players[0].color, 15);
                    audio_play(SOUND_BUTTON);
                }
            } else if (gamepad_connected) {
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

        /* Debug overlay */
        int y = 10;
        DrawText(TextFormat("frame=%d fps=%d", frame, GetFPS()), 10, y, 20, WHITE);
        y += 24;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (!IsGamepadAvailable(i)) continue;
            char btns[19] = {0};
            for (int b = 0; b < 18; b++)
                btns[b] = IsGamepadButtonDown(i, b) ? '1' : '0';
            DrawText(TextFormat("gp%d stick=%.2f,%.2f rstick=%.2f,%.2f",
                    i,
                    GetGamepadAxisMovement(i, 0),
                    GetGamepadAxisMovement(i, 1),
                    GetGamepadAxisMovement(i, 2),
                    GetGamepadAxisMovement(i, 3)),
                    10, y, 20, WHITE);
            y += 24;
            DrawText(TextFormat("     axes4,5=%.2f,%.2f btns=%s",
                    GetGamepadAxisMovement(i, 4),
                    GetGamepadAxisMovement(i, 5),
                    btns),
                    10, y, 20, WHITE);
            y += 24;
        }

        EndDrawing();
    }

quit:
    dzlog_info("exiting game loop (frame=%d t=%.1fs)", frame, elapsed);
    particles_free(&particles);
    audio_shutdown();
    CloseWindow();
    zlog_fini();
    return 0;
}
