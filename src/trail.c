#include "trail.h"

void trail_init(OrbitTrail *t, int capacity)
{
    t->head = 0;
    t->count = 0;
    t->capacity = capacity;
    if (t->capacity > TRAIL_MAX_POINTS)
        t->capacity = TRAIL_MAX_POINTS;
}

void trail_push(OrbitTrail *t, Vec3 pos)
{
    t->points[t->head] = pos;
    t->head = (t->head + 1) % t->capacity;
    if (t->count < t->capacity)
        t->count++;
}

void trail_pop(OrbitTrail *t)
{
    if (t->count <= 0) return;
    t->head = (t->head - 1 + t->capacity) % t->capacity;
    t->count--;
}

void trail_clear(OrbitTrail *t)
{
    t->head = 0;
    t->count = 0;
}

int trail_get(const OrbitTrail *t, int age, Vec3 *out)
{
    if (age >= t->count) return -1;
    int idx = (t->head - 1 - age + t->capacity) % t->capacity;
    *out = t->points[idx];
    return 0;
}
