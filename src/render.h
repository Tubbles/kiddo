#ifndef RENDER_H
#define RENDER_H

#include "raylib.h"
#include "shape.h"

void render_background(void);
void render_shape(ShapeKind kind, Vector2 pos, float rotation, float scale, Color color);

#endif
