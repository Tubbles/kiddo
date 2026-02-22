#include "particle.h"
#include <math.h>
#include <stdlib.h>

void particles_init(ParticlePool *pool)
{
    pool->capacity = PARTICLE_INITIAL_CAPACITY;
    pool->count = 0;
    pool->items = malloc(sizeof(Particle) * pool->capacity);
}

static void grow_if_needed(ParticlePool *pool, int needed)
{
    while (pool->count + needed > pool->capacity) {
        pool->capacity *= 2;
        pool->items = realloc(pool->items, sizeof(Particle) * pool->capacity);
    }
}

static float rand_float(float min, float max)
{
    return min + (float)rand() / (float)RAND_MAX * (max - min);
}

void particles_spawn(ParticlePool *pool, Vector2 pos, Color color, int count)
{
    grow_if_needed(pool, count);

    for (int i = 0; i < count; i++) {
        float angle = rand_float(0.0f, 2.0f * PI);
        float speed = rand_float(50.0f, 200.0f);
        Particle p = {
            .position = pos,
            .velocity = {cosf(angle) * speed, sinf(angle) * speed},
            .color = color,
            .lifetime = rand_float(0.5f, 1.5f),
            .size = rand_float(3.0f, 8.0f),
        };
        pool->items[pool->count++] = p;
    }
}

void particles_update(ParticlePool *pool, float dt)
{
    int i = 0;
    while (i < pool->count) {
        Particle *p = &pool->items[i];
        p->lifetime -= dt;

        if (p->lifetime <= 0.0f) {
            pool->items[i] = pool->items[pool->count - 1];
            pool->count--;
            continue;
        }

        p->position.x += p->velocity.x * dt;
        p->position.y += p->velocity.y * dt;
        p->velocity.x *= 0.98f;
        p->velocity.y *= 0.98f;
        i++;
    }
}

void particles_free(ParticlePool *pool)
{
    free(pool->items);
    pool->items = NULL;
    pool->count = 0;
    pool->capacity = 0;
}
