#include "unity.h"
#include "collision.h"
#include "shape.h"

void test_shape_overlap_touching(void)
{
    /* Two circles at distance exactly 2*radius should overlap */
    bool result = shape_overlap(SHAPE_CIRCLE, (Vector2){0, 0}, 1.0f,
                                SHAPE_CIRCLE, (Vector2){79, 0}, 1.0f);
    TEST_ASSERT_TRUE(result);
}

void test_shape_overlap_apart(void)
{
    bool result = shape_overlap(SHAPE_CIRCLE, (Vector2){0, 0}, 1.0f,
                                SHAPE_CIRCLE, (Vector2){200, 0}, 1.0f);
    TEST_ASSERT_FALSE(result);
}

void test_shape_overlap_same_position(void)
{
    bool result = shape_overlap(SHAPE_SQUARE, (Vector2){100, 100}, 1.0f,
                                SHAPE_TRIANGLE, (Vector2){100, 100}, 1.0f);
    TEST_ASSERT_TRUE(result);
}

void test_collision_detect_no_overlap(void)
{
    PlayerState players[2] = {
        player_init(0, (Vector2){0, 0}, SHAPE_CIRCLE, RED),
        player_init(1, (Vector2){500, 500}, SHAPE_CIRCLE, BLUE),
    };
    CollisionEvent events[MAX_COLLISIONS];
    int count = collision_detect(players, 2, events, MAX_COLLISIONS);
    TEST_ASSERT_EQUAL_INT(0, count);
}

void test_collision_detect_overlapping(void)
{
    PlayerState players[2] = {
        player_init(0, (Vector2){100, 100}, SHAPE_CIRCLE, RED),
        player_init(1, (Vector2){110, 100}, SHAPE_CIRCLE, BLUE),
    };
    CollisionEvent events[MAX_COLLISIONS];
    int count = collision_detect(players, 2, events, MAX_COLLISIONS);
    TEST_ASSERT_EQUAL_INT(1, count);
    TEST_ASSERT_EQUAL_INT(0, events[0].player_a);
    TEST_ASSERT_EQUAL_INT(1, events[0].player_b);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 105.0f, events[0].contact.x);
}

void test_collision_detect_skips_inactive(void)
{
    PlayerState players[2] = {
        player_init(0, (Vector2){100, 100}, SHAPE_CIRCLE, RED),
        player_init(1, (Vector2){110, 100}, SHAPE_CIRCLE, BLUE),
    };
    players[1].active = false;
    CollisionEvent events[MAX_COLLISIONS];
    int count = collision_detect(players, 2, events, MAX_COLLISIONS);
    TEST_ASSERT_EQUAL_INT(0, count);
}
