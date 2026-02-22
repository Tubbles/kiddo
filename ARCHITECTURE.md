# Kiddo - Toddler Gamepad Game

A minimal 2D shapes game for children ages 2-4, focused on hand-eye coordination and learning to use a gamepad.

## Design Principles

- **No failure state.** Every input produces a visible, satisfying result.
- **Pure visual feedback.** Shapes, colors, movement, particles — no text, no menus, no story.
- **Multiplayer from the start.** Each connected gamepad controls one shape. Plug in and play.
- **Data-oriented.** Game state is plain structs. Logic is pure functions over that data.
- **Small, testable functions.** Each function does one thing. Side effects are isolated at the edges.

## Technology

| Concern           | Choice                          |
|-------------------|---------------------------------|
| Language          | C (C11)                         |
| Graphics / Input  | raylib                          |
| Build system      | CMake (driven by Conan)         |
| Package manager   | Conan 2                         |
| Unit test framework | Unity (ThrowTheSwitch)        |
| Mocking           | fff.h (Fake Function Framework) |

Conan is the primary build orchestrator. CMake is generated/invoked by Conan. All external dependencies (raylib, Unity, fff) are declared as Conan packages.

## Game Concept

Each player controls a colored shape (circle, square, triangle, star) with a gamepad. Shapes float freely on a 2D screen. Joysticks move the shape. Buttons trigger effects:

- **Left stick:** Move shape
- **Right stick:** Rotate shape (for non-circles) / scale shape
- **Face buttons (A/B/X/Y):** Change color / spawn particles / change shape / play a sound
- **Triggers/bumpers:** Grow / shrink

When shapes overlap or collide they produce particle bursts and color blending — purely visual, no score, no win/loss.

## Architecture Overview

```
main.c
  |
  v
game_loop (tick + render)
  |
  +-- input.c      read gamepads -> InputState[]
  +-- player.c     apply InputState to PlayerState
  +-- shape.c      geometry helpers (bounds, overlap)
  +-- particle.c   spawn / update / expire particles
  +-- collision.c  detect overlaps, produce events
  +-- render.c     draw PlayerState[], particles, background
  +-- audio.c      play sounds in response to events
```

Data flows **downward**: raw input -> processed state -> rendered output. No module reaches back up.

## Data Model

```c
// Max supported simultaneous players
#define MAX_PLAYERS 4

typedef enum {
    SHAPE_CIRCLE,
    SHAPE_SQUARE,
    SHAPE_TRIANGLE,
    SHAPE_STAR,
    SHAPE_COUNT
} ShapeKind;

typedef struct {
    Vector2 left_stick;   // -1..1 per axis
    Vector2 right_stick;  // -1..1 per axis
    bool buttons[4];      // A, B, X, Y (pressed this frame)
    float left_trigger;   // 0..1
    float right_trigger;  // 0..1
} InputState;

typedef struct {
    bool active;
    ShapeKind shape;
    Vector2 position;     // screen coordinates
    float rotation;       // radians
    float scale;          // 0.5 .. 3.0
    Color color;
    int gamepad_id;
} PlayerState;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float lifetime;       // seconds remaining
    float size;
} Particle;

typedef struct {
    PlayerState players[MAX_PLAYERS];
    Particle *particles;  // dynamic array
    int particle_count;
    int particle_capacity;
} GameState;
```

## Module Responsibilities

### `main.c`
- Initialize window, audio device
- Run game loop: poll input, update state, render, handle quit
- Owns the single `GameState` instance
- The only file that calls raylib lifecycle functions (InitWindow, CloseWindow, etc.)

### `input.h / input.c`
- `InputState input_read(int gamepad_id)` — read one gamepad into an `InputState`
- `int input_count_gamepads(void)` — return number of connected gamepads
- Thin wrapper over raylib gamepad functions; the only module that calls them

### `player.h / player.c`
- `PlayerState player_init(int gamepad_id, Vector2 start_pos)` — create a new player
- `PlayerState player_update(PlayerState state, InputState input, float dt)` — pure function: apply input to produce new state
- Handles movement, rotation, scaling, shape/color changes
- Clamps position to screen bounds, scale to valid range

### `shape.h / shape.c`
- `Rectangle shape_bounds(ShapeKind kind, Vector2 pos, float scale)` — axis-aligned bounding box
- `bool shape_overlap(ShapeKind a, Vector2 pos_a, float scale_a, ShapeKind b, Vector2 pos_b, float scale_b)` — overlap test
- Pure geometry, no side effects

### `particle.h / particle.c`
- `void particles_spawn(GameState *state, Vector2 pos, Color color, int count)` — add particles
- `void particles_update(GameState *state, float dt)` — move and expire particles
- Manages the dynamic particle array (grow capacity as needed)

### `collision.h / collision.c`
- `void collision_detect(const GameState *state, CollisionEvent *events, int *count)` — pure detection
- `CollisionEvent` contains indices of the two players and the contact point
- Called each frame; results drive particle spawns and audio

### `render.h / render.c`
- `void render_background(void)` — soft color or gentle gradient
- `void render_players(const PlayerState *players, int count)` — draw each active player shape
- `void render_particles(const Particle *particles, int count)` — draw particles
- Only module (besides main) that calls raylib drawing functions

### `audio.h / audio.c`
- `void audio_init(void)` — load built-in sounds (generated procedurally or from embedded data)
- `void audio_play(SoundKind kind)` — play a sound effect
- `void audio_shutdown(void)` — cleanup
- Sounds are simple tones/blips, no asset files needed

## File Layout

```
kiddo/
├── CMakeLists.txt
├── conanfile.py
├── src/
│   ├── main.c
│   ├── input.h / input.c
│   ├── player.h / player.c
│   ├── shape.h / shape.c
│   ├── particle.h / particle.c
│   ├── collision.h / collision.c
│   ├── render.h / render.c
│   └── audio.h / audio.c
└── test/
    ├── CMakeLists.txt
    ├── test_player.c
    ├── test_shape.c
    ├── test_particle.c
    └── test_collision.c
```

## Build & Run

```bash
# Install dependencies and build
conan install . --output-folder=build --build=missing
conan build .

# Run
./build/Release/kiddo

# Run tests
./build/Release/test/kiddo_tests
```

## Testing Strategy

**What is tested (pure logic):**
- `player_update` — given an InputState and dt, assert resulting position/rotation/scale/color
- `shape_bounds` / `shape_overlap` — geometric correctness
- `collision_detect` — given known positions, assert correct events
- `particles_update` — lifetime expiry, position advancement

**What is NOT unit tested (side-effect boundaries):**
- `input.c` (wraps raylib hardware calls — faked with fff in integration tests if needed)
- `render.c` (draws to screen)
- `audio.c` (plays sounds)
- `main.c` (orchestration)

fff.h is used to fake raylib functions when a module under test calls them indirectly.

## Dependency Diagram

```
main
 ├── input    (raylib)
 ├── player   (pure)
 ├── shape    (pure)
 ├── particle (pure + allocation)
 ├── collision (pure)
 ├── render   (raylib)
 └── audio    (raylib)
```

Modules do not depend on each other. `main.c` is the sole integrator.

## Future Ideas (Out of Scope for Now)

- Keyboard / mouse as alternative input
- Shape trails and visual effects
- Background music
- Screen wrap vs bounce toggle
- Dynamic split-screen per player
