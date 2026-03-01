#include "collision.h"
#include "player.h"

#include <math.h>
#include <float.h>

#define ARENA_PAD 20.0f
#define ARENA_ROUNDNESS 0.05f

void project_corners(const Vector2 *corners, int count,
                     Vector2 axis, float *mn, float *mx)
{
    *mn = *mx = corners[0].x * axis.x + corners[0].y * axis.y;
    for (int i = 1; i < count; i++) {
        float p = corners[i].x * axis.x + corners[i].y * axis.y;
        if (p < *mn) *mn = p;
        if (p > *mx) *mx = p;
    }
}

void obb_corners(Vector2 center, float angle_deg,
                 float half_w, float half_h, Vector2 *out)
{
    float rad = angle_deg * DEG2RAD;
    float c = cosf(rad), s = sinf(rad);
    Vector2 ax = {c, s};
    Vector2 ay = {-s, c};
    out[0] = (Vector2){center.x - half_w*ax.x - half_h*ay.x,
                       center.y - half_w*ax.y - half_h*ay.y};
    out[1] = (Vector2){center.x + half_w*ax.x - half_h*ay.x,
                       center.y + half_w*ax.y - half_h*ay.y};
    out[2] = (Vector2){center.x + half_w*ax.x + half_h*ay.x,
                       center.y + half_w*ax.y + half_h*ay.y};
    out[3] = (Vector2){center.x - half_w*ax.x + half_h*ay.x,
                       center.y - half_w*ax.y + half_h*ay.y};
}

/* --- Pairwise primitive collision resolvers --- */

static Vector2 sat_resolve_corners(const Vector2 *corners_a, int count_a,
                                    const Vector2 *corners_b, int count_b,
                                    const Vector2 *axes, int num_axes)
{
    float min_overlap = FLT_MAX;
    Vector2 push_axis = {0};

    for (int a = 0; a < num_axes; a++) {
        float a_mn, a_mx, b_mn, b_mx;
        project_corners(corners_a, count_a, axes[a], &a_mn, &a_mx);
        project_corners(corners_b, count_b, axes[a], &b_mn, &b_mx);

        float overlap = fminf(a_mx - b_mn, b_mx - a_mn);
        if (overlap <= 0) return (Vector2){0, 0};

        if (overlap < min_overlap) {
            min_overlap = overlap;
            float a_mid = (a_mn + a_mx) * 0.5f;
            float b_mid = (b_mn + b_mx) * 0.5f;
            float sign = (a_mid > b_mid) ? 1.0f : -1.0f;
            push_axis = (Vector2){axes[a].x * sign, axes[a].y * sign};
        }
    }

    return (Vector2){push_axis.x * min_overlap, push_axis.y * min_overlap};
}

Vector2 resolve_rect_rect(Vector2 pos_a, float angle_a, float hw_a, float hh_a,
                          Vector2 pos_b, float angle_b, float hw_b, float hh_b)
{
    Vector2 corners_a[4], corners_b[4];
    obb_corners(pos_a, angle_a, hw_a, hh_a, corners_a);
    obb_corners(pos_b, angle_b, hw_b, hh_b, corners_b);

    float rad_a = angle_a * DEG2RAD;
    float rad_b = angle_b * DEG2RAD;
    Vector2 axes[4] = {
        {cosf(rad_a), sinf(rad_a)},
        {-sinf(rad_a), cosf(rad_a)},
        {cosf(rad_b), sinf(rad_b)},
        {-sinf(rad_b), cosf(rad_b)},
    };

    return sat_resolve_corners(corners_a, 4, corners_b, 4, axes, 4);
}

Vector2 resolve_circle_circle(Vector2 pos_a, float r_a,
                               Vector2 pos_b, float r_b)
{
    float dx = pos_a.x - pos_b.x;
    float dy = pos_a.y - pos_b.y;
    float dist = sqrtf(dx * dx + dy * dy);
    float sum_r = r_a + r_b;

    if (dist >= sum_r || dist < 0.001f)
        return (Vector2){0, 0};

    float overlap = sum_r - dist;
    float nx = dx / dist;
    float ny = dy / dist;
    return (Vector2){nx * overlap, ny * overlap};
}

Vector2 resolve_rect_circle(Vector2 rect_pos, float angle, float hw, float hh,
                             Vector2 circ_pos, float radius)
{
    /* Transform circle center into OBB local space */
    float rad = angle * DEG2RAD;
    float c = cosf(rad), s = sinf(rad);
    float dx = circ_pos.x - rect_pos.x;
    float dy = circ_pos.y - rect_pos.y;
    float local_x = dx * c + dy * s;
    float local_y = -dx * s + dy * c;

    /* Clamp to rect extents to find closest point */
    float closest_x = fmaxf(-hw, fminf(hw, local_x));
    float closest_y = fmaxf(-hh, fminf(hh, local_y));

    float diff_x = local_x - closest_x;
    float diff_y = local_y - closest_y;
    float dist_sq = diff_x * diff_x + diff_y * diff_y;

    if (dist_sq >= radius * radius)
        return (Vector2){0, 0};

    float dist = sqrtf(dist_sq);
    Vector2 push;

    if (dist < 0.001f) {
        /* Circle center is inside rect — push along shortest axis */
        float push_x = hw - fabsf(local_x);
        float push_y = hh - fabsf(local_y);
        float lx, ly;
        if (push_x < push_y) {
            lx = (local_x >= 0 ? 1 : -1) * (push_x + radius);
            ly = 0;
        } else {
            lx = 0;
            ly = (local_y >= 0 ? 1 : -1) * (push_y + radius);
        }
        /* Push is for the rect (push rect away from circle) = negate circle push */
        push = (Vector2){-(lx * c - ly * s), -(lx * s + ly * c)};
    } else {
        float overlap = radius - dist;
        float nx = diff_x / dist;
        float ny = diff_y / dist;
        /* Push in local space points from rect toward circle, negate for rect push */
        float lx = -nx * overlap;
        float ly = -ny * overlap;
        push = (Vector2){lx * c - ly * s, lx * s + ly * c};
    }

    return push;
}

Vector2 resolve_circle_rect(Vector2 circ_pos, float radius,
                             Vector2 rect_pos, float angle, float hw, float hh)
{
    Vector2 v = resolve_rect_circle(rect_pos, angle, hw, hh, circ_pos, radius);
    return (Vector2){-v.x, -v.y};
}

/* --- Triangle collision resolvers --- */

static void tri_world_verts(Vector2 pos, const Vector2 *local_verts, Vector2 *out)
{
    for (int i = 0; i < 3; i++) {
        out[i] = (Vector2){pos.x + local_verts[i].x, pos.y + local_verts[i].y};
    }
}

static void tri_edge_normals(const Vector2 *world_verts, Vector2 *axes)
{
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;
        float ex = world_verts[j].x - world_verts[i].x;
        float ey = world_verts[j].y - world_verts[i].y;
        float len = sqrtf(ex * ex + ey * ey);
        if (len < 0.001f) {
            axes[i] = (Vector2){1, 0};
        } else {
            axes[i] = (Vector2){-ey / len, ex / len};
        }
    }
}

Vector2 resolve_tri_tri(Vector2 pos_a, const Vector2 *verts_a,
                         Vector2 pos_b, const Vector2 *verts_b)
{
    Vector2 wa[3], wb[3];
    tri_world_verts(pos_a, verts_a, wa);
    tri_world_verts(pos_b, verts_b, wb);

    Vector2 axes[6];
    tri_edge_normals(wa, axes);
    tri_edge_normals(wb, axes + 3);

    return sat_resolve_corners(wa, 3, wb, 3, axes, 6);
}

Vector2 resolve_tri_rect(Vector2 tri_pos, const Vector2 *verts,
                          Vector2 rect_pos, float angle, float hw, float hh)
{
    Vector2 wt[3];
    tri_world_verts(tri_pos, verts, wt);

    Vector2 rect_corners[4];
    obb_corners(rect_pos, angle, hw, hh, rect_corners);

    float rad = angle * DEG2RAD;
    Vector2 axes[5];
    tri_edge_normals(wt, axes);
    axes[3] = (Vector2){cosf(rad), sinf(rad)};
    axes[4] = (Vector2){-sinf(rad), cosf(rad)};

    return sat_resolve_corners(wt, 3, rect_corners, 4, axes, 5);
}

Vector2 resolve_rect_tri(Vector2 rect_pos, float angle, float hw, float hh,
                          Vector2 tri_pos, const Vector2 *verts)
{
    Vector2 v = resolve_tri_rect(tri_pos, verts, rect_pos, angle, hw, hh);
    return (Vector2){-v.x, -v.y};
}

Vector2 resolve_tri_circle(Vector2 tri_pos, const Vector2 *verts,
                            Vector2 circ_pos, float radius)
{
    Vector2 wt[3];
    tri_world_verts(tri_pos, verts, wt);

    /* Test triangle edge normals + axis from triangle vertex closest to circle */
    Vector2 axes[4];
    tri_edge_normals(wt, axes);

    /* Find closest vertex to circle center for the Voronoi axis */
    float best_dist = FLT_MAX;
    int best_v = 0;
    for (int i = 0; i < 3; i++) {
        float dx = circ_pos.x - wt[i].x;
        float dy = circ_pos.y - wt[i].y;
        float d = dx * dx + dy * dy;
        if (d < best_dist) { best_dist = d; best_v = i; }
    }
    float dx = circ_pos.x - wt[best_v].x;
    float dy = circ_pos.y - wt[best_v].y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f)
        axes[3] = (Vector2){1, 0};
    else
        axes[3] = (Vector2){dx / len, dy / len};

    /* Project circle as an interval on each axis */
    float min_overlap = FLT_MAX;
    Vector2 push_axis = {0};

    for (int a = 0; a < 4; a++) {
        float t_mn, t_mx;
        project_corners(wt, 3, axes[a], &t_mn, &t_mx);

        float c_proj = circ_pos.x * axes[a].x + circ_pos.y * axes[a].y;
        float c_mn = c_proj - radius;
        float c_mx = c_proj + radius;

        float overlap = fminf(t_mx - c_mn, c_mx - t_mn);
        if (overlap <= 0) return (Vector2){0, 0};

        if (overlap < min_overlap) {
            min_overlap = overlap;
            float t_mid = (t_mn + t_mx) * 0.5f;
            float sign = (t_mid > c_proj) ? 1.0f : -1.0f;
            push_axis = (Vector2){axes[a].x * sign, axes[a].y * sign};
        }
    }

    return (Vector2){push_axis.x * min_overlap, push_axis.y * min_overlap};
}

Vector2 resolve_circle_tri(Vector2 circ_pos, float radius,
                            Vector2 tri_pos, const Vector2 *verts)
{
    Vector2 v = resolve_tri_circle(tri_pos, verts, circ_pos, radius);
    return (Vector2){-v.x, -v.y};
}

/* --- Composite shape resolution --- */

Vector2 prim_world_pos(Vector2 entity_pos, float entity_angle, Vector2 offset)
{
    if (offset.x == 0 && offset.y == 0)
        return entity_pos;

    float rad = entity_angle * DEG2RAD;
    float c = cosf(rad), s = sinf(rad);
    return (Vector2){
        entity_pos.x + offset.x * c - offset.y * s,
        entity_pos.y + offset.x * s + offset.y * c,
    };
}

Vector2 resolve_prim_pair(const CollisionPrimitive *a, Vector2 world_pos_a, float angle_a,
                           const CollisionPrimitive *b, Vector2 world_pos_b, float angle_b)
{
    float a_angle = angle_a + a->angle_offset;
    float b_angle = angle_b + b->angle_offset;

    if (a->kind == COLLIDER_RECT && b->kind == COLLIDER_RECT)
        return resolve_rect_rect(world_pos_a, a_angle, a->rect.half_w, a->rect.half_h,
                                  world_pos_b, b_angle, b->rect.half_w, b->rect.half_h);

    if (a->kind == COLLIDER_CIRCLE && b->kind == COLLIDER_CIRCLE)
        return resolve_circle_circle(world_pos_a, a->circle.radius,
                                      world_pos_b, b->circle.radius);

    if (a->kind == COLLIDER_RECT && b->kind == COLLIDER_CIRCLE)
        return resolve_rect_circle(world_pos_a, a_angle, a->rect.half_w, a->rect.half_h,
                                    world_pos_b, b->circle.radius);

    if (a->kind == COLLIDER_CIRCLE && b->kind == COLLIDER_RECT)
        return resolve_circle_rect(world_pos_a, a->circle.radius,
                                    world_pos_b, b_angle, b->rect.half_w, b->rect.half_h);

    if (a->kind == COLLIDER_TRIANGLE && b->kind == COLLIDER_TRIANGLE)
        return resolve_tri_tri(world_pos_a, a->triangle.verts,
                                world_pos_b, b->triangle.verts);

    if (a->kind == COLLIDER_TRIANGLE && b->kind == COLLIDER_RECT)
        return resolve_tri_rect(world_pos_a, a->triangle.verts,
                                 world_pos_b, b_angle, b->rect.half_w, b->rect.half_h);

    if (a->kind == COLLIDER_RECT && b->kind == COLLIDER_TRIANGLE)
        return resolve_rect_tri(world_pos_a, a_angle, a->rect.half_w, a->rect.half_h,
                                 world_pos_b, b->triangle.verts);

    if (a->kind == COLLIDER_TRIANGLE && b->kind == COLLIDER_CIRCLE)
        return resolve_tri_circle(world_pos_a, a->triangle.verts,
                                   world_pos_b, b->circle.radius);

    if (a->kind == COLLIDER_CIRCLE && b->kind == COLLIDER_TRIANGLE)
        return resolve_circle_tri(world_pos_a, a->circle.radius,
                                   world_pos_b, b->triangle.verts);

    return (Vector2){0, 0};
}

Vector2 resolve_composite(const CollisionShape *a, Vector2 pos_a, float angle_a,
                           const CollisionShape *b, Vector2 pos_b, float angle_b)
{
    Vector2 best = {0, 0};
    float best_mag = 0;

    for (int i = 0; i < a->count; i++) {
        Vector2 wp_a = prim_world_pos(pos_a, angle_a, a->prims[i].offset);
        for (int j = 0; j < b->count; j++) {
            Vector2 wp_b = prim_world_pos(pos_b, angle_b, b->prims[j].offset);
            Vector2 push = resolve_prim_pair(&a->prims[i], wp_a, angle_a,
                                              &b->prims[j], wp_b, angle_b);
            float mag = push.x * push.x + push.y * push.y;
            if (mag > best_mag) {
                best_mag = mag;
                best = push;
            }
        }
    }
    return best;
}

bool composite_overlap(const CollisionShape *a, Vector2 pos_a, float angle_a,
                       const CollisionShape *b, Vector2 pos_b, float angle_b)
{
    Vector2 v = resolve_composite(a, pos_a, angle_a, b, pos_b, angle_b);
    return v.x != 0 || v.y != 0;
}

Vector2 resolve_composite_wall(const CollisionShape *shape, Vector2 pos, float angle,
                                Rectangle wall)
{
    CollisionShape wall_shape = {0};
    wall_shape.count = 1;
    wall_shape.prims[0].kind = COLLIDER_RECT;
    wall_shape.prims[0].offset = (Vector2){0, 0};
    wall_shape.prims[0].angle_offset = 0;
    wall_shape.prims[0].rect.half_w = wall.width / 2;
    wall_shape.prims[0].rect.half_h = wall.height / 2;

    Vector2 wall_center = {wall.x + wall.width / 2, wall.y + wall.height / 2};
    return resolve_composite(shape, pos, angle, &wall_shape, wall_center, 0);
}

void resolve_arena_composite(const CollisionShape *shape, Vector2 *pos, float angle)
{
    float thick = 200.0f;
    float w = (float)screen_width;
    float h = (float)screen_height;
    float arena_w = w - 2 * ARENA_PAD;
    float arena_h = h - 2 * ARENA_PAD;
    float r = fminf(arena_w, arena_h) * ARENA_ROUNDNESS * 0.5f;

    /* 4 edge walls */
    Rectangle edge_walls[4] = {
        {ARENA_PAD - thick, ARENA_PAD,         thick, h - 2 * ARENA_PAD},
        {w - ARENA_PAD,     ARENA_PAD,         thick, h - 2 * ARENA_PAD},
        {ARENA_PAD,         ARENA_PAD - thick,  w - 2 * ARENA_PAD, thick},
        {ARENA_PAD,         h - ARENA_PAD,      w - 2 * ARENA_PAD, thick},
    };
    for (int i = 0; i < 4; i++) {
        Vector2 push = resolve_composite_wall(shape, *pos, angle, edge_walls[i]);
        pos->x += push.x;
        pos->y += push.y;
    }

    /* Corner arcs — push primitive world positions inside the arc */
    Vector2 corner_centers[4] = {
        {ARENA_PAD + r,     ARENA_PAD + r},
        {w - ARENA_PAD - r, ARENA_PAD + r},
        {ARENA_PAD + r,     h - ARENA_PAD - r},
        {w - ARENA_PAD - r, h - ARENA_PAD - r},
    };
    for (int c = 0; c < 4; c++) {
        for (int p = 0; p < shape->count; p++) {
            Vector2 wp = prim_world_pos(*pos, angle, shape->prims[p].offset);
            const CollisionPrimitive *prim = &shape->prims[p];

            if (prim->kind == COLLIDER_RECT) {
                float pa = angle + prim->angle_offset;
                Vector2 ob[4];
                obb_corners(wp, pa, prim->rect.half_w, prim->rect.half_h, ob);
                for (int j = 0; j < 4; j++) {
                    float dx = ob[j].x - corner_centers[c].x;
                    float dy = ob[j].y - corner_centers[c].y;
                    bool in_corner = false;
                    if (c == 0) in_corner = dx < 0 && dy < 0;
                    if (c == 1) in_corner = dx > 0 && dy < 0;
                    if (c == 2) in_corner = dx < 0 && dy > 0;
                    if (c == 3) in_corner = dx > 0 && dy > 0;
                    if (!in_corner) continue;

                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist > r && dist > 0.001f) {
                        float push = dist - r;
                        pos->x -= (dx / dist) * push;
                        pos->y -= (dy / dist) * push;
                    }
                }
            } else if (prim->kind == COLLIDER_CIRCLE) {
                float dx = wp.x - corner_centers[c].x;
                float dy = wp.y - corner_centers[c].y;
                bool in_corner = false;
                if (c == 0) in_corner = dx < 0 && dy < 0;
                if (c == 1) in_corner = dx > 0 && dy < 0;
                if (c == 2) in_corner = dx < 0 && dy > 0;
                if (c == 3) in_corner = dx > 0 && dy > 0;
                if (!in_corner) continue;

                float dist = sqrtf(dx * dx + dy * dy);
                float effective_r = r - prim->circle.radius;
                if (dist > effective_r && dist > 0.001f) {
                    float push = dist - effective_r;
                    pos->x -= (dx / dist) * push;
                    pos->y -= (dy / dist) * push;
                }
            } else if (prim->kind == COLLIDER_TRIANGLE) {
                Vector2 wt[3];
                tri_world_verts(wp, prim->triangle.verts, wt);
                for (int j = 0; j < 3; j++) {
                    float dx = wt[j].x - corner_centers[c].x;
                    float dy = wt[j].y - corner_centers[c].y;
                    bool in_corner = false;
                    if (c == 0) in_corner = dx < 0 && dy < 0;
                    if (c == 1) in_corner = dx > 0 && dy < 0;
                    if (c == 2) in_corner = dx < 0 && dy > 0;
                    if (c == 3) in_corner = dx > 0 && dy > 0;
                    if (!in_corner) continue;

                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist > r && dist > 0.001f) {
                        float push_d = dist - r;
                        pos->x -= (dx / dist) * push_d;
                        pos->y -= (dy / dist) * push_d;
                    }
                }
            }
        }
    }
}
