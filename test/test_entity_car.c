#include "unity.h"
#include "entity_car.h"

#include <math.h>

void test_entity_car_init_sets_fields(void)
{
    Texture2D textures[1] = {{0}};
    Entity e = entity_car_init(2, (Vector2){150, 250}, GREEN, textures, 1);
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
    Texture2D textures[1] = {{0}};
    Entity e = entity_car_init(0, (Vector2){400, 300}, RED, textures, 1);
    InputState input = {0};
    input.left_stick.x = 1.0f;
    input.left_stick.y = 0.0f;

    e.vtable->update(&e, input, 0.016f);
    /* Facing angle = atan2(0, 1) * RAD2DEG + 90 = 0 + 90 = 90 */
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 90.0f, e.car.facing_angle);
}

void test_entity_car_get_collision_shape(void)
{
    Texture2D textures[1] = {{0}};
    Entity e = entity_car_init(0, (Vector2){200, 200}, RED, textures, 1);
    CollisionShape cs;
    e.vtable->get_collision_shape(&e, &cs);

    TEST_ASSERT_EQUAL_INT(1, cs.count);
    TEST_ASSERT_EQUAL_INT(COLLIDER_RECT, cs.prims[0].kind);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, CAR_HALF_W, cs.prims[0].rect.half_w);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, CAR_HALF_H, cs.prims[0].rect.half_h);
}
