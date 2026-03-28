#include "field.h"

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
        }

        for (int i = 0; i < np; i++)
            models[m].params[i] = saved[i];
    }
}
