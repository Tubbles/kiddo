#include "unity.h"
#include "entity_shape.h"

void test_entity_shape_init_sets_fields(void)
{
    Entity e = entity_shape_init(1, (Vector2){200, 300}, SHAPE_STAR, BLUE);
    TEST_ASSERT_EQUAL_INT(ENTITY_SHAPE, e.kind);
    TEST_ASSERT_TRUE(e.active);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, e.position.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 300.0f, e.position.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, e.rotation);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, e.scale);
    TEST_ASSERT_EQUAL_INT(1, e.gamepad_id);
    TEST_ASSERT_EQUAL_INT(SHAPE_STAR, e.shape.shape);
    TEST_ASSERT_NOT_NULL(e.vtable);
    TEST_ASSERT_NOT_NULL(e.vtable->update);
    TEST_ASSERT_NOT_NULL(e.vtable->render);
    TEST_ASSERT_NOT_NULL(e.vtable->get_obb);
}

void test_entity_shape_update_moves(void)
{
    Entity e = entity_shape_init(0, (Vector2){400, 300}, SHAPE_CIRCLE, RED);
    InputState input = {0};
    input.left_stick.x = 1.0f;

    e.vtable->update(&e, input, 1.0f);
    TEST_ASSERT_TRUE(e.position.x > 400.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 300.0f, e.position.y);
}

void test_entity_shape_get_obb(void)
{
    Entity e = entity_shape_init(0, (Vector2){100, 100}, SHAPE_SQUARE, RED);
    Vector2 corners[4];
    e.vtable->get_obb(&e, corners);

    /* With rotation=0, scale=1, the OBB should be a SHAPE_BASE_SIZE square
       centered at (100,100) */
    float sz = SHAPE_BASE_SIZE;
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f - sz, corners[0].x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f - sz, corners[0].y);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f + sz, corners[2].x);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f + sz, corners[2].y);
}
