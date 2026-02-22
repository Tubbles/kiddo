#include "input.h"

InputState input_read(int gamepad_id)
{
    InputState state = {0};

    if (!IsGamepadAvailable(gamepad_id))
        return state;

    state.left_stick.x = GetGamepadAxisMovement(gamepad_id, GAMEPAD_AXIS_LEFT_X);
    state.left_stick.y = GetGamepadAxisMovement(gamepad_id, GAMEPAD_AXIS_LEFT_Y);
    state.right_stick.x = GetGamepadAxisMovement(gamepad_id, GAMEPAD_AXIS_RIGHT_X);
    state.right_stick.y = GetGamepadAxisMovement(gamepad_id, GAMEPAD_AXIS_RIGHT_Y);

    state.buttons[0] = IsGamepadButtonPressed(gamepad_id, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    state.buttons[1] = IsGamepadButtonPressed(gamepad_id, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    state.buttons[2] = IsGamepadButtonPressed(gamepad_id, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
    state.buttons[3] = IsGamepadButtonPressed(gamepad_id, GAMEPAD_BUTTON_RIGHT_FACE_UP);

    state.left_trigger = GetGamepadAxisMovement(gamepad_id, GAMEPAD_AXIS_LEFT_TRIGGER);
    state.right_trigger = GetGamepadAxisMovement(gamepad_id, GAMEPAD_AXIS_RIGHT_TRIGGER);

    /* Normalize triggers from -1..1 to 0..1 */
    state.left_trigger = (state.left_trigger + 1.0f) * 0.5f;
    state.right_trigger = (state.right_trigger + 1.0f) * 0.5f;

    return state;
}

InputState input_read_keyboard(void)
{
    InputState state = {0};

    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))  state.left_stick.x = -1.0f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) state.left_stick.x =  1.0f;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))    state.left_stick.y = -1.0f;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))   state.left_stick.y =  1.0f;

    if (IsKeyDown(KEY_Q)) state.right_stick.x = -1.0f;
    if (IsKeyDown(KEY_E)) state.right_stick.x =  1.0f;

    state.buttons[0] = IsKeyPressed(KEY_SPACE);
    state.buttons[1] = IsKeyPressed(KEY_TAB);
    state.buttons[2] = IsKeyPressed(KEY_LEFT_SHIFT);
    state.buttons[3] = IsKeyPressed(KEY_LEFT_CONTROL);

    if (IsKeyDown(KEY_Z)) state.left_trigger = 1.0f;
    if (IsKeyDown(KEY_X)) state.right_trigger = 1.0f;

    return state;
}

int input_count_gamepads(void)
{
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if (IsGamepadAvailable(i))
            count++;
    }
    return count;
}
