#ifndef T96_H
#define T96_H

/* External (magnetospheric) field of T96_01, GSM nT, at GSM (x,y,z) in RE.
 * parmod: Pdyn (nPa), Dst (nT), IMF By (nT), IMF Bz (nT). ps: dipole tilt (rad).
 * Inside the model magnetopause this is the field of the magnetospheric
 * currents, to be added to an internal field. In the boundary layer and
 * outside it, the Fortran subtracts its own 1980 dipole (moment 30574 nT RE^3,
 * tilt ps), so that this plus that dipole is the model's interconnection field
 * there, and adding IGRF instead leaves IGRF minus that dipole. */
void t96_field(const double parmod[4], double ps, double x, double y, double z,
               double *bx, double *by, double *bz);

/* 1 if (x,y,z) is inside T96's model magnetopause (the region where T96_01
 * returns the full model field), 0 if in its boundary layer or outside.
 * Only parmod[0] (Pdyn) sets the boundary. */
int t96_inside(const double parmod[4], double x, double y, double z);

#endif
