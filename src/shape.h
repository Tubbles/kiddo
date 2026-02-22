#ifndef SHAPE_H
#define SHAPE_H

#include "raylib.h"

typedef enum {
    SHAPE_CIRCLE,
    SHAPE_SQUARE,
    SHAPE_TRIANGLE,
    SHAPE_STAR,
    SHAPE_COUNT
} ShapeKind;

#define SHAPE_BASE_SIZE 40.0f

Rectangle shape_bounds(ShapeKind kind, Vector2 pos, float scale);
bool shape_overlap(ShapeKind a, Vector2 pos_a, float scale_a,
                   ShapeKind b, Vector2 pos_b, float scale_b);

#endif
