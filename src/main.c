#include "raylib.h"
#include "audio.h"
#include "collision.h"
#include "input.h"
#include "particle.h"
#include "player.h"
#include "render.h"

#include "zlog.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

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
typedef enum { MODE_FREE_PLAY, MODE_PARK } GameMode;

#define MENU_ITEM_COUNT 2
static const char *MENU_ITEMS[MENU_ITEM_COUNT] = {
    "FRI",
    "BIL",
};
static const GameMode MENU_MODES[MENU_ITEM_COUNT] = {
    MODE_FREE_PLAY,
    MODE_PARK,
};

#define PARKING_LOT_W 200.0f
#define PARKING_LOT_H 140.0f
#define PARKING_LOT_MARGIN 120.0f
#define CAR_SIZE 100.0f
#define CAR_HALF_W 22.0f
#define CAR_HALF_H 43.0f
#define MAX_WALLS 5
#define WALL_LONG_MIN 200.0f
#define WALL_LONG_MAX 400.0f
#define WALL_SHORT_MIN 20.0f
#define WALL_SHORT_MAX 40.0f
#define WALL_MARGIN 80.0f
#define ARENA_PAD 20.0f
#define ARENA_RADIUS 80.0f

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

static bool start_pressed_solo(int gamepad_id)
{
    if (!IsGamepadAvailable(gamepad_id))
        return false;
    return IsGamepadButtonPressed(gamepad_id, GAMEPAD_BUTTON_MIDDLE_RIGHT)
        && !IsGamepadButtonDown(gamepad_id, GAMEPAD_BUTTON_MIDDLE_LEFT);
}

static Rectangle randomize_parking_lot(void)
{
    float x = PARKING_LOT_MARGIN +
              (float)(rand() % (int)(screen_width - 2 * PARKING_LOT_MARGIN - PARKING_LOT_W));
    float y = PARKING_LOT_MARGIN +
              (float)(rand() % (int)(screen_height - 2 * PARKING_LOT_MARGIN - PARKING_LOT_H));
    return (Rectangle){x, y, PARKING_LOT_W, PARKING_LOT_H};
}

static float randf(float min, float max)
{
    return min + (float)rand() / (float)RAND_MAX * (max - min);
}

static void randomize_walls(Rectangle *walls, int count, Rectangle parking_lot)
{
    for (int i = 0; i < count; i++) {
        for (int attempts = 0; attempts < 50; attempts++) {
            float long_side = randf(WALL_LONG_MIN, WALL_LONG_MAX);
            float short_side = randf(WALL_SHORT_MIN, WALL_SHORT_MAX);
            bool horizontal = rand() % 2;
            float w = horizontal ? long_side : short_side;
            float h = horizontal ? short_side : long_side;
            float x = randf(WALL_MARGIN, screen_width - WALL_MARGIN - w);
            float y = randf(WALL_MARGIN, screen_height - WALL_MARGIN - h);
            Rectangle wall = {x, y, w, h};
            if (!CheckCollisionRecs(wall, parking_lot)) {
                walls[i] = wall;
                break;
            }
        }
    }
}

static void project_corners(const Vector2 *corners, int count,
                            Vector2 axis, float *mn, float *mx)
{
    *mn = *mx = corners[0].x * axis.x + corners[0].y * axis.y;
    for (int i = 1; i < count; i++) {
        float p = corners[i].x * axis.x + corners[i].y * axis.y;
        if (p < *mn) *mn = p;
        if (p > *mx) *mx = p;
    }
}

static void obb_corners(Vector2 center, float angle_deg,
                        float half_w, float half_h, Vector2 *out)
{
    float rad = angle_deg * DEG2RAD;
    float c = cosf(rad), s = sinf(rad);
    Vector2 ax = {c, s};
    Vector2 ay = {-s, c};
    out[0] = (Vector2){center.x - half_w*ax.x - half_h*ay.x,
                       center.y - half_w*ax.y - half_h*ay.y};
    out[1] = (Vector2){center.x + half_w*ax.x - half_h*ay.x,
                       center.y + half_w*ax.y - half_h*ay.y};
    out[2] = (Vector2){center.x + half_w*ax.x + half_h*ay.x,
                       center.y + half_w*ax.y + half_h*ay.y};
    out[3] = (Vector2){center.x - half_w*ax.x + half_h*ay.x,
                       center.y - half_w*ax.y + half_h*ay.y};
}

static void car_obb_corners(Vector2 center, float angle_deg, Vector2 *out)
{
    obb_corners(center, angle_deg, CAR_HALF_W, CAR_HALF_H, out);
}

static void resolve_wall_collision_ex(PlayerState *player, float angle_deg,
                                      float half_w, float half_h,
                                      Rectangle wall)
{
    float rad = angle_deg * DEG2RAD;
    float c = cosf(rad), s = sinf(rad);
    Vector2 car_ax = {c, s};
    Vector2 car_ay = {-s, c};

    Vector2 car_corners[4];
    obb_corners(player->position, angle_deg, half_w, half_h, car_corners);

    Vector2 wall_corners[4] = {
        {wall.x, wall.y},
        {wall.x + wall.width, wall.y},
        {wall.x + wall.width, wall.y + wall.height},
        {wall.x, wall.y + wall.height},
    };

    /* SAT: 2 car axes + 2 wall axes (x, y) */
    Vector2 axes[4] = {car_ax, car_ay, {1, 0}, {0, 1}};

    float min_overlap = 1e9f;
    Vector2 push_axis = {0};

    for (int a = 0; a < 4; a++) {
        float c_mn, c_mx, w_mn, w_mx;
        project_corners(car_corners, 4, axes[a], &c_mn, &c_mx);
        project_corners(wall_corners, 4, axes[a], &w_mn, &w_mx);

        float overlap = fminf(c_mx - w_mn, w_mx - c_mn);
        if (overlap <= 0) return; /* separated */

        if (overlap < min_overlap) {
            min_overlap = overlap;
            float car_mid = (c_mn + c_mx) * 0.5f;
            float wall_mid = (w_mn + w_mx) * 0.5f;
            float sign = (car_mid > wall_mid) ? 1.0f : -1.0f;
            push_axis = (Vector2){axes[a].x * sign, axes[a].y * sign};
        }
    }

    player->position.x += push_axis.x * min_overlap;
    player->position.y += push_axis.y * min_overlap;
}

static void resolve_arena_collision(PlayerState *player, float angle_deg,
                                    float half_w, float half_h)
{
    float thick = 200.0f;
    float r = ARENA_RADIUS;
    float w = (float)screen_width;
    float h = (float)screen_height;

    /* 4 edge walls just outside the arena */
    Rectangle edge_walls[4] = {
        {ARENA_PAD - thick, ARENA_PAD,         thick, h - 2 * ARENA_PAD}, /* left */
        {w - ARENA_PAD,     ARENA_PAD,         thick, h - 2 * ARENA_PAD}, /* right */
        {ARENA_PAD,         ARENA_PAD - thick,  w - 2 * ARENA_PAD, thick}, /* top */
        {ARENA_PAD,         h - ARENA_PAD,      w - 2 * ARENA_PAD, thick}, /* bottom */
    };
    for (int i = 0; i < 4; i++)
        resolve_wall_collision_ex(player, angle_deg, half_w, half_h,
                                  edge_walls[i]);

    /* Corner arcs — push OBB corners inside the arc */
    Vector2 corner_centers[4] = {
        {ARENA_PAD + r,     ARENA_PAD + r},
        {w - ARENA_PAD - r, ARENA_PAD + r},
        {ARENA_PAD + r,     h - ARENA_PAD - r},
        {w - ARENA_PAD - r, h - ARENA_PAD - r},
    };
    for (int c = 0; c < 4; c++) {
        Vector2 ob[4];
        obb_corners(player->position, angle_deg, half_w, half_h, ob);
        for (int j = 0; j < 4; j++) {
            float dx = ob[j].x - corner_centers[c].x;
            float dy = ob[j].y - corner_centers[c].y;
            bool in_corner = false;
            if (c == 0) in_corner = dx < 0 && dy < 0;
            if (c == 1) in_corner = dx > 0 && dy < 0;
            if (c == 2) in_corner = dx < 0 && dy > 0;
            if (c == 3) in_corner = dx > 0 && dy > 0;
            if (!in_corner) continue;

            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > r && dist > 0.001f) {
                float push = dist - r;
                float nx = dx / dist;
                float ny = dy / dist;
                player->position.x -= nx * push;
                player->position.y -= ny * push;
            }
        }
    }
}

static float angle_from_stick(Vector2 stick, float current_angle)
{
    if (fabsf(stick.x) < 0.2f && fabsf(stick.y) < 0.2f)
        return current_angle;
    return atan2f(stick.y, stick.x) * RAD2DEG + 90.0f;
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

    Font font = LoadFontEx("assets/Fredoka.ttf", 120, NULL, 0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

    Texture2D car_tex = LoadTexture("assets/taxi.png");
    SetTextureFilter(car_tex, TEXTURE_FILTER_BILINEAR);

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
    GameMode game_mode = MODE_FREE_PLAY;
    int menu_selection = 0;
    bool prev_stick_up = false;
    bool prev_stick_down = false;

    srand((unsigned)time(NULL));
    Rectangle parking_lot = randomize_parking_lot();
    Rectangle walls[MAX_WALLS] = {0};
    randomize_walls(walls, MAX_WALLS, parking_lot);
    float car_angles[MAX_PLAYERS] = {0};

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

        /* Toggle debug overlay with Select or F3 */
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (IsGamepadAvailable(i) &&
                IsGamepadButtonPressed(i, GAMEPAD_BUTTON_MIDDLE_LEFT))
                debug_visible = !debug_visible;
        }
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
                game_mode = MENU_MODES[menu_selection];
                if (game_mode == MODE_PARK) {
                    parking_lot = randomize_parking_lot();
                    randomize_walls(walls, MAX_WALLS, parking_lot);
                }
                scene = SCENE_PLAYING;
            }

            /* --- Menu rendering --- */
            BeginDrawing();
            render_background();

            const char *title = "Kiddo";
            float title_size = 120.0f;
            Vector2 title_sz = MeasureTextEx(font, title, title_size, 1);
            DrawTextEx(font, title,
                       (Vector2){(screen_width - title_sz.x) / 2,
                                 screen_height / 6.0f},
                       title_size, 1, DARKGRAY);

            float item_size = 60.0f;
            int item_spacing = 80;
            int menu_height = MENU_ITEM_COUNT * item_spacing;
            int menu_start_y = (screen_height - menu_height) / 2;

            for (int i = 0; i < MENU_ITEM_COUNT; i++) {
                Color color = (i == menu_selection) ? ORANGE : DARKGRAY;
                Vector2 sz = MeasureTextEx(font, MENU_ITEMS[i], item_size, 1);
                DrawTextEx(font, MENU_ITEMS[i],
                           (Vector2){(screen_width - sz.x) / 2,
                                     menu_start_y + i * item_spacing},
                           item_size, 1, color);
            }

            EndDrawing();
        } else {
            /* --- Game scene --- */

            /* Return to menu on Start (solo, without Select) */
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (start_pressed_solo(i)) {
                    dzlog_info("returning to menu via Start (gp%d)", i);
                    log_ring_push("returning to menu");
                    scene = SCENE_MENU;
                }
            }
            if (IsKeyPressed(KEY_ESCAPE))
                scene = SCENE_MENU;

            if (scene == SCENE_MENU)
                continue;

            /* --- Input & update (shared across modes) --- */
            InputState inputs[MAX_PLAYERS] = {0};
            for (int i = 0; i < MAX_PLAYERS; i++) {
                bool gamepad_connected = IsGamepadAvailable(i);

                if (i == 0) {
                    InputState kb = input_read_keyboard();
                    inputs[0] = gamepad_connected
                        ? merge_input(input_read(0), kb)
                        : kb;
                    players[0] = player_update(players[0], inputs[0], dt);
                } else if (gamepad_connected) {
                    if (!players[i].active) {
                        players[i] = player_init(i, player_start_pos(i),
                                                 PLAYER_SHAPES[i], PLAYER_COLORS[i]);
                    }
                    inputs[i] = input_read(i);
                    players[i] = player_update(players[i], inputs[i], dt);
                } else {
                    players[i].active = false;
                }
            }

            /* Arena boundary collision (all modes) */
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (!players[i].active) continue;
                if (game_mode == MODE_PARK) {
                    resolve_arena_collision(&players[i], car_angles[i],
                                            CAR_HALF_W, CAR_HALF_H);
                } else {
                    float sz = SHAPE_BASE_SIZE * players[i].scale;
                    resolve_arena_collision(&players[i],
                                            players[i].rotation * RAD2DEG,
                                            sz, sz);
                }
            }

            if (game_mode == MODE_FREE_PLAY) {
                /* --- Free Play mode --- */
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (players[i].active && any_button_pressed(&inputs[i])) {
                        particles_spawn(&particles, players[i].position,
                                        players[i].color, 15);
                        audio_play(SOUND_BUTTON);
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
            }

            if (game_mode == MODE_PARK) {
                /* --- Park mode --- */
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (!players[i].active) continue;
                    car_angles[i] = angle_from_stick(inputs[i].left_stick,
                                                     car_angles[i]);

                    /* Wall collision */
                    for (int w = 0; w < MAX_WALLS; w++)
                        resolve_wall_collision_ex(&players[i], car_angles[i],
                                                   CAR_HALF_W, CAR_HALF_H,
                                                   walls[w]);

                    /* Check parking — car OBB overlaps parking lot */
                    Rectangle car_aabb = {
                        players[i].position.x - CAR_HALF_W,
                        players[i].position.y - CAR_HALF_H,
                        CAR_HALF_W * 2, CAR_HALF_H * 2
                    };
                    if (CheckCollisionRecs(car_aabb, parking_lot)) {
                        Vector2 lot_center = {
                            parking_lot.x + parking_lot.width / 2,
                            parking_lot.y + parking_lot.height / 2
                        };
                        particles_spawn(&particles, lot_center,
                                        players[i].color, 30);
                        audio_play(SOUND_COLLISION);
                        parking_lot = randomize_parking_lot();
                        randomize_walls(walls, MAX_WALLS, parking_lot);
                    }
                }
            }

            particles_update(&particles, dt);

            BeginDrawing();
            render_background();

            if (game_mode == MODE_FREE_PLAY) {
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (players[i].active)
                        render_shape(players[i].shape, players[i].position,
                                     players[i].rotation, players[i].scale,
                                     players[i].color);
                }
            }

            if (game_mode == MODE_PARK) {
                /* Draw walls */
                for (int i = 0; i < MAX_WALLS; i++)
                    DrawRectangleRec(walls[i], DARKGRAY);

                /* Draw parking lot */
                DrawRectangleRec(parking_lot, GRAY);
                Vector2 p_center = {
                    parking_lot.x + parking_lot.width / 2,
                    parking_lot.y + parking_lot.height / 2
                };
                Vector2 p_sz = MeasureTextEx(font, "P", 80.0f, 1);
                DrawTextEx(font, "P",
                           (Vector2){p_center.x - p_sz.x / 2,
                                     p_center.y - p_sz.y / 2},
                           80.0f, 1, WHITE);

                /* Draw cars */
                float car_scale = CAR_SIZE / (float)car_tex.width;
                Rectangle src = {0, 0, (float)car_tex.width,
                                 (float)car_tex.height};
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (!players[i].active) continue;
                    float dw = (float)car_tex.width * car_scale;
                    float dh = (float)car_tex.height * car_scale;
                    Rectangle dst = {players[i].position.x,
                                     players[i].position.y, dw, dh};
                    Vector2 origin = {dw / 2, dh / 2};
                    DrawTexturePro(car_tex, src, dst, origin,
                                   car_angles[i], players[i].color);
                }
            }

            render_particles(particles.items, particles.count);

            /* Debug overlay (toggle with F3) */
            if (debug_visible) {
                /* Arena boundary */
                Rectangle arena_rect = {ARENA_PAD, ARENA_PAD,
                    screen_width - 2 * ARENA_PAD,
                    screen_height - 2 * ARENA_PAD};
                DrawRectangleRoundedLinesEx(arena_rect, 0.05f, 16, 2,
                                            (Color){255, 0, 0, 100});

                if (game_mode == MODE_PARK) {
                    /* Walls */
                    for (int i = 0; i < MAX_WALLS; i++)
                        DrawRectangleLinesEx(walls[i], 2,
                                             (Color){255, 0, 0, 100});
                    /* Parking lot */
                    DrawRectangleLinesEx(parking_lot, 2,
                                         (Color){0, 200, 0, 100});
                }

                /* Player bounding boxes */
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (!players[i].active) continue;
                    Color bc = {players[i].color.r, players[i].color.g,
                                players[i].color.b, 80};
                    Vector2 corners[4];
                    if (game_mode == MODE_PARK) {
                        car_obb_corners(players[i].position,
                                        car_angles[i], corners);
                    } else {
                        float sz = SHAPE_BASE_SIZE * players[i].scale;
                        obb_corners(players[i].position,
                                    players[i].rotation * RAD2DEG,
                                    sz, sz, corners);
                    }
                    for (int j = 0; j < 4; j++) {
                        int k = (j + 1) % 4;
                        DrawLineEx(corners[j], corners[k], 2, bc);
                    }
                }

                /* Compute left panel height */
                float dbg_size = 22.0f;
                float gp_size = 26.0f;
                float left_h = 0;
                float left_w = 0;
                const char *shape_names[] = {"circle", "square", "tri", "star"};
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (!players[i].active) continue;
                    const char *txt = TextFormat(
                            "p%d shape=%s scale=%.2f color=(%d,%d,%d,%d) pos=(%.0f,%.0f)",
                            i,
                            (players[i].shape < SHAPE_COUNT) ? shape_names[players[i].shape] : "???",
                            players[i].scale,
                            players[i].color.r, players[i].color.g,
                            players[i].color.b, players[i].color.a,
                            players[i].position.x, players[i].position.y);
                    Vector2 sz = MeasureTextEx(font, txt, dbg_size, 1);
                    if (sz.x > left_w) left_w = sz.x;
                    left_h += 26;
                }
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (!IsGamepadAvailable(i)) continue;
                    left_h += 62;
                    /* Approximate width from gp_size lines */
                    Vector2 sz = MeasureTextEx(font, "gp0 stick=-1.00,-1.00 rstick=-1.00,-1.00", gp_size, 1);
                    if (sz.x > left_w) left_w = sz.x;
                }

                /* Compute log ring width */
                float log_size = 20.0f;
                float log_w = 0;
                float log_h = log_ring_count * 24.0f;
                int start = (log_ring_count < LOG_RING_SIZE)
                    ? 0 : log_ring_head;
                for (int i = 0; i < log_ring_count; i++) {
                    int idx = (start + i) % LOG_RING_SIZE;
                    Vector2 sz = MeasureTextEx(font, log_ring[idx], log_size, 1);
                    if (sz.x > log_w) log_w = sz.x;
                }

                /* Draw backdrops */
                int pad = 6;
                Color bg = {255, 255, 255, 200};
                if (left_h > 0)
                    DrawRectangle(10 - pad, 10 - pad,
                                  (int)left_w + 2 * pad,
                                  (int)left_h + 2 * pad, bg);
                if (log_h > 0)
                    DrawRectangle(screen_width - (int)log_w - 10 - pad,
                                  10 - pad,
                                  (int)log_w + 2 * pad,
                                  (int)log_h + 2 * pad, bg);

                /* Draw left panel text */
                float y = 10;
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (!players[i].active) continue;
                    const char *txt = TextFormat(
                            "p%d shape=%s scale=%.2f color=(%d,%d,%d,%d) pos=(%.0f,%.0f)",
                            i,
                            (players[i].shape < SHAPE_COUNT) ? shape_names[players[i].shape] : "???",
                            players[i].scale,
                            players[i].color.r, players[i].color.g,
                            players[i].color.b, players[i].color.a,
                            players[i].position.x, players[i].position.y);
                    DrawTextEx(font, txt, (Vector2){10, y}, dbg_size, 1, BLACK);
                    y += 26;
                }
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (!IsGamepadAvailable(i)) continue;
                    char btns[19] = {0};
                    for (int b = 0; b < 18; b++)
                        btns[b] = IsGamepadButtonDown(i, b) ? '1' : '0';
                    const char *line1 = TextFormat(
                            "gp%d stick=%.2f,%.2f rstick=%.2f,%.2f",
                            i,
                            GetGamepadAxisMovement(i, 0),
                            GetGamepadAxisMovement(i, 1),
                            GetGamepadAxisMovement(i, 2),
                            GetGamepadAxisMovement(i, 3));
                    DrawTextEx(font, line1, (Vector2){10, y}, gp_size, 1, BLACK);
                    y += 31;
                    const char *line2 = TextFormat(
                            "     axes4,5=%.2f,%.2f btns=%s",
                            GetGamepadAxisMovement(i, 4),
                            GetGamepadAxisMovement(i, 5),
                            btns);
                    DrawTextEx(font, line2, (Vector2){10, y}, gp_size, 1, BLACK);
                    y += 31;
                }

                /* Draw log ring — top-right corner, oldest first */
                float log_y = 10;
                for (int i = 0; i < log_ring_count; i++) {
                    int idx = (start + i) % LOG_RING_SIZE;
                    Vector2 sz = MeasureTextEx(font, log_ring[idx], log_size, 1);
                    DrawTextEx(font, log_ring[idx],
                               (Vector2){screen_width - sz.x - 10, log_y},
                               log_size, 1, BLACK);
                    log_y += 24;
                }
            }

            EndDrawing();
        }
    }

quit:
    dzlog_info("exiting game loop (frame=%d t=%.1fs)", frame, elapsed);
    particles_free(&particles);
    UnloadTexture(car_tex);
    UnloadFont(font);
    audio_shutdown();
    CloseWindow();
    zlog_fini();
    return 0;
}
