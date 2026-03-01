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
    TEST_ASSERT_NOT_NULL(e.vtable->get_collision_shape);
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

void test_entity_shape_get_collision_shape(void)
{
    /* Square -> COLLIDER_RECT */
    Entity e = entity_shape_init(0, (Vector2){100, 100}, SHAPE_SQUARE, RED);
    CollisionShape cs;
    e.vtable->get_collision_shape(&e, &cs);
    TEST_ASSERT_EQUAL_INT(1, cs.count);
    TEST_ASSERT_EQUAL_INT(COLLIDER_RECT, cs.prims[0].kind);
    float sz = SHAPE_BASE_SIZE;
    TEST_ASSERT_FLOAT_WITHIN(1.0f, sz, cs.prims[0].rect.half_w);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, sz, cs.prims[0].rect.half_h);
}

void test_entity_shape_circle_collider(void)
{
    Entity e = entity_shape_init(0, (Vector2){100, 100}, SHAPE_CIRCLE, RED);
    CollisionShape cs;
    e.vtable->get_collision_shape(&e, &cs);
    TEST_ASSERT_EQUAL_INT(1, cs.count);
    TEST_ASSERT_EQUAL_INT(COLLIDER_CIRCLE, cs.prims[0].kind);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, SHAPE_BASE_SIZE, cs.prims[0].circle.radius);
}

void test_entity_shape_triangle_collider(void)
{
    Entity e = entity_shape_init(0, (Vector2){100, 100}, SHAPE_TRIANGLE, RED);
    CollisionShape cs;
    e.vtable->get_collision_shape(&e, &cs);
    TEST_ASSERT_EQUAL_INT(1, cs.count);
    TEST_ASSERT_EQUAL_INT(COLLIDER_TRIANGLE, cs.prims[0].kind);
}

void test_entity_shape_star_collider(void)
{
    Entity e = entity_shape_init(0, (Vector2){100, 100}, SHAPE_STAR, RED);
    CollisionShape cs;
    e.vtable->get_collision_shape(&e, &cs);
    TEST_ASSERT_EQUAL_INT(1, cs.count);
    TEST_ASSERT_EQUAL_INT(COLLIDER_CIRCLE, cs.prims[0].kind);
    TEST_ASSERT_TRUE(cs.prims[0].circle.radius < SHAPE_BASE_SIZE);
}
