#ifndef TRAIL_H
#define TRAIL_H

#include "vec3.h"

#define TRAIL_MAX_POINTS 768000

typedef struct {
    Vec3 points[TRAIL_MAX_POINTS];
    int head;       /* next write index */
    int count;      /* current number of stored points */
    int capacity;   /* <= TRAIL_MAX_POINTS */
} OrbitTrail;

void trail_init(OrbitTrail *t, int capacity);
void trail_push(OrbitTrail *t, Vec3 pos);
void trail_clear(OrbitTrail *t);
void trail_pop(OrbitTrail *t);  /* remove newest point */

/* Get point by age (0 = newest). Returns 0 on success, -1 if age >= count. */
int trail_get(const OrbitTrail *t, int age, Vec3 *out);

#endif
