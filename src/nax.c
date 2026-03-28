#include "nax.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ---- Interpolation on periodic arrays ---- */

double nax_interp(const double *arr, int nphi, int nfp, double phi)
{
    double period = 2.0 * M_PI / nfp;
    phi = fmod(phi, period);
    if (phi < 0) phi += period;

    double idx = phi / period * nphi;
    int i0 = (int)idx;
    double frac = idx - i0;
    if (i0 >= nphi) i0 = 0;
    int i1 = (i0 + 1) % nphi;
    return arr[i0] * (1.0 - frac) + arr[i1] * frac;
}

/* ---- Sigma ODE (Riccati equation, matching pyQSC) ----
   From pyQSC calculate_r1.py, setting residual=0:
     d(sigma)/d(varphi) = -iota_N*(e4 + 1 + sigma^2) - 2*e2*torsion*abs_G0_over_B0

   where e2 = etabar^2/kappa^2, e4 = e2^2, abs_G0_over_B0 = L/(2*pi),
   iota_N = iota + helicity*nfp (helicity=0 for QA).

   Converting to phi: d(sigma)/d(phi) = (d_varphi/d_phi) * d(sigma)/d(varphi)
   with d_varphi/d_phi = dl_dphi / abs_G0_over_B0 = dl_dphi * 2*pi/L.

   So: d(sigma)/d(phi) = (2*pi*dl_dphi/L) * [-iota_N*(e4+1+sigma^2)] - 2*e2*torsion*dl_dphi
*/

static double sigma_rhs(double sigma, double kappa, double torsion,
                        double dl_dphi, double etabar, double iota_N,
                        int nfp, double L_per_period)
{
    double e2 = etabar * etabar / (kappa * kappa);
    /* abs_G0_over_B0 = L_total/(2pi) = L_per_period*nfp/(2pi)
       B0_over_abs_G0 = 2pi/(L_per_period*nfp)
       d_varphi/d_phi = B0_over_abs_G0 * dl_dphi */
    double factor = 2.0 * M_PI * dl_dphi / (L_per_period * nfp);
    /* G0/B0 = sG * abs_G0_over_B0 = L_per_period*nfp/(2pi) (sG=1) */
    double G0_over_B0 = L_per_period * nfp / (2.0 * M_PI);
    return -factor * iota_N * (e2*e2 + 1.0 + sigma*sigma)
           + factor * 2.0 * e2 * (-torsion) * G0_over_B0;
}

/* Integrate sigma ODE over one field period using RK4.
   Returns sigma at end of period. */
static double sigma_integrate(const NAXConfig *cfg, double sigma0, double iota_N,
                              double L, double *sigma_out)
{
    int N = cfg->nphi;
    double dphi = cfg->phi[1] - cfg->phi[0];
    double s = sigma0;

    if (sigma_out) sigma_out[0] = s;

    for (int i = 0; i < N; i++) {
        double k0  = cfg->kappa[i];
        double t0  = cfg->torsion[i];
        double dl0 = cfg->dl_dphi[i];
        int ih = (i + 1) % N;
        double kh  = 0.5 * (cfg->kappa[i] + cfg->kappa[ih]);
        double th  = 0.5 * (cfg->torsion[i] + cfg->torsion[ih]);
        double dlh = 0.5 * (cfg->dl_dphi[i] + cfg->dl_dphi[ih]);
        double k1v = cfg->kappa[ih];
        double t1v = cfg->torsion[ih];
        double dl1 = cfg->dl_dphi[ih];

        int nfp = cfg->af.nfp;
        double rk1 = dphi * sigma_rhs(s,           k0,  t0,  dl0, cfg->etabar, iota_N, nfp, L);
        double rk2 = dphi * sigma_rhs(s + 0.5*rk1, kh,  th,  dlh, cfg->etabar, iota_N, nfp, L);
        double rk3 = dphi * sigma_rhs(s + 0.5*rk2, kh,  th,  dlh, cfg->etabar, iota_N, nfp, L);
        double rk4 = dphi * sigma_rhs(s + rk3,     k1v, t1v, dl1, cfg->etabar, iota_N, nfp, L);

        s += (rk1 + 2*rk2 + 2*rk3 + rk4) / 6.0;

        /* Guard against Riccati blowup */
        if (!isfinite(s) || fabs(s) > 1e6) return 1e30;

        if (sigma_out && i < N - 1) sigma_out[i+1] = s;
    }
    return s;
}

/* Solve for sigma(phi) and iota via Newton shooting */
static void solve_sigma(NAXConfig *cfg)
{
    /* Compute axis length per field period */
    double L = 0;
    double dphi = cfg->phi[1] - cfg->phi[0];
    for (int i = 0; i < cfg->nphi; i++) L += cfg->dl_dphi[i] * dphi;
    printf("  Axis length per period L = %.6f\n", L);

    double sigma0 = 0.0;  /* Both configs have sigma(0)=0 */

    /* Initial guess for iota_N from integral condition */
    int nfp = cfg->af.nfp;
    double G0_over_B0 = L * nfp / (2.0 * M_PI);
    double num = 0, den = 0;
    for (int i = 0; i < cfg->nphi; i++) {
        double e2 = cfg->etabar * cfg->etabar / (cfg->kappa[i] * cfg->kappa[i]);
        double factor = 2.0 * M_PI * cfg->dl_dphi[i] / (L * nfp);
        num += factor * 2.0 * e2 * (-cfg->torsion[i]) * G0_over_B0 * dphi;
        den += factor * (e2*e2 + 1.0) * dphi;
    }
    double iota_N = (fabs(den) > 1e-30) ? num / den : 0.3;
    printf("  Initial iota_N guess = %.6f\n", iota_N);

    for (int iter = 0; iter < 50; iter++) {
        double s_end = sigma_integrate(cfg, sigma0, iota_N, L, NULL);
        double residual = s_end - sigma0;

        if (fabs(residual) < 1e-12) {
            printf("  sigma converged in %d iterations, residual=%.2e\n", iter, residual);
            break;
        }

        if (!isfinite(residual)) {
            iota_N *= 0.95;
            continue;
        }

        /* Numerical Jacobian */
        double di = 1e-6;
        double s_end_p = sigma_integrate(cfg, sigma0, iota_N + di, L, NULL);
        double dF = (s_end_p - s_end) / di;

        if (!isfinite(dF) || fabs(dF) < 1e-30) {
            s_end_p = sigma_integrate(cfg, sigma0, iota_N - di, L, NULL);
            dF = (s_end - s_end_p) / di;
            if (!isfinite(dF) || fabs(dF) < 1e-30) {
                iota_N *= 1.05;
                continue;
            }
        }

        double correction = residual / dF;
        if (fabs(correction) > 0.1) correction = 0.1 * (correction > 0 ? 1 : -1);
        iota_N -= correction;

        if (iter < 5 || iter % 10 == 0)
            printf("  iter %d: iota_N=%.8f, residual=%.6e\n", iter, iota_N, residual);
    }

    cfg->iota = iota_N;  /* For QA: helicity=0, so iota = iota_N */
    cfg->L_per_period = L;
    sigma_integrate(cfg, sigma0, iota_N, L, cfg->sigma);
}

/* ---- First-order shaping ---- */

static void compute_shaping(NAXConfig *cfg)
{
    for (int i = 0; i < cfg->nphi; i++) {
        double k = cfg->kappa[i];
        if (k < 1e-15) k = 1e-15;

        cfg->X1c[i] = cfg->etabar / k;
        cfg->X1s[i] = 0.0;
        cfg->Y1c[i] = cfg->sigma[i] * k / cfg->etabar;
        cfg->Y1s[i] = k / cfg->etabar;
    }
}

/* ---- Initialize config ---- */

void nax_init(NAXConfig *cfg)
{
    int N = NAX_NPHI;
    cfg->nphi = N;
    double period = 2.0 * M_PI / cfg->af.nfp;
    double dphi = period / N;

    /* Compute axis geometry */
    for (int i = 0; i < N; i++) {
        cfg->phi[i] = i * dphi;
        axis_frenet(&cfg->af, cfg->phi[i], NULL, NULL, NULL, NULL,
                    &cfg->kappa[i], &cfg->torsion[i], &cfg->dl_dphi[i]);
    }

    printf("NAX Config: %s\n", cfg->name);
    printf("  nfp = %d, etabar = %.6f, B0 = %.4f\n",
           cfg->af.nfp, cfg->etabar, cfg->B0);

    /* Solve sigma ODE and get iota */
    solve_sigma(cfg);
    printf("  iota = %.6f\n", cfg->iota);

    /* Print sigma range */
    double smin = 1e30, smax = -1e30;
    for (int i = 0; i < N; i++) {
        if (cfg->sigma[i] < smin) smin = cfg->sigma[i];
        if (cfg->sigma[i] > smax) smax = cfg->sigma[i];
    }
    printf("  sigma range: [%.4f, %.4f]\n", smin, smax);

    /* First-order shaping */
    compute_shaping(cfg);

    printf("  R0_phys = %.3f m, B0_phys = %.3f T\n", cfg->R0_phys, cfg->B0_phys);
}

/* ---- Find nearest phi on axis ---- */

double nax_nearest_phi(const NAXConfig *cfg, Vec3 pos, double phi_guess)
{
    double phi = phi_guess;
    for (int iter = 0; iter < 20; iter++) {
        Vec3 r0, t, n_v, b_v;
        double k, tau, dl;
        axis_frenet(&cfg->af, phi, &r0, &t, &n_v, &b_v, &k, &tau, &dl);

        /* Scale axis position to physical coords */
        r0 = vec3_scale(cfg->R0_phys, r0);
        Vec3 diff = vec3_sub(pos, r0);
        Vec3 t_phys = vec3_scale(dl * cfg->R0_phys, t);
        double f = -vec3_dot(diff, t_phys);
        double fp = vec3_dot(t_phys, t_phys);
        if (fabs(fp) < 1e-30) break;

        double dp = -f / fp;
        phi += dp;
        if (fabs(dp) < 1e-12) break;
    }
    return phi;
}

/* ---- B field evaluation ---- */

Vec3 nax_B(const NAXConfig *cfg, Vec3 pos)
{
    /* 1. Find nearest axis point via scanning + Newton */
    double best_phi = 0;
    double best_dist2 = 1e30;
    for (int i = 0; i < 64; i++) {
        double phi = 2.0 * M_PI * i / 64;
        Vec3 r0 = vec3_scale(cfg->R0_phys, axis_pos(&cfg->af, phi));
        double d2 = vec3_dot(vec3_sub(pos, r0), vec3_sub(pos, r0));
        if (d2 < best_dist2) { best_dist2 = d2; best_phi = phi; }
    }
    double phi_star = nax_nearest_phi(cfg, pos, best_phi);

    /* 2. Frenet frame at phi_star */
    Vec3 r0, tangent, normal, binormal;
    double kappa, torsion_val, dl;
    axis_frenet(&cfg->af, phi_star, &r0, &tangent, &normal, &binormal,
                &kappa, &torsion_val, &dl);
    r0 = vec3_scale(cfg->R0_phys, r0);

    /* 3. Displacement in Frenet frame */
    Vec3 delta = vec3_sub(pos, r0);
    double X = vec3_dot(delta, normal);
    double Y = vec3_dot(delta, binormal);

    /* 4. Invert shaping to find NAE coords (rho, alpha) */
    double x1c = nax_interp(cfg->X1c, cfg->nphi, cfg->af.nfp, phi_star);
    double y1c = nax_interp(cfg->Y1c, cfg->nphi, cfg->af.nfp, phi_star);
    double y1s = nax_interp(cfg->Y1s, cfg->nphi, cfg->af.nfp, phi_star);

    /* X = rho*(x1c*cos(alpha)), Y = rho*(y1c*cos(alpha) + y1s*sin(alpha)) */
    double rc_a, rs_a;
    if (fabs(x1c) < 1e-15 || fabs(y1s) < 1e-15) {
        rc_a = 0; rs_a = 0;
    } else {
        rc_a = X / x1c;
        rs_a = (Y - X * y1c / x1c) / y1s;
    }
    double rho_nae = sqrt(rc_a * rc_a + rs_a * rs_a);
    double alpha = atan2(rs_a, rc_a);

    /* 5-6. B vector from pyQSC first-order NAE (Landreman 2021 eqs 3.5-3.6). */
    double abs_G0B0 = cfg->L_per_period * cfg->af.nfp / (2.0 * M_PI);
    double factor_B = cfg->B0 / abs_G0B0;
    double dl_dv = abs_G0B0;

    double ca = cos(alpha), sa = sin(alpha);

    /* Shaping coefficient derivatives w.r.t. cylindrical phi (finite differences) */
    double dp = 1e-5;
    int nfp_loc = cfg->af.nfp;
    int nphi_loc = cfg->nphi;
    double dx1c_dphi = (nax_interp(cfg->X1c, nphi_loc, nfp_loc, phi_star+dp)
                      - nax_interp(cfg->X1c, nphi_loc, nfp_loc, phi_star-dp)) / (2*dp);
    double dy1c_dphi = (nax_interp(cfg->Y1c, nphi_loc, nfp_loc, phi_star+dp)
                      - nax_interp(cfg->Y1c, nphi_loc, nfp_loc, phi_star-dp)) / (2*dp);
    double dy1s_dphi = (nax_interp(cfg->Y1s, nphi_loc, nfp_loc, phi_star+dp)
                      - nax_interp(cfg->Y1s, nphi_loc, nfp_loc, phi_star-dp)) / (2*dp);

    /* Convert to varphi derivatives: d/dvarphi = (abs_G0B0/dl_dphi) * d/dphi */
    double phi_to_varphi = abs_G0B0 / dl;
    double dx1c_dv = dx1c_dphi * phi_to_varphi;
    double dy1c_dv = dy1c_dphi * phi_to_varphi;
    double dy1s_dv = dy1s_dphi * phi_to_varphi;

    double iota_N = cfg->iota;  /* For QA: helicity=0, so iota_N = iota */

    /* B1 components (X1s = 0, sG = 1) */
    double B1_t = cfg->B0 * kappa * x1c * ca;

    double B1_n = factor_B * (ca * (dx1c_dv - y1c * dl_dv * torsion_val)
                            + sa * (-y1s * dl_dv * torsion_val - iota_N * x1c));

    double B1_b = factor_B * (ca * (dy1c_dv + x1c * dl_dv * torsion_val + iota_N * y1s)
                            + sa * (dy1s_dv - iota_N * y1c));

    /* Full B in Frenet frame (NAE units) */
    double Bt = cfg->B0 + rho_nae * B1_t;
    double Bn = rho_nae * B1_n;
    double Bb = rho_nae * B1_b;

    /* Guard against NaN from degenerate geometry */
    if (!isfinite(Bt) || !isfinite(Bn) || !isfinite(Bb))
        return (Vec3){0.0, 0.0, 0.0};

    /* Scale to physical units and convert to Cartesian */
    double scale = cfg->B0_phys / cfg->B0;
    return vec3_add(vec3_scale(scale * Bt, tangent),
           vec3_add(vec3_scale(scale * Bn, normal),
                    vec3_scale(scale * Bb, binormal)));
}

/* ---- Standard configurations ---- */

void nax_config_r2s51(NAXConfig *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    snprintf(cfg->name, sizeof(cfg->name), "r2 section 5.1 (QA)");

    cfg->af.nfp = 2;
    cfg->af.n_rc = 3;
    cfg->af.rc[0] = 1.0;
    cfg->af.rc[1] = 0.155;
    cfg->af.rc[2] = 0.0102;

    cfg->af.n_zs = 3;
    cfg->af.zs[0] = 0.0;
    cfg->af.zs[1] = 0.154;
    cfg->af.zs[2] = 0.0111;

    cfg->etabar = 0.64;
    cfg->B0 = 1.0;
    cfg->R0_phys = 1.0;
    cfg->B0_phys = 5.0;
}

void nax_config_precise_qa(NAXConfig *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    snprintf(cfg->name, sizeof(cfg->name), "Precise QA (LP 2022)");

    cfg->af.nfp = 2;
    cfg->af.n_rc = 10;
    cfg->af.rc[0] = 1.0038581971135636;
    cfg->af.rc[1] = 0.18400998741139907;
    cfg->af.rc[2] = 0.021723381370503204;
    cfg->af.rc[3] = 0.0025968236014410812;
    cfg->af.rc[4] = 0.00030601568477064874;
    cfg->af.rc[5] = 3.5540509760304384e-05;
    cfg->af.rc[6] = 4.102693907398271e-06;
    cfg->af.rc[7] = 5.154300428457222e-07;
    cfg->af.rc[8] = 4.8802742243232844e-08;
    cfg->af.rc[9] = 7.3011320375259876e-09;

    cfg->af.n_zs = 10;
    cfg->af.zs[0] = 0.0;
    cfg->af.zs[1] = -0.1581148860568176;
    cfg->af.zs[2] = -0.02060702320552523;
    cfg->af.zs[3] = -0.002558840496952667;
    cfg->af.zs[4] = -0.0003061368667524159;
    cfg->af.zs[5] = -3.600111450532304e-05;
    cfg->af.zs[6] = -4.174376962124085e-06;
    cfg->af.zs[7] = -4.557462755956434e-07;
    cfg->af.zs[8] = -8.173481495049928e-08;
    cfg->af.zs[9] = -3.732477282851326e-09;

    cfg->etabar = -0.6783912804454629;
    cfg->B0 = 1.006541121335688;
    cfg->R0_phys = 1.0;
    cfg->B0_phys = 5.0;
}
