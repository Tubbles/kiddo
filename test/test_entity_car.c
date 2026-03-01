#include "unity.h"
#include "entity_car.h"

#include <math.h>

void test_entity_car_init_sets_fields(void)
{
    Texture2D tex = {0};
    Entity e = entity_car_init(2, (Vector2){150, 250}, GREEN, &tex);
    TEST_ASSERT_EQUAL_INT(ENTITY_CAR, e.kind);
    TEST_ASSERT_TRUE(e.active);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 150.0f, e.position.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 250.0f, e.position.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, e.car.facing_angle);
    TEST_ASSERT_EQUAL_INT(2, e.gamepad_id);
    TEST_ASSERT_NOT_NULL(e.vtable);
}

void test_entity_car_facing_angle_updates(void)
{
    Texture2D tex = {0};
    Entity e = entity_car_init(0, (Vector2){400, 300}, RED, &tex);
    InputState input = {0};
    input.left_stick.x = 1.0f;
    input.left_stick.y = 0.0f;

    e.vtable->update(&e, input, 0.016f);
    /* Facing angle = atan2(0, 1) * RAD2DEG + 90 = 0 + 90 = 90 */
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 90.0f, e.car.facing_angle);
}

void test_entity_car_get_obb(void)
{
    Texture2D tex = {0};
    Entity e = entity_car_init(0, (Vector2){200, 200}, RED, &tex);
    Vector2 corners[4];
    e.vtable->get_obb(&e, corners);

    /* With facing_angle=0, OBB should be CAR_HALF_W x CAR_HALF_H centered
       at (200,200) */
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 200.0f - CAR_HALF_W, corners[0].x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 200.0f - CAR_HALF_H, corners[0].y);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 200.0f + CAR_HALF_W, corners[2].x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 200.0f + CAR_HALF_H, corners[2].y);
}
