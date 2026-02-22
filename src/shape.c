#include "shape.h"
#include <stdbool.h>

Rectangle shape_bounds(ShapeKind kind, Vector2 pos, float scale)
{
    float size = SHAPE_BASE_SIZE * scale;
    (void)kind;
    return (Rectangle){
        .x = pos.x - size,
        .y = pos.y - size,
        .width = size * 2.0f,
        .height = size * 2.0f,
    };
}

bool shape_overlap(ShapeKind a, Vector2 pos_a, float scale_a,
                   ShapeKind b, Vector2 pos_b, float scale_b)
{
    float ra = SHAPE_BASE_SIZE * scale_a;
    float rb = SHAPE_BASE_SIZE * scale_b;
    float dx = pos_a.x - pos_b.x;
    float dy = pos_a.y - pos_b.y;
    float dist_sq = dx * dx + dy * dy;
    float sum_r = ra + rb;
    (void)a;
    (void)b;
    return dist_sq <= sum_r * sum_r;
}
