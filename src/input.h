#ifndef INPUT_H
#define INPUT_H

#include "raylib.h"
#include <stdbool.h>

typedef struct {
    Vector2 left_stick;
    Vector2 right_stick;
    bool buttons[4];
    float left_trigger;
    float right_trigger;
} InputState;

InputState input_read(int gamepad_id);
InputState input_read_keyboard(void);
int input_count_gamepads(void);

#endif
