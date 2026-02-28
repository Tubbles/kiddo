#include "render.h"
#include "player.h"
#include "raymath.h"
#include <math.h>

void render_background(void)
{
    ClearBackground(BLACK);
    Rectangle arena = {20, 20, screen_width - 40, screen_height - 40};
    DrawRectangleRounded(arena, 0.05f, 16, (Color){245, 245, 220, 255});
}

static void draw_star(Vector2 pos, float size, float rotation, Color color)
{
    int points = 5;
    float outer = size;
    float inner = size * 0.4f;

    for (int i = 0; i < points; i++) {
        float angle1 = rotation + (float)i * 2.0f * PI / points - PI / 2.0f;
        float angle2 = angle1 + PI / points;
        float angle3 = angle1 + 2.0f * PI / points;

        Vector2 outer1 = {pos.x + cosf(angle1) * outer, pos.y + sinf(angle1) * outer};
        Vector2 inner1 = {pos.x + cosf(angle2) * inner, pos.y + sinf(angle2) * inner};
        Vector2 outer2 = {pos.x + cosf(angle3) * outer, pos.y + sinf(angle3) * outer};

        DrawTriangle(pos, inner1, outer1, color);
        DrawTriangle(pos, outer2, inner1, color);
    }
}

static void draw_rotated_square(Vector2 pos, float size, float rotation, Color color)
{
    Vector2 corners[4];
    float angles[4] = {-0.75f * PI, -0.25f * PI, 0.25f * PI, 0.75f * PI};

    for (int i = 0; i < 4; i++) {
        float a = rotation + angles[i];
        corners[i] = (Vector2){pos.x + cosf(a) * size, pos.y + sinf(a) * size};
    }

    DrawTriangle(corners[2], corners[1], corners[0], color);
    DrawTriangle(corners[3], corners[2], corners[0], color);
}

static void draw_rotated_triangle(Vector2 pos, float size, float rotation, Color color)
{
    Vector2 verts[3];
    for (int i = 0; i < 3; i++) {
        float a = rotation + (float)i * 2.0f * PI / 3.0f - PI / 2.0f;
        verts[i] = (Vector2){pos.x + cosf(a) * size, pos.y + sinf(a) * size};
    }
    DrawTriangle(verts[2], verts[1], verts[0], color);
}

void render_shape(ShapeKind kind, Vector2 pos, float rotation, float scale, Color color)
{
    float size = SHAPE_BASE_SIZE * scale;

    switch (kind) {
    case SHAPE_CIRCLE:
        DrawCircleV(pos, size, color);
        break;
    case SHAPE_SQUARE:
        draw_rotated_square(pos, size, rotation, color);
        break;
    case SHAPE_TRIANGLE:
        draw_rotated_triangle(pos, size, rotation, color);
        break;
    case SHAPE_STAR:
        draw_star(pos, size, rotation, color);
        break;
    default:
        break;
    }
}

void render_particles(const Particle *particles, int count)
{
    for (int i = 0; i < count; i++) {
        const Particle *p = &particles[i];
        unsigned char alpha = (unsigned char)(255.0f * (p->lifetime / 1.5f));
        if (alpha > 255) alpha = 255;
        Color c = {p->color.r, p->color.g, p->color.b, alpha};
        DrawCircleV(p->position, p->size, c);
    }
}
