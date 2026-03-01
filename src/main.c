#include "raylib.h"
#include "audio.h"
#include "collision.h"
#include "entity.h"
#include "entity_car.h"
#include "entity_parking.h"
#include "entity_shape.h"
#include "entity_wall.h"
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
#define MAX_WALLS 5
#define WALL_LONG_MIN 200.0f
#define WALL_LONG_MAX 400.0f
#define WALL_SHORT_MIN 20.0f
#define WALL_SHORT_MAX 40.0f
#define WALL_MARGIN 80.0f
#define ARENA_PAD 20.0f

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

/* Entity array helpers */
static int entity_count;
static Entity entities[MAX_ENTITIES];

static int add_entity(Entity e)
{
    if (entity_count >= MAX_ENTITIES) return -1;
    entities[entity_count] = e;
    return entity_count++;
}

static void reset_entities_free_play(void)
{
    entity_count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        Entity e = entity_shape_init(i, player_start_pos(i),
                                     PLAYER_SHAPES[i], PLAYER_COLORS[i]);
        e.active = (i == 0);
        add_entity(e);
    }
}

static void reset_entities_park(Texture2D *car_textures, int tex_count,
                                Font *font,
                                Rectangle parking_lot, Rectangle *walls)
{
    entity_count = 0;

    /* Player cars (indices 0..MAX_PLAYERS-1) */
    for (int i = 0; i < MAX_PLAYERS; i++) {
        Entity e = entity_car_init(i, player_start_pos(i),
                                   PLAYER_COLORS[i], car_textures, tex_count);
        e.active = (i == 0);
        add_entity(e);
    }

    /* Parking lot */
    add_entity(entity_parking_init(parking_lot, font));

    /* Walls */
    for (int i = 0; i < MAX_WALLS; i++)
        add_entity(entity_wall_init(walls[i]));
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

    const char *car_tex_paths[] = {
        "assets/taxi.png",
        "assets/audi.png",
        "assets/police.png",
        "assets/ambulance.png",
        "assets/mini_truck.png",
        "assets/truck.png",
        "assets/black_viper.png",
        "assets/mini_van.png",
        "assets/car.png",
    };
    #define CAR_TEX_COUNT ((int)(sizeof(car_tex_paths) / sizeof(car_tex_paths[0])))
    Texture2D car_textures[CAR_TEX_COUNT];
    for (int i = 0; i < CAR_TEX_COUNT; i++) {
        car_textures[i] = LoadTexture(car_tex_paths[i]);
        SetTextureFilter(car_textures[i], TEXTURE_FILTER_BILINEAR);
    }

    input_load_mappings("assets/gamecontrollerdb.txt");
    SetTargetFPS(60);
    audio_init();

    /* Initial entity setup (free play by default) */
    reset_entities_free_play();

    ParticlePool particles;
    particles_init(&particles);

    int frame = 0;
    float elapsed = 0.0f;
    int prev_gamepads = -1;
    bool debug_visible = false;
    bool prev_colliding[MAX_PLAYERS][MAX_PLAYERS] = {{false}};

    GameScene scene = SCENE_MENU;
    GameMode game_mode = MODE_FREE_PLAY;
    bool game_started = false;
    int menu_selection = 0;
    bool prev_stick_up = false;
    bool prev_stick_down = false;

    srand((unsigned)time(NULL));
    Rectangle parking_lot = randomize_parking_lot();
    Rectangle walls[MAX_WALLS] = {0};
    randomize_walls(walls, MAX_WALLS, parking_lot);

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
                if (i < entity_count && entities[i].active) active++;
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
                            entities[i].position.x, entities[i].position.y);
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

            /* Resume current game: Start or right-face-button (back) */
            bool resume = false;
            if (game_started) {
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (start_pressed_solo(i))
                        resume = true;
                    if (IsGamepadAvailable(i) &&
                        IsGamepadButtonPressed(i, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
                        resume = true;
                }
                if (IsKeyPressed(KEY_ESCAPE))
                    resume = true;
            }
            if (resume) {
                dzlog_info("menu: resuming game");
                log_ring_push("menu: resuming");
                scene = SCENE_PLAYING;
            }

            /* Confirm selection — start a new game */
            bool confirmed = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER);
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (IsGamepadAvailable(i) &&
                    IsGamepadButtonPressed(i, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
                    confirmed = true;
            }

            if (confirmed) {
                dzlog_info("menu: selected '%s'", MENU_ITEMS[menu_selection]);
                log_ring_push("menu: selected '%s'", MENU_ITEMS[menu_selection]);
                game_mode = MENU_MODES[menu_selection];
                memset(prev_colliding, 0, sizeof(prev_colliding));
                particles_init(&particles);
                if (game_mode == MODE_PARK) {
                    parking_lot = randomize_parking_lot();
                    randomize_walls(walls, MAX_WALLS, parking_lot);
                    reset_entities_park(car_textures, CAR_TEX_COUNT, &font, parking_lot, walls);
                } else {
                    reset_entities_free_play();
                }
                game_started = true;
                scene = SCENE_PLAYING;
            }

            /* --- Menu rendering --- */
            BeginDrawing();
            render_background();

            /* Draw paused game underneath the menu overlay */
            if (game_started) {
                for (int i = 0; i < entity_count; i++) {
                    if (!entities[i].active) continue;
                    if (entities[i].vtable->render)
                        entities[i].vtable->render(&entities[i]);
                }
                /* Dim overlay */
                DrawRectangle(0, 0, screen_width, screen_height,
                              (Color){0, 0, 0, 120});
            }

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

            /* --- Input & update --- */
            InputState inputs[MAX_PLAYERS] = {0};
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (i >= entity_count) break;
                bool gamepad_connected = IsGamepadAvailable(i);

                if (i == 0) {
                    InputState kb = input_read_keyboard();
                    inputs[0] = gamepad_connected
                        ? merge_input(input_read(0), kb)
                        : kb;
                } else if (gamepad_connected) {
                    if (!entities[i].active) {
                        /* Reactivate player entity on gamepad connect */
                        if (game_mode == MODE_FREE_PLAY) {
                            entities[i] = entity_shape_init(i, player_start_pos(i),
                                                            PLAYER_SHAPES[i], PLAYER_COLORS[i]);
                        } else {
                            entities[i] = entity_car_init(i, player_start_pos(i),
                                                          PLAYER_COLORS[i],
                                                          car_textures, CAR_TEX_COUNT);
                        }
                    }
                    inputs[i] = input_read(i);
                } else {
                    entities[i].active = false;
                }
            }

            /* Update player entities via vtable */
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (i >= entity_count || !entities[i].active) continue;
                if (entities[i].vtable->update)
                    entities[i].vtable->update(&entities[i], inputs[i], dt);
            }

            /* Arena boundary collision for player entities */
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (i >= entity_count || !entities[i].active) continue;
                CollisionShape cs;
                entities[i].vtable->get_collision_shape(&entities[i], &cs);
                float angle = entities[i].rotation;
                resolve_arena_composite(&cs, &entities[i].position, angle);
            }

            /* Mode-specific game events */
            if (game_mode == MODE_FREE_PLAY) {
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (i >= entity_count || !entities[i].active) continue;
                    if (any_button_pressed(&inputs[i])) {
                        particles_spawn(&particles, entities[i].position,
                                        entities[i].color, 15);
                        audio_play(SOUND_BUTTON);
                    }
                }

                /* Shape-shape collision detection */
                bool cur_colliding[MAX_PLAYERS][MAX_PLAYERS] = {{false}};
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (i >= entity_count || !entities[i].active) continue;
                    CollisionShape cs_a;
                    entities[i].vtable->get_collision_shape(&entities[i], &cs_a);
                    for (int j = i + 1; j < MAX_PLAYERS; j++) {
                        if (j >= entity_count || !entities[j].active) continue;
                        CollisionShape cs_b;
                        entities[j].vtable->get_collision_shape(&entities[j], &cs_b);
                        if (composite_overlap(&cs_a, entities[i].position,
                                              entities[i].rotation,
                                              &cs_b, entities[j].position,
                                              entities[j].rotation)) {
                            cur_colliding[i][j] = true;
                            cur_colliding[j][i] = true;
                            if (!prev_colliding[i][j]) {
                                Vector2 contact = {
                                    (entities[i].position.x + entities[j].position.x) * 0.5f,
                                    (entities[i].position.y + entities[j].position.y) * 0.5f,
                                };
                                particles_spawn(&particles, contact, MAGENTA, 20);
                                audio_play(SOUND_COLLISION);
                            }
                        }
                    }
                }
                memcpy(prev_colliding, cur_colliding, sizeof(prev_colliding));
            }

            if (game_mode == MODE_PARK) {
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (i >= entity_count || !entities[i].active) continue;

                    /* Wall collision: car vs wall entities */
                    CollisionShape car_cs;
                    entities[i].vtable->get_collision_shape(&entities[i], &car_cs);
                    for (int w = 0; w < entity_count; w++) {
                        if (entities[w].kind != ENTITY_WALL || !entities[w].active)
                            continue;
                        CollisionShape wall_cs;
                        entities[w].vtable->get_collision_shape(&entities[w], &wall_cs);
                        Vector2 push = resolve_composite(
                            &car_cs, entities[i].position, entities[i].rotation,
                            &wall_cs, entities[w].position, entities[w].rotation);
                        entities[i].position.x += push.x;
                        entities[i].position.y += push.y;
                    }

                    /* Check parking — car overlaps parking lot */
                    for (int p = 0; p < entity_count; p++) {
                        if (entities[p].kind != ENTITY_PARKING || !entities[p].active)
                            continue;
                        CollisionShape park_cs;
                        entities[p].vtable->get_collision_shape(&entities[p], &park_cs);
                        if (composite_overlap(
                                &car_cs, entities[i].position, entities[i].rotation,
                                &park_cs, entities[p].position, entities[p].rotation)) {
                            particles_spawn(&particles, entities[p].position,
                                            entities[i].color, 30);
                            audio_play(SOUND_COLLISION);
                            parking_lot = randomize_parking_lot();
                            randomize_walls(walls, MAX_WALLS, parking_lot);
                            /* Replace only parking lot + walls, keep players */
                            entity_count = MAX_PLAYERS;
                            add_entity(entity_parking_init(parking_lot, &font));
                            for (int wi = 0; wi < MAX_WALLS; wi++)
                                add_entity(entity_wall_init(walls[wi]));
                            goto done_park_check;
                        }
                    }
                }
                done_park_check:;
            }

            particles_update(&particles, dt);

            BeginDrawing();
            render_background();

            /* Render all active entities via vtable */
            for (int i = 0; i < entity_count; i++) {
                if (!entities[i].active) continue;
                if (entities[i].vtable->render)
                    entities[i].vtable->render(&entities[i]);
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

                /* Entity collision shapes */
                for (int i = 0; i < entity_count; i++) {
                    if (!entities[i].active) continue;
                    if (!entities[i].vtable->get_collision_shape) continue;

                    Color bc;
                    if (entities[i].kind == ENTITY_WALL) {
                        bc = (Color){255, 0, 0, 100};
                    } else if (entities[i].kind == ENTITY_PARKING) {
                        bc = (Color){0, 200, 0, 100};
                    } else {
                        bc = (Color){entities[i].color.r, entities[i].color.g,
                                     entities[i].color.b, 80};
                    }

                    CollisionShape cs;
                    entities[i].vtable->get_collision_shape(&entities[i], &cs);
                    for (int p = 0; p < cs.count; p++) {
                        Vector2 wp = prim_world_pos(entities[i].position,
                                                     entities[i].rotation,
                                                     cs.prims[p].offset);
                        if (cs.prims[p].kind == COLLIDER_RECT) {
                            float pa = entities[i].rotation + cs.prims[p].angle_offset;
                            Vector2 corners[4];
                            obb_corners(wp, pa,
                                        cs.prims[p].rect.half_w,
                                        cs.prims[p].rect.half_h, corners);
                            for (int j = 0; j < 4; j++) {
                                int k = (j + 1) % 4;
                                DrawLineEx(corners[j], corners[k], 2, bc);
                            }
                        } else if (cs.prims[p].kind == COLLIDER_CIRCLE) {
                            DrawCircleLinesV(wp, cs.prims[p].circle.radius, bc);
                        } else if (cs.prims[p].kind == COLLIDER_TRIANGLE) {
                            Vector2 tv[3];
                            for (int v = 0; v < 3; v++) {
                                tv[v] = (Vector2){
                                    wp.x + cs.prims[p].triangle.verts[v].x,
                                    wp.y + cs.prims[p].triangle.verts[v].y,
                                };
                            }
                            for (int j = 0; j < 3; j++) {
                                int k = (j + 1) % 3;
                                DrawLineEx(tv[j], tv[k], 2, bc);
                            }
                        }
                    }
                }

                /* Compute left panel height */
                float dbg_size = 22.0f;
                float gp_size = 26.0f;
                float left_h = 0;
                float left_w = 0;
                const char *shape_names[] = {"circle", "square", "tri", "star"};
                for (int i = 0; i < MAX_PLAYERS && i < entity_count; i++) {
                    if (!entities[i].active) continue;
                    const char *kind_str = (entities[i].kind == ENTITY_SHAPE) ? "shape" : "car";
                    const char *txt;
                    if (entities[i].kind == ENTITY_SHAPE) {
                        txt = TextFormat(
                                "p%d type=%s shape=%s scale=%.2f color=(%d,%d,%d,%d) pos=(%.0f,%.0f)",
                                i, kind_str,
                                (entities[i].shape.shape < SHAPE_COUNT) ? shape_names[entities[i].shape.shape] : "???",
                                entities[i].scale,
                                entities[i].color.r, entities[i].color.g,
                                entities[i].color.b, entities[i].color.a,
                                entities[i].position.x, entities[i].position.y);
                    } else {
                        txt = TextFormat(
                                "p%d type=%s angle=%.1f scale=%.2f color=(%d,%d,%d,%d) pos=(%.0f,%.0f)",
                                i, kind_str,
                                entities[i].car.facing_angle,
                                entities[i].scale,
                                entities[i].color.r, entities[i].color.g,
                                entities[i].color.b, entities[i].color.a,
                                entities[i].position.x, entities[i].position.y);
                    }
                    Vector2 sz = MeasureTextEx(font, txt, dbg_size, 1);
                    if (sz.x > left_w) left_w = sz.x;
                    left_h += 26;
                }
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (!IsGamepadAvailable(i)) continue;
                    left_h += 62;
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
                for (int i = 0; i < MAX_PLAYERS && i < entity_count; i++) {
                    if (!entities[i].active) continue;
                    const char *kind_str = (entities[i].kind == ENTITY_SHAPE) ? "shape" : "car";
                    const char *txt;
                    if (entities[i].kind == ENTITY_SHAPE) {
                        txt = TextFormat(
                                "p%d type=%s shape=%s scale=%.2f color=(%d,%d,%d,%d) pos=(%.0f,%.0f)",
                                i, kind_str,
                                (entities[i].shape.shape < SHAPE_COUNT) ? shape_names[entities[i].shape.shape] : "???",
                                entities[i].scale,
                                entities[i].color.r, entities[i].color.g,
                                entities[i].color.b, entities[i].color.a,
                                entities[i].position.x, entities[i].position.y);
                    } else {
                        txt = TextFormat(
                                "p%d type=%s angle=%.1f scale=%.2f color=(%d,%d,%d,%d) pos=(%.0f,%.0f)",
                                i, kind_str,
                                entities[i].car.facing_angle,
                                entities[i].scale,
                                entities[i].color.r, entities[i].color.g,
                                entities[i].color.b, entities[i].color.a,
                                entities[i].position.x, entities[i].position.y);
                    }
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
    for (int i = 0; i < CAR_TEX_COUNT; i++)
        UnloadTexture(car_textures[i]);
    UnloadFont(font);
    audio_shutdown();
    CloseWindow();
    zlog_fini();
    return 0;
}
