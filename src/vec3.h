#ifndef VEC3_H
#define VEC3_H

typedef struct {
    double x, y, z;
} Vec3;

static inline Vec3 vec3_add(Vec3 a, Vec3 b) { return (Vec3){a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline Vec3 vec3_sub(Vec3 a, Vec3 b) { return (Vec3){a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline Vec3 vec3_scale(double s, Vec3 a) { return (Vec3){s*a.x, s*a.y, s*a.z}; }
static inline double vec3_dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
static inline double vec3_len(Vec3 a) { return __builtin_sqrt(vec3_dot(a,a)); }
static inline Vec3 vec3_norm(Vec3 a) { double l = vec3_len(a); return vec3_scale(1.0/l, a); }
static inline Vec3 vec3_zero(void) { return (Vec3){0.0, 0.0, 0.0}; }

#endif
