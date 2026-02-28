#include "unity.h"
#include "player.h"

void test_player_init_sets_fields(void)
{
    PlayerState p = player_init(2, (Vector2){100, 200}, SHAPE_STAR, BLUE);
    TEST_ASSERT_TRUE(p.active);
    TEST_ASSERT_EQUAL_INT(SHAPE_STAR, p.shape);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, p.position.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, p.position.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, p.rotation);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, p.scale);
    TEST_ASSERT_EQUAL_INT(2, p.gamepad_id);
}

void test_player_moves_right(void)
{
    PlayerState p = player_init(0, (Vector2){400, 300}, SHAPE_CIRCLE, RED);
    InputState input = {0};
    input.left_stick.x = 1.0f;

    PlayerState next = player_update(p, input, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 400.0f + PLAYER_SPEED, next.position.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 300.0f, next.position.y);
}

void test_player_clamps_to_screen(void)
{
    PlayerState p = player_init(0, (Vector2){10, 10}, SHAPE_CIRCLE, RED);
    InputState input = {0};
    input.left_stick.x = -1.0f;
    input.left_stick.y = -1.0f;

    PlayerState next = player_update(p, input, 10.0f);
    TEST_ASSERT_TRUE(next.position.x >= 0);
    TEST_ASSERT_TRUE(next.position.y >= 0);
}

void test_player_rotation(void)
{
    PlayerState p = player_init(0, (Vector2){400, 300}, SHAPE_SQUARE, RED);
    InputState input = {0};
    input.right_stick.x = 1.0f;

    PlayerState next = player_update(p, input, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, PLAYER_ROT_SPEED, next.rotation);
}

void test_player_scale_clamps(void)
{
    PlayerState p = player_init(0, (Vector2){400, 300}, SHAPE_CIRCLE, RED);
    InputState input = {0};
    input.right_trigger = 1.0f;

    /* Scale up for a long time */
    PlayerState next = player_update(p, input, 100.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, PLAYER_SCALE_MAX, next.scale);

    /* Scale down for a long time */
    input.right_trigger = 0.0f;
    input.left_trigger = 1.0f;
    next = player_update(p, input, 100.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, PLAYER_SCALE_MIN, next.scale);
}

void test_player_button_changes_color(void)
{
    PlayerState p = player_init(0, (Vector2){400, 300}, SHAPE_CIRCLE, RED);
    InputState input = {0};
    input.buttons[0] = true;

    PlayerState next = player_update(p, input, 0.016f);
    /* Color should have changed from RED */
    TEST_ASSERT_FALSE(next.color.r == RED.r && next.color.g == RED.g &&
                      next.color.b == RED.b);
}

void test_player_button_changes_shape(void)
{
    PlayerState p = player_init(0, (Vector2){400, 300}, SHAPE_CIRCLE, RED);
    InputState input = {0};
    input.buttons[1] = true;

    PlayerState next = player_update(p, input, 0.016f);
    TEST_ASSERT_EQUAL_INT(SHAPE_SQUARE, next.shape);
}

void test_player_no_input_no_change(void)
{
    PlayerState p = player_init(0, (Vector2){400, 300}, SHAPE_CIRCLE, RED);
    InputState input = {0};

    PlayerState next = player_update(p, input, 0.016f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 400.0f, next.position.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 300.0f, next.position.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, next.rotation);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, next.scale);
}
