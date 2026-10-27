#include "field.h"
#include <math.h>

double field_Bmag(const FieldModel *fm, Vec3 pos)
{
    Vec3 B = fm->eval_B(fm->params, pos);
    return vec3_len(B);
}

Vec3 field_gradB(const FieldModel *fm, Vec3 pos)
{
    double h = 1e-5;
    double Bxp = field_Bmag(fm, (Vec3){pos.x+h, pos.y, pos.z});
    double Bxm = field_Bmag(fm, (Vec3){pos.x-h, pos.y, pos.z});
    double Byp = field_Bmag(fm, (Vec3){pos.x, pos.y+h, pos.z});
    double Bym = field_Bmag(fm, (Vec3){pos.x, pos.y-h, pos.z});
    double Bzp = field_Bmag(fm, (Vec3){pos.x, pos.y, pos.z+h});
    double Bzm = field_Bmag(fm, (Vec3){pos.x, pos.y, pos.z-h});
    return (Vec3){
        (Bxp - Bxm) / (2*h),
        (Byp - Bym) / (2*h),
        (Bzp - Bzm) / (2*h)
    };
}

static Vec3 unit_B(const FieldModel *fm, Vec3 pos)
{
    Vec3 B = fm->eval_B(fm->params, pos);
    double Bmag = vec3_len(B);
    return Bmag > 0.0 ? vec3_scale(1.0 / Bmag, B) : vec3_zero();
}

int field_L_shell(const FieldModel *fm, Vec3 pos, double *L)
{
    if (fm->body_radius <= 0.0 || field_inside_body(fm, pos)) return 0;

    double Bmin = INFINITY;
    Vec3 pmin = pos;
    for (int dir = -1; dir <= 1; dir += 2) {
        Vec3 p = pos;
        int closed = 0;
        /* RK4 along b, step 1% of the geocentric distance */
        for (int i = 0; i < 20000; i++) {
            Vec3 B = fm->eval_B(fm->params, p);
            double Bmag = vec3_len(B);
            if (Bmag <= 0.0) break;
            if (Bmag < Bmin) { Bmin = Bmag; pmin = p; }
            if (field_inside_body(fm, p)) { closed = 1; break; }
            double r = vec3_len(p);
            if (r > 100.0) break;
            double h = dir * 0.01 * r;
            Vec3 k1 = vec3_scale(1.0 / Bmag, B);
            Vec3 k2 = unit_B(fm, vec3_add(p, vec3_scale(0.5 * h, k1)));
            Vec3 k3 = unit_B(fm, vec3_add(p, vec3_scale(0.5 * h, k2)));
            Vec3 k4 = unit_B(fm, vec3_add(p, vec3_scale(h, k3)));
            Vec3 k = vec3_add(vec3_add(k1, k4), vec3_scale(2.0, vec3_add(k2, k3)));
            p = vec3_add(p, vec3_scale(h / 6.0, k));
        }
        if (!closed) return 0;
    }
    *L = vec3_len(pmin);
    return 1;
}

void field_init_all(FieldModel models[FIELD_NUM_MODELS])
{
    field_init_circular(&models[0]);
    field_init_grad_b(&models[1]);
    field_init_higher_grad(&models[2]);
    field_init_sinusoidal(&models[3]);
    field_init_nonphysical(&models[4]);
    field_init_dipole(&models[5]);
    field_init_bottle(&models[6]);
    field_init_tokamak(&models[7]);
    field_init_stellarator(&models[8]);
    field_init_torus(&models[9]);
    field_init_harris(&models[10]);
    field_init_turbulence(&models[11]);
    field_init_t89(&models[12]);
}

void field_retranslate(FieldModel models[FIELD_NUM_MODELS])
{
    /* Save current params, re-init (gets new translated names), restore params */
    for (int m = 0; m < FIELD_NUM_MODELS; m++) {
        double saved[FIELD_MAX_PARAMS];
        for (int i = 0; i < models[m].n_params; i++)
            saved[i] = models[m].params[i];
        int np = models[m].n_params;

        switch (m) {
        case 0: field_init_circular(&models[m]); break;
        case 1: field_init_grad_b(&models[m]); break;
        case 2: field_init_higher_grad(&models[m]); break;
        case 3: field_init_sinusoidal(&models[m]); break;
        case 4: field_init_nonphysical(&models[m]); break;
        case 5: field_init_dipole(&models[m]); break;
        case 6: field_init_bottle(&models[m]); break;
        case 7: field_init_tokamak(&models[m]); break;
        case 8: field_init_stellarator(&models[m]); break;
        case 9: field_init_torus(&models[m]); break;
        case 10: field_init_harris(&models[m]); break;
        case 11: field_init_turbulence(&models[m]); break;
        case 12: field_init_t89(&models[m]); break;
        }

        for (int i = 0; i < np; i++)
            models[m].params[i] = saved[i];
    }
}
