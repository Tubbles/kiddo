#include "unity.h"
#include "player.h"

int screen_width = SCREEN_WIDTH_DEFAULT;
int screen_height = SCREEN_HEIGHT_DEFAULT;

void setUp(void) {}
void tearDown(void) {}

/* test_stub.c */
void test_stub_passes(void);

/* test_shape.c */
void test_circle_bounds_centered(void);
void test_square_bounds_same_as_circle(void);
void test_bounds_scale_affects_size(void);
void test_star_bounds_at_origin(void);

/* test_player.c */
void test_player_init_sets_fields(void);
void test_player_moves_right(void);
void test_player_clamps_to_screen(void);
void test_player_rotation(void);
void test_player_scale_clamps(void);
void test_player_button_changes_color(void);
void test_player_button_changes_shape(void);
void test_player_no_input_no_change(void);

/* test_particle.c */
void test_particle_init(void);
void test_particle_spawn_increases_count(void);
void test_particle_lifetime_expiry(void);
void test_particle_position_updates(void);
void test_particle_capacity_grows(void);
void test_particle_free_cleans_up(void);

/* test_collision.c */
void test_shape_overlap_touching(void);
void test_shape_overlap_apart(void);
void test_shape_overlap_same_position(void);
void test_collision_detect_no_overlap(void);
void test_collision_detect_overlapping(void);
void test_collision_detect_skips_inactive(void);
void test_rect_rect_overlap(void);
void test_rect_rect_no_overlap(void);
void test_rect_rect_rotated(void);
void test_circle_circle_overlap(void);
void test_circle_circle_no_overlap(void);
void test_rect_circle_overlap(void);
void test_rect_circle_no_overlap(void);
void test_circle_rect_is_negated(void);
void test_composite_single_rect_matches_rect_rect(void);
void test_composite_overlap_bool(void);
void test_composite_wall(void);
void test_tri_tri_overlap(void);
void test_tri_tri_no_overlap(void);
void test_tri_circle_overlap(void);
void test_tri_rect_overlap(void);

/* test_entity_shape.c */
void test_entity_shape_init_sets_fields(void);
void test_entity_shape_update_moves(void);
void test_entity_shape_get_obb(void);

/* test_entity_car.c */
void test_entity_car_init_sets_fields(void);
void test_entity_car_facing_angle_updates(void);
void test_entity_car_get_obb(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_stub_passes);

    RUN_TEST(test_circle_bounds_centered);
    RUN_TEST(test_square_bounds_same_as_circle);
    RUN_TEST(test_bounds_scale_affects_size);
    RUN_TEST(test_star_bounds_at_origin);

    RUN_TEST(test_player_init_sets_fields);
    RUN_TEST(test_player_moves_right);
    RUN_TEST(test_player_clamps_to_screen);
    RUN_TEST(test_player_rotation);
    RUN_TEST(test_player_scale_clamps);
    RUN_TEST(test_player_button_changes_color);
    RUN_TEST(test_player_button_changes_shape);
    RUN_TEST(test_player_no_input_no_change);

    RUN_TEST(test_particle_init);
    RUN_TEST(test_particle_spawn_increases_count);
    RUN_TEST(test_particle_lifetime_expiry);
    RUN_TEST(test_particle_position_updates);
    RUN_TEST(test_particle_capacity_grows);
    RUN_TEST(test_particle_free_cleans_up);

    RUN_TEST(test_shape_overlap_touching);
    RUN_TEST(test_shape_overlap_apart);
    RUN_TEST(test_shape_overlap_same_position);
    RUN_TEST(test_collision_detect_no_overlap);
    RUN_TEST(test_collision_detect_overlapping);
    RUN_TEST(test_collision_detect_skips_inactive);
    RUN_TEST(test_rect_rect_overlap);
    RUN_TEST(test_rect_rect_no_overlap);
    RUN_TEST(test_rect_rect_rotated);
    RUN_TEST(test_circle_circle_overlap);
    RUN_TEST(test_circle_circle_no_overlap);
    RUN_TEST(test_rect_circle_overlap);
    RUN_TEST(test_rect_circle_no_overlap);
    RUN_TEST(test_circle_rect_is_negated);
    RUN_TEST(test_composite_single_rect_matches_rect_rect);
    RUN_TEST(test_composite_overlap_bool);
    RUN_TEST(test_composite_wall);
    RUN_TEST(test_tri_tri_overlap);
    RUN_TEST(test_tri_tri_no_overlap);
    RUN_TEST(test_tri_circle_overlap);
    RUN_TEST(test_tri_rect_overlap);

    RUN_TEST(test_entity_shape_init_sets_fields);
    RUN_TEST(test_entity_shape_update_moves);
    RUN_TEST(test_entity_shape_get_obb);

    RUN_TEST(test_entity_car_init_sets_fields);
    RUN_TEST(test_entity_car_facing_angle_updates);
    RUN_TEST(test_entity_car_get_obb);

    return UNITY_END();
}
