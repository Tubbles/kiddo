#include "raylib.h"
#include "audio.h"
#include "collision.h"
#include "input.h"
#include "particle.h"
#include "player.h"
#include "render.h"

#include "zlog.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define LOG_RING_SIZE 12
#define LOG_LINE_LEN 120

static char log_ring[LOG_RING_SIZE][LOG_LINE_LEN];
static int log_ring_head = 0;
static int log_ring_count = 0;

static void log_ring_push(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(log_ring[log_ring_head], LOG_LINE_LEN, fmt, ap);
    va_end(ap);
    log_ring_head = (log_ring_head + 1) % LOG_RING_SIZE;
    if (log_ring_count < LOG_RING_SIZE) log_ring_count++;
}

int screen_width = SCREEN_WIDTH_DEFAULT;
int screen_height = SCREEN_HEIGHT_DEFAULT;

typedef enum { SCENE_MENU, SCENE_PLAYING } GameScene;

#define MENU_ITEM_COUNT 3
static const char *MENU_ITEMS[MENU_ITEM_COUNT] = {
    "Free Play",
    "Chase",
    "Color Match",
};

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

static void reset_game_state(PlayerState *players, ParticlePool *particles,
                              bool prev_colliding[MAX_PLAYERS][MAX_PLAYERS])
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i] = player_init(i, player_start_pos(i),
                                 PLAYER_SHAPES[i], PLAYER_COLORS[i]);
        players[i].active = (i == 0);
    }
    particles_init(particles);
    memset(prev_colliding, 0, sizeof(bool) * MAX_PLAYERS * MAX_PLAYERS);
}

static bool select_pressed_solo(int gamepad_id)
{
    if (!IsGamepadAvailable(gamepad_id))
        return false;
    return IsGamepadButtonPressed(gamepad_id, GAMEPAD_BUTTON_MIDDLE_LEFT)
        && !IsGamepadButtonDown(gamepad_id, GAMEPAD_BUTTON_MIDDLE_RIGHT);
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
    bool debug_visible = false;
    bool prev_colliding[MAX_PLAYERS][MAX_PLAYERS] = {{false}};

    GameScene scene = SCENE_PLAYING;
    int menu_selection = 0;
    bool prev_stick_up = false;
    bool prev_stick_down = false;

    dzlog_info("entering game loop");
    log_ring_push("entering game loop");

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
            log_ring_push("frame=%d t=%.1fs fps=%d players=%d",
                    frame, elapsed, GetFPS(), active);
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
            log_ring_push("gamepads %d -> %d (frame=%d)",
                    prev_gamepads, gamepads, frame);
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (IsGamepadAvailable(i)) {
                    dzlog_info("gamepad %d: %s", i, GetGamepadName(i));
                    log_ring_push("gamepad %d: %s", i, GetGamepadName(i));
                }
            }
            prev_gamepads = gamepads;
        }

        /* Exit on Start+Select from any gamepad (always active) */
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (input_exit_requested(i))
                goto quit;
        }

        /* Toggle debug overlay with F3 */
        if (IsKeyPressed(KEY_F3))
            debug_visible = !debug_visible;

        if (scene == SCENE_MENU) {
            /* --- Menu input --- */

            /* D-pad navigation */
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (!IsGamepadAvailable(i)) continue;
                if (IsGamepadButtonPressed(i, GAMEPAD_BUTTON_LEFT_FACE_UP))
                    menu_selection--;
                if (IsGamepadButtonPressed(i, GAMEPAD_BUTTON_LEFT_FACE_DOWN))
                    menu_selection++;
            }

            /* Analog stick navigation (edge-triggered) */
            float stick_y = 0.0f;
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (!IsGamepadAvailable(i)) continue;
                float sy = GetGamepadAxisMovement(i, GAMEPAD_AXIS_LEFT_Y);
                if (sy < -0.5f || sy > 0.5f) stick_y = sy;
            }
            bool stick_up = stick_y < -0.5f;
            bool stick_down = stick_y > 0.5f;
            if (stick_up && !prev_stick_up) menu_selection--;
            if (stick_down && !prev_stick_down) menu_selection++;
            prev_stick_up = stick_up;
            prev_stick_down = stick_down;

            /* Keyboard navigation */
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
                menu_selection--;
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
                menu_selection++;

            /* Wrap around */
            if (menu_selection < 0)
                menu_selection = MENU_ITEM_COUNT - 1;
            if (menu_selection >= MENU_ITEM_COUNT)
                menu_selection = 0;

            /* Confirm selection */
            bool confirmed = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER);
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (IsGamepadAvailable(i) &&
                    IsGamepadButtonPressed(i, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
                    confirmed = true;
            }

            if (confirmed) {
                dzlog_info("menu: selected '%s'", MENU_ITEMS[menu_selection]);
                log_ring_push("menu: selected '%s'", MENU_ITEMS[menu_selection]);
                reset_game_state(players, &particles, prev_colliding);
                scene = SCENE_PLAYING;
            }

            /* --- Menu rendering --- */
            BeginDrawing();
            render_background();

            const char *title = "Kiddo";
            int title_size = 60;
            int title_w = MeasureText(title, title_size);
            DrawText(title, (screen_width - title_w) / 2,
                     screen_height / 5, title_size, DARKGRAY);

            int item_size = 30;
            int item_spacing = 50;
            int menu_height = MENU_ITEM_COUNT * item_spacing;
            int menu_start_y = (screen_height - menu_height) / 2;

            for (int i = 0; i < MENU_ITEM_COUNT; i++) {
                Color color = (i == menu_selection) ? ORANGE : DARKGRAY;
                int tw = MeasureText(MENU_ITEMS[i], item_size);
                DrawText(MENU_ITEMS[i], (screen_width - tw) / 2,
                         menu_start_y + i * item_spacing, item_size, color);
            }

            EndDrawing();
        } else {
            /* --- Game scene --- */

            /* Return to menu on Select (solo, without Start) */
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (select_pressed_solo(i)) {
                    dzlog_info("returning to menu via Select (gp%d)", i);
                    log_ring_push("returning to menu");
                    scene = SCENE_MENU;
                }
            }
            if (IsKeyPressed(KEY_ESCAPE))
                scene = SCENE_MENU;

            if (scene == SCENE_MENU)
                continue;

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
            bool cur_colliding[MAX_PLAYERS][MAX_PLAYERS] = {{false}};
            for (int i = 0; i < collision_count; i++) {
                int a = collisions[i].player_a;
                int b = collisions[i].player_b;
                cur_colliding[a][b] = true;
                cur_colliding[b][a] = true;
                if (!prev_colliding[a][b]) {
                    particles_spawn(&particles, collisions[i].contact, MAGENTA, 20);
                    audio_play(SOUND_COLLISION);
                }
            }
            memcpy(prev_colliding, cur_colliding, sizeof(prev_colliding));

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

            /* Debug overlay (toggle with F3) */
            if (debug_visible) {
                int y = 10;
                const char *shape_names[] = {"circle", "square", "tri", "star"};
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (!players[i].active) continue;
                    DrawText(TextFormat("p%d shape=%s scale=%.2f color=(%d,%d,%d,%d) pos=(%.0f,%.0f)",
                            i,
                            (players[i].shape < SHAPE_COUNT) ? shape_names[players[i].shape] : "???",
                            players[i].scale,
                            players[i].color.r, players[i].color.g,
                            players[i].color.b, players[i].color.a,
                            players[i].position.x, players[i].position.y),
                            10, y, 16, BLACK);
                    y += 20;
                }
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
                            10, y, 20, BLACK);
                    y += 24;
                    DrawText(TextFormat("     axes4,5=%.2f,%.2f btns=%s",
                            GetGamepadAxisMovement(i, 4),
                            GetGamepadAxisMovement(i, 5),
                            btns),
                            10, y, 20, BLACK);
                    y += 24;
                }

                /* Log ring — top-right corner, oldest first */
                int log_y = 10;
                int start = (log_ring_count < LOG_RING_SIZE)
                    ? 0
                    : log_ring_head;
                for (int i = 0; i < log_ring_count; i++) {
                    int idx = (start + i) % LOG_RING_SIZE;
                    int tw = MeasureText(log_ring[idx], 16);
                    DrawText(log_ring[idx], screen_width - tw - 10,
                            log_y, 16, BLACK);
                    log_y += 20;
                }
            }

            EndDrawing();
        }
    }

quit:
    dzlog_info("exiting game loop (frame=%d t=%.1fs)", frame, elapsed);
    particles_free(&particles);
    audio_shutdown();
    CloseWindow();
    zlog_fini();
    return 0;
}
