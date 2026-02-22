#include "unity.h"

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

    return UNITY_END();
}
