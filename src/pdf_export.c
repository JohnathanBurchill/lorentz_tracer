/* Vector PDF export — homemade, no external deps.
 *
 * Standard 14 fonts (Helvetica, Helvetica-Bold, Courier, Symbol). Greek glyphs
 * (α μ κ τ etc.) come from Symbol; ° × come from WinAnsiEncoding. Fonts will
 * not exactly match the loaded TTFs but proportions are close.
 *
 * Emits paths and text directly into an uncompressed content stream. The whole
 * stream is buffered in memory then written when finalized so we know its
 * length. Coordinates use raylib screen-pixel space (Y down); a per-emit Y
 * macro flips to PDF page space (Y up). Page size matches the window.
 */

#include "pdf_export.h"
#include "app.h"
#include "render3d.h"
#include "i18n.h"
#include "diagnostics.h"
#include "trail.h"
#include "field.h"
#include "vec3.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <locale.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PDF_MAX_OBJECTS 1024
#define PDF_MAX_ALPHAS  256

typedef struct {
    FILE *fp;
    long bytes;                       /* current byte offset in file */
    long obj_off[PDF_MAX_OBJECTS];    /* obj_off[N] = offset of object N */
    int n_objects;                    /* highest object number used */

    /* Content stream — buffered in memory until length is known */
    char *cs;
    size_t cs_len, cs_cap;

    /* Page size (raylib screen pixels = PDF points) */
    float page_w, page_h;

    /* ExtGState alpha cache (stroke + fill, applied via /CA + /ca) */
    int alphas_q[PDF_MAX_ALPHAS];     /* quantized 0..255 */
    int n_alphas;

    /* Object numbers filled in during pdf_end() */
    int catalog_obj, pages_obj, page_obj, content_obj;
    int font_helv, font_helv_b, font_cour, font_sym;
    int gs_opaque_obj;
    int gs_objs[PDF_MAX_ALPHAS];
} PdfWriter;

/* ---- content stream buffer ---- */

static void cs_reserve(PdfWriter *w, size_t n)
{
    if (w->cs_len + n + 1 > w->cs_cap) {
        size_t nc = w->cs_cap ? w->cs_cap * 2 : 4096;
        while (nc < w->cs_len + n + 1) nc *= 2;
        char *p = realloc(w->cs, nc);
        if (!p) return;
        w->cs = p;
        w->cs_cap = nc;
    }
}

static void cs_append(PdfWriter *w, const char *s, size_t n)
{
    cs_reserve(w, n);
    if (!w->cs) return;
    memcpy(w->cs + w->cs_len, s, n);
    w->cs_len += n;
    w->cs[w->cs_len] = '\0';
}

static void cs_printf(PdfWriter *w, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0 && n < (int)sizeof(buf)) {
        cs_append(w, buf, (size_t)n);
    } else if (n >= (int)sizeof(buf)) {
        /* Unlikely, but handle long strings (e.g. text content) */
        char *big = malloc((size_t)n + 1);
        if (!big) return;
        va_start(ap, fmt);
        vsnprintf(big, (size_t)n + 1, fmt, ap);
        va_end(ap);
        cs_append(w, big, (size_t)n);
        free(big);
    }
}

/* Y-flip: convert raylib screen Y (down) to PDF Y (up). */
#define PDFY(W, Y_) ((W)->page_h - (float)(Y_))

/* ---- file-level writer ---- */

static int pdf_next_obj(PdfWriter *w)
{
    if (w->n_objects + 1 >= PDF_MAX_OBJECTS) return -1;
    return ++w->n_objects;
}

static void pdf_writef(PdfWriter *w, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) {
        fwrite(buf, 1, (size_t)n, w->fp);
        w->bytes += n;
    }
}

static void pdf_begin_obj(PdfWriter *w, int n)
{
    w->obj_off[n] = w->bytes;
    pdf_writef(w, "%d 0 obj\n", n);
}

static void pdf_end_obj(PdfWriter *w) { pdf_writef(w, "endobj\n"); }

static PdfWriter *pdf_begin(const char *path, float page_w, float page_h)
{
    PdfWriter *w = calloc(1, sizeof(PdfWriter));
    if (!w) return NULL;
    w->fp = fopen(path, "wb");
    if (!w->fp) { free(w); return NULL; }
    w->page_w = page_w;
    w->page_h = page_h;
    /* Header with binary marker so PDF tools recognize the file as binary */
    static const char hdr[] = "%PDF-1.4\n%\xe2\xe3\xcf\xd3\n";
    fwrite(hdr, 1, sizeof(hdr) - 1, w->fp);
    w->bytes = (long)(sizeof(hdr) - 1);
    return w;
}

/* ---- ExtGState alpha cache ---- */

/* Returns name to use after /. -1 means fully opaque (use /GSO). */
static int pdf_alpha_lookup(PdfWriter *w, int alpha_0_255)
{
    if (alpha_0_255 >= 255) return -1;
    if (alpha_0_255 < 0) alpha_0_255 = 0;
    for (int i = 0; i < w->n_alphas; i++)
        if (w->alphas_q[i] == alpha_0_255) return i;
    if (w->n_alphas >= PDF_MAX_ALPHAS) {
        /* Snap to nearest existing */
        int best = 0, best_d = 256;
        for (int i = 0; i < w->n_alphas; i++) {
            int d = abs(w->alphas_q[i] - alpha_0_255);
            if (d < best_d) { best_d = d; best = i; }
        }
        return best;
    }
    w->alphas_q[w->n_alphas] = alpha_0_255;
    return w->n_alphas++;
}

/* ---- Drawing primitives (emit into content stream) ---- */

static void pdf_save(PdfWriter *w)    { cs_append(w, "q\n", 2); }
static void pdf_restore(PdfWriter *w) { cs_append(w, "Q\n", 2); }

static void pdf_set_stroke_color(PdfWriter *w, Color c)
{
    cs_printf(w, "%.4f %.4f %.4f RG\n", c.r/255.0, c.g/255.0, c.b/255.0);
    int idx = pdf_alpha_lookup(w, c.a);
    if (idx >= 0) cs_printf(w, "/GS%d gs\n", idx);
    else cs_append(w, "/GSO gs\n", 8);
}

static void pdf_set_fill_color(PdfWriter *w, Color c)
{
    cs_printf(w, "%.4f %.4f %.4f rg\n", c.r/255.0, c.g/255.0, c.b/255.0);
    int idx = pdf_alpha_lookup(w, c.a);
    if (idx >= 0) cs_printf(w, "/GS%d gs\n", idx);
    else cs_append(w, "/GSO gs\n", 8);
}

static void pdf_set_line_width(PdfWriter *w, float lw)
{
    cs_printf(w, "%.3f w\n", lw);
}

static void pdf_clip_rect(PdfWriter *w, float x, float y, float ww, float hh)
{
    cs_printf(w, "%.4f %.4f %.4f %.4f re W n\n",
              x, PDFY(w, y + hh), ww, hh);
}

static void pdf_rect_fill(PdfWriter *w, float x, float y, float ww, float hh)
{
    cs_printf(w, "%.4f %.4f %.4f %.4f re f\n",
              x, PDFY(w, y + hh), ww, hh);
}

static void pdf_rect_stroke(PdfWriter *w, float x, float y, float ww, float hh)
{
    cs_printf(w, "%.4f %.4f %.4f %.4f re S\n",
              x, PDFY(w, y + hh), ww, hh);
}

static void pdf_line(PdfWriter *w, float x1, float y1, float x2, float y2)
{
    cs_printf(w, "%.4f %.4f m %.4f %.4f l S\n",
              x1, PDFY(w, y1), x2, PDFY(w, y2));
}

static void pdf_triangle_fill(PdfWriter *w,
                              float x1, float y1,
                              float x2, float y2,
                              float x3, float y3)
{
    cs_printf(w, "%.4f %.4f m %.4f %.4f l %.4f %.4f l h f\n",
              x1, PDFY(w, y1), x2, PDFY(w, y2), x3, PDFY(w, y3));
}

static void pdf_quad_fill(PdfWriter *w,
                          float x1, float y1,
                          float x2, float y2,
                          float x3, float y3,
                          float x4, float y4)
{
    cs_printf(w,
              "%.4f %.4f m %.4f %.4f l %.4f %.4f l %.4f %.4f l h f\n",
              x1, PDFY(w, y1), x2, PDFY(w, y2),
              x3, PDFY(w, y3), x4, PDFY(w, y4));
}

/* Filled circle at (cx, cy) with radius r — 4 cubic Béziers. */
static void pdf_circle_fill(PdfWriter *w, float cx, float cy, float r)
{
    const float k = 0.5522847498f;  /* 4*(sqrt(2)-1)/3 — best cubic-Bézier circle */
    float kr = k * r;
    float yc = PDFY(w, cy);
    cs_printf(w,
        "%.4f %.4f m "
        "%.4f %.4f %.4f %.4f %.4f %.4f c "
        "%.4f %.4f %.4f %.4f %.4f %.4f c "
        "%.4f %.4f %.4f %.4f %.4f %.4f c "
        "%.4f %.4f %.4f %.4f %.4f %.4f c "
        "h f\n",
        cx + r, yc,
        cx + r, yc + kr, cx + kr, yc + r, cx,      yc + r,
        cx - kr, yc + r, cx - r,  yc + kr, cx - r, yc,
        cx - r,  yc - kr, cx - kr, yc - r, cx,     yc - r,
        cx + kr, yc - r, cx + r,  yc - kr, cx + r, yc);
}

/* ---- UTF-8 → font + encoded byte ---- */

static int utf8_decode(const char *s, unsigned int *cp)
{
    unsigned char c = (unsigned char)s[0];
    if (c < 0x80) { *cp = c; return 1; }
    if ((c & 0xE0) == 0xC0) {
        unsigned char c1 = (unsigned char)s[1];
        if ((c1 & 0xC0) != 0x80) return 0;
        *cp = ((c & 0x1F) << 6) | (c1 & 0x3F);
        return 2;
    }
    if ((c & 0xF0) == 0xE0) {
        unsigned char c1 = (unsigned char)s[1], c2 = (unsigned char)s[2];
        if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) return 0;
        *cp = ((c & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
        return 3;
    }
    if ((c & 0xF8) == 0xF0) {
        unsigned char c1 = (unsigned char)s[1], c2 = (unsigned char)s[2],
                      c3 = (unsigned char)s[3];
        if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 ||
            (c3 & 0xC0) != 0x80) return 0;
        *cp = ((c & 0x07) << 18) | ((c1 & 0x3F) << 12) |
              ((c2 & 0x3F) << 6) | (c3 & 0x3F);
        return 4;
    }
    return 0;
}

/* Map UTF-8 codepoint to (font, encoded byte).
 * Font codes: 0=Helvetica, 1=Helvetica-Bold, 2=Courier, 3=Symbol.
 * preferred_font is used for ASCII; Greek/Symbol maps to Symbol regardless. */
static void map_codepoint(unsigned int cp, int preferred_font,
                          int *out_font, int *out_byte)
{
    if (cp < 0x80) { *out_font = preferred_font; *out_byte = (int)cp; return; }
    /* Common Latin-1 supplement chars in WinAnsi (Helvetica/Courier) */
    if (cp == 0x00B0 || cp == 0x00D7 || cp == 0x00B1 ||
        cp == 0x2212 /* minus */) {
        *out_font = preferred_font;
        if (cp == 0x2212) *out_byte = '-';
        else              *out_byte = (int)cp & 0xFF;
        return;
    }
    /* Greek (lowercase + a few uppercase) → Symbol font */
    *out_font = 3;
    switch (cp) {
        case 0x0391: *out_byte = 'A'; return;
        case 0x0392: *out_byte = 'B'; return;
        case 0x0393: *out_byte = 'G'; return;
        case 0x0394: *out_byte = 'D'; return;
        case 0x0395: *out_byte = 'E'; return;
        case 0x0396: *out_byte = 'Z'; return;
        case 0x0397: *out_byte = 'H'; return;
        case 0x0398: *out_byte = 'Q'; return;
        case 0x0399: *out_byte = 'I'; return;
        case 0x039A: *out_byte = 'K'; return;
        case 0x039B: *out_byte = 'L'; return;
        case 0x039C: *out_byte = 'M'; return;
        case 0x039D: *out_byte = 'N'; return;
        case 0x039E: *out_byte = 'X'; return;
        case 0x039F: *out_byte = 'O'; return;
        case 0x03A0: *out_byte = 'P'; return;
        case 0x03A1: *out_byte = 'R'; return;
        case 0x03A3: *out_byte = 'S'; return;
        case 0x03A4: *out_byte = 'T'; return;
        case 0x03A5: *out_byte = 'U'; return;
        case 0x03A6: *out_byte = 'F'; return;
        case 0x03A7: *out_byte = 'C'; return;
        case 0x03A8: *out_byte = 'Y'; return;
        case 0x03A9: *out_byte = 'W'; return;
        case 0x03B1: *out_byte = 'a'; return;
        case 0x03B2: *out_byte = 'b'; return;
        case 0x03B3: *out_byte = 'g'; return;
        case 0x03B4: *out_byte = 'd'; return;
        case 0x03B5: *out_byte = 'e'; return;
        case 0x03B6: *out_byte = 'z'; return;
        case 0x03B7: *out_byte = 'h'; return;
        case 0x03B8: *out_byte = 'q'; return;
        case 0x03B9: *out_byte = 'i'; return;
        case 0x03BA: *out_byte = 'k'; return;
        case 0x03BB: *out_byte = 'l'; return;
        case 0x03BC: *out_byte = 'm'; return;
        case 0x03BD: *out_byte = 'n'; return;
        case 0x03BE: *out_byte = 'x'; return;
        case 0x03BF: *out_byte = 'o'; return;
        case 0x03C0: *out_byte = 'p'; return;
        case 0x03C1: *out_byte = 'r'; return;
        case 0x03C2: *out_byte = 'V'; return;
        case 0x03C3: *out_byte = 's'; return;
        case 0x03C4: *out_byte = 't'; return;
        case 0x03C5: *out_byte = 'u'; return;
        case 0x03C6: *out_byte = 'f'; return;
        case 0x03C7: *out_byte = 'c'; return;
        case 0x03C8: *out_byte = 'y'; return;
        case 0x03C9: *out_byte = 'w'; return;
    }
    /* Unmapped: fall back to '?' in preferred font */
    *out_font = preferred_font;
    *out_byte = '?';
}

/* Width of `s` when rendered in PDF Courier (mono, font slot F2): codepoint
 * count × 0.6 em. The on-screen font (SourceCodePro) and PDF Courier have
 * close but not identical advance metrics, so for mono text positioning we
 * compute the Courier-correct width to avoid the right edge sliding past
 * where the layout assumed it would land (which causes axis-label overlap
 * and cramped subscripts). */
static float pdf_mono_width(const char *s, float fsz)
{
    if (!s) return 0;
    int n = 0;
    for (const char *p = s; *p; ) {
        unsigned int cp;
        int nb = utf8_decode(p, &cp);
        if (nb <= 0) break;
        n++;
        p += nb;
    }
    return n * 0.6f * fsz;
}

static void emit_pdf_byte(PdfWriter *w, int byte)
{
    if (byte == '(' || byte == ')' || byte == '\\') {
        char buf[3] = {'\\', (char)byte, 0};
        cs_append(w, buf, 2);
    } else if (byte < 0x20 || byte > 0x7E) {
        cs_printf(w, "\\%03o", byte & 0xFF);
    } else {
        char c = (char)byte;
        cs_append(w, &c, 1);
    }
}

/* Emit text at screen-coord top-left (raylib convention). Multi-font runs
 * (Helvetica + Symbol) handled with Tf switches. */
static void pdf_text(PdfWriter *w, float x, float y_top, const char *utf8,
                     float font_size, int font_idx)
{
    if (!utf8 || !*utf8) return;
    /* Convert raylib top-left to PDF baseline. raylib's DrawTextEx places top
     * of glyph cell at y_top; in PDF the text matrix origin is the baseline.
     * Approximate ascent ≈ 0.8 × font_size. */
    float baseline_y = y_top + font_size * 0.8f;
    cs_printf(w, "BT /F%d %.2f Tf %.4f %.4f Td (",
              font_idx, font_size, x, PDFY(w, baseline_y));
    int cur_font = font_idx;
    const char *p = utf8;
    while (*p) {
        unsigned int cp;
        int n = utf8_decode(p, &cp);
        if (n <= 0) { p++; continue; }
        p += n;
        int f, b;
        map_codepoint(cp, font_idx, &f, &b);
        if (f != cur_font) {
            cs_printf(w, ") Tj /F%d %.2f Tf (", f, font_size);
            cur_font = f;
        }
        emit_pdf_byte(w, b);
    }
    cs_append(w, ") Tj ET\n", 8);
}

/* Emit text rotated by raylib screen-rotation angle (degrees), anchored at
 * the raylib position with pivot (0,0) — i.e. the rotated text's local
 * origin is at (anchor_x, anchor_y). */
static void pdf_text_rot(PdfWriter *w, float anchor_x, float anchor_y,
                         const char *utf8, float font_size, int font_idx,
                         float angle_raylib_deg)
{
    if (!utf8 || !*utf8) return;
    /* Convert raylib rotation (positive = CCW visually with Y-down screen) to
     * PDF rotation: θ_pdf = -angle_raylib. Tm = [a b c d e f]:
     *   a=cos θ, b=sin θ, c=-sin θ, d=cos θ, (e,f)=(x, page_h - y). */
    double th = -angle_raylib_deg * M_PI / 180.0;
    double ca = cos(th), sa = sin(th);
    cs_printf(w, "BT /F%d %.2f Tf %.4f %.4f %.4f %.4f %.4f %.4f Tm (",
              font_idx, font_size, ca, sa, -sa, ca,
              anchor_x, PDFY(w, anchor_y));
    int cur_font = font_idx;
    const char *p = utf8;
    while (*p) {
        unsigned int cp;
        int n = utf8_decode(p, &cp);
        if (n <= 0) { p++; continue; }
        p += n;
        int f, b;
        map_codepoint(cp, font_idx, &f, &b);
        if (f != cur_font) {
            cs_printf(w, ") Tj /F%d %.2f Tf (", f, font_size);
            cur_font = f;
        }
        emit_pdf_byte(w, b);
    }
    cs_append(w, ") Tj ET\n", 8);
}

/* ---- Plot strip emission (mirrors render2d.c) ---- */

static float pmap_x(float px, float pw, double xmin, double xmax, double xv)
{
    if (xmax - xmin < 1e-30) return px;
    return px + (float)((xv - xmin) / (xmax - xmin)) * pw;
}

static float pmap_y(float py, float ph, double ymin, double ymax, double yv)
{
    if (ymax - ymin < 1e-30) return py + ph;
    return py + ph - (float)((yv - ymin) / (ymax - ymin)) * ph;
}

/* Emit polyline of a data series, broken on NaN and time gaps. */
static void emit_series(PdfWriter *w,
                        const double *t, const double *y,
                        int count, int head, int cap, int max_show,
                        double tmin, double tmax, double ymin, double ymax,
                        float px, float py, float pw, float ph,
                        Color color, float lw)
{
    if (count < 2) return;
    int show = count < max_show ? count : max_show;
    int idx_new = (head - 1 + cap) % cap;
    int idx_old = (head - 1 - (show - 1) + cap) % cap;
    double dt_span = t[idx_new] - t[idx_old];
    double gap_thresh = (show > 1 && dt_span > 0)
                        ? dt_span / show * 10.0 : 1e30;

    pdf_set_stroke_color(w, color);
    pdf_set_line_width(w, lw);

    /* Walk in chronological order (oldest → newest) so polylines are natural. */
    int started = 0;
    int prev_valid = 0;
    float px_p = 0, py_p = 0;
    int run_pts = 0;
    /* We'll emit one big stream of m/l ops broken at gaps. */
    for (int i = show - 1; i >= 0; i--) {
        int idx = (head - 1 - i + cap) % cap;
        double tv = t[idx], yv = y[idx];
        if (isnan(yv)) { started = 0; prev_valid = 0; continue; }
        float xs = pmap_x(px, pw, tmin, tmax, tv);
        float ys = pmap_y(py, ph, ymin, ymax, yv);
        if (prev_valid) {
            double dt = tv - t[(head - 1 - (i + 1) + cap) % cap];
            if (dt < 0 || dt > gap_thresh) { started = 0; }
        }
        if (!started) {
            cs_printf(w, "%.4f %.4f m\n", xs, PDFY(w, ys));
            started = 1;
            run_pts = 1;
        } else {
            cs_printf(w, "%.4f %.4f l\n", xs, PDFY(w, ys));
            run_pts++;
        }
        prev_valid = 1;
        px_p = xs; py_p = ys;
        (void)px_p; (void)py_p;
    }
    if (run_pts > 0) cs_append(w, "S\n", 2);
}

/* Dashed series — emit as individual short lines (each "on" segment). */
static void emit_series_dashed(PdfWriter *w,
                               const double *t, const double *y,
                               int count, int head, int cap, int max_show,
                               double tmin, double tmax,
                               double ymin, double ymax,
                               float px, float py, float pw, float ph,
                               Color color, float lw)
{
    if (count < 2) return;
    int show = count < max_show ? count : max_show;
    int idx_new = (head - 1 + cap) % cap;
    int idx_old = (head - 1 - (show - 1) + cap) % cap;
    double dt_span = t[idx_new] - t[idx_old];
    double gap_thresh = (show > 1 && dt_span > 0)
                        ? dt_span / show * 10.0 : 1e30;

    pdf_set_stroke_color(w, color);
    pdf_set_line_width(w, lw);

    float accum = 0.0f;
    float dash_on = 8.0f, dash_off = 4.0f;
    float dash_cycle = dash_on + dash_off;

    for (int i = 1; i < show; i++) {
        int i0 = (head - 1 - (i - 1) + cap) % cap;
        int i1 = (head - 1 - i + cap) % cap;
        if (isnan(y[i0]) || isnan(y[i1])) { accum = 0; continue; }
        double dt = t[i0] - t[i1];
        if (dt < 0 || dt > gap_thresh) { accum = 0; continue; }
        float x1 = pmap_x(px, pw, tmin, tmax, t[i0]);
        float y1f = pmap_y(py, ph, ymin, ymax, y[i0]);
        float x2 = pmap_x(px, pw, tmin, tmax, t[i1]);
        float y2f = pmap_y(py, ph, ymin, ymax, y[i1]);
        if (y[i0] < ymin || y[i0] > ymax ||
            y[i1] < ymin || y[i1] > ymax) continue;
        float dx = x2 - x1, dy = y2f - y1f;
        float seg_len = sqrtf(dx*dx + dy*dy);
        float phase = fmodf(accum, dash_cycle);
        if (phase < dash_on) {
            cs_printf(w, "%.4f %.4f m %.4f %.4f l S\n",
                      x1, PDFY(w, y1f), x2, PDFY(w, y2f));
        }
        accum += seg_len;
    }
}

static void emit_panel_multi(PdfWriter *w, const struct AppState *app,
                             const char *title, const char *gk, const char *units,
                             int field_sel,
                             int fixed_yrange, double fixed_ymin, double fixed_ymax,
                             Rectangle bounds)
{
    const Theme *th = &app->theme;
    float pz = app->plot_zoom;
    float margin_l = fmaxf(70 * pz, 50), margin_r = 4;
    float margin_t = fmaxf(22 * pz, 18), margin_b = fmaxf(60 * pz, 50);
    float px = bounds.x + margin_l;
    float py = bounds.y + margin_t;
    float pw = bounds.width - margin_l - margin_r;
    float ph = bounds.height - margin_t - margin_b;

    pdf_set_fill_color(w, th->plot_bg);
    pdf_rect_fill(w, bounds.x, bounds.y, bounds.width, bounds.height);
    Color inner = (Color){th->plot_bg.r-5, th->plot_bg.g-5, th->plot_bg.b-5, 255};
    pdf_set_fill_color(w, inner);
    pdf_rect_fill(w, px, py, pw, ph);
    pdf_set_stroke_color(w, th->text_dim);
    pdf_set_line_width(w, 1.0f);
    pdf_rect_stroke(w, px, py, pw, ph);

    float fsz = 14 * pz;
    float title_fsz = 14 * pz;

    /* Title centered. The title comes in as a UTF-8 prefix like "α (°)" or
     * "μ (J/T)" — we just emit it; positioning uses raylib screen metrics
     * (already used for the on-screen layout, kept consistent here). */
    Vector2 tw = MeasureTextEx(app->font_ui, title, title_fsz, 1);
    pdf_set_fill_color(w, th->text_dim);
    pdf_text(w, px + pw/2 - tw.x/2, bounds.y + 4, title, title_fsz, 0);

    /* Y-axis label: "<gk> (<units>)" rotated -90°. We emit with the same
     * anchor convention raylib uses. The label is built at one font size. */
    char ylabel[64];
    snprintf(ylabel, sizeof(ylabel), "%s %s", gk, units);
    {
        Vector2 yw = MeasureTextEx(app->font_ui, ylabel, fsz, 1);
        float label_x = px - 50;
        float label_y = py + ph/2 + yw.x/2;
        pdf_text_rot(w, label_x, label_y, ylabel, fsz, 0, -90.0f);
    }

    int sel = app->selected_particle;
    if (sel < 0 || sel >= app->n_particles) return;
    const DiagTimeSeries *d = &app->diags[sel];

    int max_show_table[] = {480, 4800, 48000};
    int pr = app->plot_range;
    if (pr < 0) pr = 0; if (pr > 2) pr = 2;
    int max_show = max_show_table[pr];
    if (max_show > d->capacity) max_show = d->capacity;
    int show_count = d->count < max_show ? d->count : max_show;
    if (show_count < 2) return;

    int idx_newest = (d->head - 1 + d->capacity) % d->capacity;
    int idx_oldest = (d->head - 1 - (d->count - 1) + d->capacity) % d->capacity;
    double t_newest = d->time[idx_newest];
    double t_oldest = d->time[idx_oldest];

    double tmin, tmax;
    if (d->count < max_show) {
        tmin = 0.0;
        double dt_per_sample = (d->count > 1)
                               ? (t_newest - t_oldest) / (d->count - 1) : 1.0;
        tmax = dt_per_sample * (max_show - 1);
        if (tmax < t_newest) tmax = t_newest * 1.1;
    } else {
        int idx_win_oldest = (d->head - 1 - (max_show - 1) + d->capacity) % d->capacity;
        tmin = d->time[idx_win_oldest];
        tmax = t_newest;
    }
    if (tmax - tmin < 1e-30) tmax = tmin + 1.0;

    double ymin, ymax;
    const double *sel_y = (field_sel == 0) ? d->pitch_angle : d->mu;
    if (fixed_yrange) {
        ymin = fixed_ymin; ymax = fixed_ymax;
    } else {
        double lo = 1e30, hi = -1e30;
        for (int pi = 0; pi < app->n_particles; pi++) {
            const DiagTimeSeries *di = &app->diags[pi];
            const double *yi = (field_sel == 0) ? di->pitch_angle : di->mu;
            int cnt = di->count < max_show ? di->count : max_show;
            for (int i = 0; i < cnt; i++) {
                int idx = (di->head - 1 - i + di->capacity) % di->capacity;
                if (isnan(yi[idx])) continue;
                if (yi[idx] < lo) lo = yi[idx];
                if (yi[idx] > hi) hi = yi[idx];
            }
        }
        double yrange = hi - lo;
        double ypad;
        if (yrange > 0.0) ypad = yrange * 0.05;
        else { double mag = fabs(hi) + fabs(lo);
               ypad = (mag > 0.0) ? mag * 0.1 : 1.0; }
        ymin = lo - ypad; ymax = hi + ypad;
        if (lo >= 0.0 && ymin < 0.0) ymin = 0.0;
    }

    /* Clip line drawing to the plot box so series can't escape it. */
    pdf_save(w);
    pdf_clip_rect(w, px, py, pw, ph);

    /* GC traces (dashed, darker) */
    if (app->show_gc_trajectory) {
        for (int pi = 0; pi < app->n_particles; pi++) {
            const DiagTimeSeries *di = &app->diags[pi];
            const double *gc_y = (field_sel == 0) ? di->gc_pitch_angle : di->gc_mu;
            int sp = app->particles[pi].species;
            if (sp < 0 || sp >= NUM_SPECIES) sp = NUM_SPECIES - 1;
            Color c = app->species_colors[sp];
            c.r = (unsigned char)(c.r * 0.55f);
            c.g = (unsigned char)(c.g * 0.55f);
            c.b = (unsigned char)(c.b * 0.55f);
            if (pi != sel) c.a = 60;
            emit_series_dashed(w, di->time, gc_y, di->count, di->head,
                               di->capacity, max_show, tmin, tmax, ymin, ymax,
                               px, py, pw, ph, c, 2.0f);
        }
    }

    /* Background particles (dimmed) */
    for (int pi = 0; pi < app->n_particles; pi++) {
        if (pi == sel) continue;
        const DiagTimeSeries *di = &app->diags[pi];
        const double *yi = (field_sel == 0) ? di->pitch_angle : di->mu;
        int sp = app->particles[pi].species;
        if (sp < 0 || sp >= NUM_SPECIES) sp = NUM_SPECIES - 1;
        Color c = app->species_colors[sp];
        c.a = 60;
        emit_series(w, di->time, yi, di->count, di->head, di->capacity,
                    max_show, tmin, tmax, ymin, ymax,
                    px, py, pw, ph, c, 2.0f);
    }

    /* Selected particle on top */
    {
        int sp = app->particles[sel].species;
        if (sp < 0 || sp >= NUM_SPECIES) sp = NUM_SPECIES - 1;
        Color c = app->species_colors[sp];
        emit_series(w, d->time, sel_y, d->count, d->head, d->capacity,
                    max_show, tmin, tmax, ymin, ymax,
                    px, py, pw, ph, c, 2.0f);
    }

    pdf_restore(w);

    /* Tick labels */
    float tsz = 11 * pz;
    char buf[32];
    pdf_set_fill_color(w, th->text_dim);

    snprintf(buf, sizeof(buf), "%.3g", ymin);
    pdf_text(w, px - pdf_mono_width(buf, tsz) - 7,
             py + ph - tsz, buf, tsz, 2);
    snprintf(buf, sizeof(buf), "%.3g", ymax);
    pdf_text(w, px - pdf_mono_width(buf, tsz) - 7, py, buf, tsz, 2);

    float tick_y = py + ph + 3;
    snprintf(buf, sizeof(buf), "%.3g", tmin);
    pdf_text(w, px, tick_y, buf, tsz, 2);
    snprintf(buf, sizeof(buf), "%.3g", tmax);
    pdf_text(w, px + pw - pdf_mono_width(buf, tsz),
             tick_y, buf, tsz, 2);

    /* Tick marks */
    pdf_set_stroke_color(w, th->text_dim);
    pdf_set_line_width(w, 1.0f);
    pdf_line(w, px, py + ph, px, py + ph + 3);
    pdf_line(w, px + pw, py + ph, px + pw, py + ph + 3);

    /* X-axis label */
    Vector2 tlw = MeasureTextEx(app->font_ui, TR(STR_PLOT_TIME), tsz, 1);
    pdf_text(w, px + pw/2 - tlw.x/2, tick_y + tsz + 2, TR(STR_PLOT_TIME), tsz, 0);
}

static void emit_phase_panel(PdfWriter *w, const struct AppState *app,
                             Rectangle bounds)
{
    const Theme *th = &app->theme;
    float pz = app->plot_zoom;
    float margin_l = fmaxf(70 * pz, 50);
    float margin_r = 4;
    float margin_t = fmaxf(22 * pz, 18);
    float margin_b = fmaxf(60 * pz, 50);
    float px = bounds.x + margin_l;
    float py = bounds.y + margin_t;
    float pw = bounds.width - margin_l - margin_r;
    float ph = bounds.height - margin_t - margin_b;

    pdf_set_fill_color(w, th->plot_bg);
    pdf_rect_fill(w, bounds.x, bounds.y, bounds.width, bounds.height);
    Color inner = (Color){th->plot_bg.r-5, th->plot_bg.g-5, th->plot_bg.b-5, 255};
    pdf_set_fill_color(w, inner);
    pdf_rect_fill(w, px, py, pw, ph);
    pdf_set_stroke_color(w, th->text_dim);
    pdf_set_line_width(w, 1.0f);
    pdf_rect_stroke(w, px, py, pw, ph);

    const char *x_gk = app->phase_swap_axes ? "\xce\xba" : "\xcf\x84";
    const char *y_gk = app->phase_swap_axes ? "\xcf\x84" : "\xce\xba";

    float fsz = 14 * pz;
    float title_fsz = 14 * pz;

    /* Title: "v_<y> vs v_<x>" with subscripts. Build same as render2d.c */
    {
        float sub_fsz = title_fsz * 0.75f;
        float sub_dy  = title_fsz * 0.3f;
        Vector2 vsz  = MeasureTextEx(app->font_ui, "v",   title_fsz, 1);
        Vector2 ksz  = MeasureTextEx(app->font_ui, y_gk, sub_fsz, 1);
        Vector2 vssz = MeasureTextEx(app->font_ui, " vs ", title_fsz, 1);
        Vector2 tszg = MeasureTextEx(app->font_ui, x_gk, sub_fsz, 1);
        float total = 2*vsz.x + ksz.x + tszg.x + vssz.x;
        float tx = px + pw/2 - total/2;
        float ty = bounds.y + 4;
        pdf_set_fill_color(w, th->text_dim);
        pdf_text(w, tx, ty, "v", title_fsz, 0); tx += vsz.x;
        pdf_text(w, tx, ty + sub_dy, y_gk, sub_fsz, 0); tx += ksz.x;
        pdf_text(w, tx, ty, " vs ", title_fsz, 0); tx += vssz.x;
        pdf_text(w, tx, ty, "v", title_fsz, 0); tx += vsz.x;
        pdf_text(w, tx, ty + sub_dy, x_gk, sub_fsz, 0);
    }

    int sel = app->selected_particle;
    if (sel < 0 || sel >= app->n_particles) return;
    const DiagTimeSeries *d = &app->diags[sel];
    int max_show_table[] = {480, 4800, 48000};
    int pr = app->plot_range;
    if (pr < 0) pr = 0; if (pr > 2) pr = 2;
    int max_show = max_show_table[pr];
    if (max_show > d->capacity) max_show = d->capacity;
    int show = d->count < max_show ? d->count : max_show;
    if (show < 2) return;

    double absmax = 0;
    for (int i = 0; i < show; i++) {
        int idx = (d->head - 1 - i + d->capacity) % d->capacity;
        if (isnan(d->v_kappa[idx]) || isnan(d->v_tau[idx])) continue;
        double a = fabs(d->v_kappa[idx]);
        double b = fabs(d->v_tau[idx]);
        if (a > absmax) absmax = a;
        if (b > absmax) absmax = b;
    }
    if (!(absmax > 0.0)) absmax = 1.0;
    double ymin = -absmax * 1.05, ymax = absmax * 1.05;
    double xmin = ymin, xmax = ymax;

    /* Y-axis label "v_<y_gk> (m/s)" rotated -90° */
    {
        float sub_fsz = fsz * 0.75f;
        Vector2 vsz = MeasureTextEx(app->font_ui, "v", fsz, 1);
        Vector2 gsz = MeasureTextEx(app->font_ui, y_gk, sub_fsz, 1);
        Vector2 usz = MeasureTextEx(app->font_ui, " (m/s)", fsz, 1);
        float total_w = vsz.x + gsz.x + usz.x;
        float anchor_x = px - 50;
        float anchor_y = py + ph/2 + total_w/2;
        /* Compose rotated label as separate runs. The math for stacking
         * after -90° rotation matches the screen code's draw_v_label_rot. */
        float sub_dy = fsz * 0.3f;
        pdf_set_fill_color(w, th->text_dim);
        pdf_text_rot(w, anchor_x, anchor_y, "v", fsz, 0, -90.0f);
        pdf_text_rot(w, anchor_x + sub_dy, anchor_y - vsz.x,
                     y_gk, sub_fsz, 0, -90.0f);
        pdf_text_rot(w, anchor_x, anchor_y - vsz.x - gsz.x,
                     " (m/s)", fsz, 0, -90.0f);
    }

    /* Trajectory (selected particle only) */
    int sp = app->particles[sel].species;
    if (sp < 0 || sp >= NUM_SPECIES) sp = NUM_SPECIES - 1;
    Color c = app->species_colors[sp];

    int idx_new = (d->head - 1 + d->capacity) % d->capacity;
    int idx_old = (d->head - 1 - (show - 1) + d->capacity) % d->capacity;
    double dt_span = d->time[idx_new] - d->time[idx_old];
    double gap_thresh = (show > 1 && dt_span > 0)
                        ? dt_span / show * 10.0 : 1e30;

    double sx_sign = app->phase_flip_x ? -1.0 : 1.0;
    double sy_sign = app->phase_flip_y ? -1.0 : 1.0;

    /* Clip to inner plot rect */
    pdf_save(w);
    pdf_clip_rect(w, px, py, pw, ph);
    pdf_set_stroke_color(w, c);
    pdf_set_line_width(w, 2.0f);

    /* Walk oldest-to-newest emitting m/l ops; break on NaN/gap. */
    int started = 0;
    for (int i = show - 1; i >= 1; i--) {
        int i0 = (d->head - 1 - (i - 1) + d->capacity) % d->capacity;
        int i1 = (d->head - 1 - i + d->capacity) % d->capacity;
        if (isnan(d->v_kappa[i0]) || isnan(d->v_kappa[i1])) { started = 0; continue; }
        if (isnan(d->v_tau[i0])   || isnan(d->v_tau[i1]))   { started = 0; continue; }
        double dt = d->time[i0] - d->time[i1];
        if (dt < 0 || dt > gap_thresh) { started = 0; continue; }
        double xv1 = app->phase_swap_axes ? d->v_kappa[i1] : d->v_tau[i1];
        double yv1 = app->phase_swap_axes ? d->v_tau[i1]   : d->v_kappa[i1];
        double xv0 = app->phase_swap_axes ? d->v_kappa[i0] : d->v_tau[i0];
        double yv0 = app->phase_swap_axes ? d->v_tau[i0]   : d->v_kappa[i0];
        xv1 *= sx_sign; yv1 *= sy_sign;
        xv0 *= sx_sign; yv0 *= sy_sign;
        float xs1 = pmap_x(px, pw, xmin, xmax, xv1);
        float ys1 = pmap_y(py, ph, ymin, ymax, yv1);
        float xs0 = pmap_x(px, pw, xmin, xmax, xv0);
        float ys0 = pmap_y(py, ph, ymin, ymax, yv0);
        if (!started) {
            cs_printf(w, "%.4f %.4f m\n", xs1, PDFY(w, ys1));
            started = 1;
        }
        cs_printf(w, "%.4f %.4f l\n", xs0, PDFY(w, ys0));
    }
    if (started) cs_append(w, "S\n", 2);

    /* Direction arrow at the current point */
    {
        int i_now = (d->head - 1 + d->capacity) % d->capacity;
        int max_back = show - 1;
        if (max_back > 8) max_back = 8;
        float dx = 0, dy = 0, mag = 0;
        float sx_now = 0, sy_now = 0;
        int found = 0;
        if (!isnan(d->v_kappa[i_now]) && !isnan(d->v_tau[i_now])) {
            double xv = (app->phase_swap_axes ? d->v_kappa[i_now]
                                              : d->v_tau[i_now]) * sx_sign;
            double yv = (app->phase_swap_axes ? d->v_tau[i_now]
                                              : d->v_kappa[i_now]) * sy_sign;
            sx_now = pmap_x(px, pw, xmin, xmax, xv);
            sy_now = pmap_y(py, ph, ymin, ymax, yv);
            for (int back = 1; back <= max_back; back++) {
                int i_old = (d->head - 1 - back + d->capacity) % d->capacity;
                if (isnan(d->v_kappa[i_old]) || isnan(d->v_tau[i_old])) continue;
                double xv_o = (app->phase_swap_axes ? d->v_kappa[i_old]
                                                    : d->v_tau[i_old]) * sx_sign;
                double yv_o = (app->phase_swap_axes ? d->v_tau[i_old]
                                                    : d->v_kappa[i_old]) * sy_sign;
                float sx_old = pmap_x(px, pw, xmin, xmax, xv_o);
                float sy_old = pmap_y(py, ph, ymin, ymax, yv_o);
                dx = sx_now - sx_old; dy = sy_now - sy_old;
                mag = sqrtf(dx*dx + dy*dy);
                if (mag > 1.0f) { found = 1; break; }
            }
        }
        if (found) {
            float dirx = dx / mag, diry = dy / mag;
            float perpx = -diry, perpy = dirx;
            float L = 10.0f, hw = 5.0f;
            pdf_set_fill_color(w, c);
            pdf_triangle_fill(w,
                sx_now, sy_now,
                sx_now - dirx*L + perpx*hw, sy_now - diry*L + perpy*hw,
                sx_now - dirx*L - perpx*hw, sy_now - diry*L - perpy*hw);
        }
    }

    /* Zero crosshair */
    {
        float x0 = pmap_x(px, pw, xmin, xmax, 0.0);
        float ycen = pmap_y(py, ph, ymin, ymax, 0.0);
        Color gc = th->text_dim;
        gc.a = 60;
        pdf_set_stroke_color(w, gc);
        pdf_set_line_width(w, 1.0f);
        pdf_line(w, px, ycen, px + pw, ycen);
        pdf_line(w, x0, py, x0, py + ph);
    }

    pdf_restore(w);

    /* Tick labels */
    float tsz = 11 * pz;
    char buf[32];
    double y_top    = app->phase_flip_y ? -absmax :  absmax;
    double y_bottom = app->phase_flip_y ?  absmax : -absmax;
    double x_left   = app->phase_flip_x ?  absmax : -absmax;
    double x_right  = app->phase_flip_x ? -absmax :  absmax;
    pdf_set_fill_color(w, th->text_dim);

    snprintf(buf, sizeof(buf), "%+.2g", y_bottom);
    pdf_text(w, px - pdf_mono_width(buf, tsz) - 7,
             py + ph - tsz, buf, tsz, 2);
    snprintf(buf, sizeof(buf), "%+.2g", y_top);
    pdf_text(w, px - pdf_mono_width(buf, tsz) - 7, py, buf, tsz, 2);

    float tick_y = py + ph + 3;
    snprintf(buf, sizeof(buf), "%+.2g", x_left);
    pdf_text(w, px, tick_y, buf, tsz, 2);
    snprintf(buf, sizeof(buf), "%+.2g", x_right);
    pdf_text(w, px + pw - pdf_mono_width(buf, tsz),
             tick_y, buf, tsz, 2);

    pdf_set_stroke_color(w, th->text_dim);
    pdf_set_line_width(w, 1.0f);
    pdf_line(w, px, py + ph, px, py + ph + 3);
    pdf_line(w, px + pw, py + ph, px + pw, py + ph + 3);

    /* X-axis label "v_<x_gk> (m/s)" — composed runs at same baseline. */
    {
        float sub_fsz = tsz * 0.75f;
        float sub_dy  = tsz * 0.3f;
        Vector2 vsz = MeasureTextEx(app->font_ui, "v", tsz, 1);
        Vector2 gsz = MeasureTextEx(app->font_ui, x_gk, sub_fsz, 1);
        Vector2 usz = MeasureTextEx(app->font_ui, " (m/s)", tsz, 1);
        float total_w = vsz.x + gsz.x + usz.x;
        float xlx = px + pw/2 - total_w/2;
        float xly = tick_y + tsz + 2;
        pdf_set_fill_color(w, th->text_dim);
        pdf_text(w, xlx, xly, "v", tsz, 0); xlx += vsz.x;
        pdf_text(w, xlx, xly + sub_dy, x_gk, sub_fsz, 0); xlx += gsz.x;
        pdf_text(w, xlx, xly, " (m/s)", tsz, 0);
    }
}

static void emit_plot_strip(PdfWriter *w, const struct AppState *app)
{
    if (!app->show_plots) return;
    float sx, sy, sw, sh;
    app_scene_rect(app, &sx, &sy, &sw, &sh);
    float plot_h = app->win_h * app->plots_height_frac;
    float y0 = (app->plots_edge == 1) ? 0.0f : sy + sh;

    float outer_pad = 10.0f;
    float strip_x = sx + outer_pad;
    float strip_w = sw - 2 * outer_pad;
    if (strip_w < 100) strip_w = 100;

    float pz = app->plot_zoom;
    float m_l = fmaxf(70 * pz, 50);
    float m_r = 4;
    float m_t = fmaxf(22 * pz, 18);
    float m_b = fmaxf(60 * pz, 50);
    float inner_h = plot_h - m_t - m_b;
    if (inner_h < 1) inner_h = 1;
    float phase_w = inner_h + m_l + m_r;
    if (phase_w > strip_w * 0.5f) phase_w = strip_w * 0.5f;
    float ts_w = (strip_w - phase_w) / 2.0f;
    if (ts_w < 50) ts_w = 50;

    int pitch_fixed = !app->pitch_autoscale;
    /* Pass title and axis tokens separately so the y-axis label can be
     * built like the screen code (Greek + units). The strings below are
     * UTF-8 already and the Greek glyphs route to Symbol font. */
    emit_panel_multi(w, app,
        TR(STR_PLOT_PITCH_ANGLE), "\xce\xb1", "(\xc2\xb0)", 0,
        pitch_fixed, 0.0, 180.0,
        (Rectangle){strip_x, y0, ts_w, plot_h});
    emit_panel_multi(w, app,
        TR(STR_PLOT_MAG_MOMENT), "\xce\xbc", "(J/T)", 1,
        0, 0.0, 0.0,
        (Rectangle){strip_x + ts_w, y0, ts_w, plot_h});
    emit_phase_panel(w, app,
        (Rectangle){strip_x + 2 * ts_w, y0, phase_w, plot_h});
}

/* ---- Overlays (Gij + initial conditions) ---- */

static void emit_gij_overlay(PdfWriter *w, const struct AppState *app,
                             float scene_x, float scene_y)
{
    (void)scene_y;
    if (!app->show_Gij || !app->Gij_valid) return;

    float gz = app->gij_zoom;
    float gx = scene_x + 14, gy = 10;
    float fsz = 11 * gz;
    float row_sp = 14 * gz;

    double maxval = 0;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            double a = fabs(app->Gij[i][j]);
            if (a > maxval) maxval = a;
        }
    int exponent = 0;
    double scale = 1.0;
    if (maxval > 1e-30) {
        double raw = floor(log10(maxval));
        exponent = (int)(trunc(raw / 3.0)) * 3;
        scale = pow(10.0, -exponent);
    }
    Color tc = app->theme.text_dim;
    float exp_fsz = fsz * 0.75f;

    /* "G" with subscript "ij" */
    pdf_set_fill_color(w, tc);
    pdf_text(w, gx, gy, "G", fsz, 2);
    Vector2 gsz = MeasureTextEx(app->font_mono, "G", fsz, 1);
    pdf_text(w, gx + gsz.x, gy + fsz * 0.3f, "ij", exp_fsz, 2);
    gy += row_sp;

    float mat_x = gx + 10;
    /* Use PDF Courier metrics for layout (0.6 em-advance, fixed pitch) so
     * matrix width matches what PDF actually renders. */
    float emsz_x = 0.6f * fsz;
    float paren_x = mat_x - 3 + emsz_x * 0.75f;
    float mat_w = 0;

    for (int i = 0; i < 3; i++) {
        char row[128];
        snprintf(row, sizeof(row), "%+7.3f %+7.3f %+7.3f",
                 app->Gij[i][0] * scale,
                 app->Gij[i][1] * scale,
                 app->Gij[i][2] * scale);
        pdf_text(w, mat_x, gy + i * row_sp, row, fsz, 2);
        if (i == 0) mat_w = (float)strlen(row) * emsz_x;
    }

    /* Parentheses */
    float py0 = gy - 2 + row_sp * 0.125f;
    float py1 = gy + 3 * row_sp - 4 + row_sp * 0.125f;
    float px_l = paren_x;
    float px_r = mat_x + mat_w + 5 + emsz_x * 0.75f;
    float serif = 4;
    pdf_set_stroke_color(w, tc);
    pdf_set_line_width(w, 1.0f);
    pdf_line(w, px_l, py0, px_l, py1);
    pdf_line(w, px_l, py0, px_l + serif, py0);
    pdf_line(w, px_l, py1, px_l + serif, py1);
    pdf_line(w, px_r, py0, px_r, py1);
    pdf_line(w, px_r - serif, py0, px_r, py0);
    pdf_line(w, px_r - serif, py1, px_r, py1);

    /* Units */
    pdf_set_fill_color(w, tc);
    float ux = px_r + 6;
    float uy = gy;
    if (exponent != 0) {
        pdf_text(w, ux, uy, "\xc3\x97 10", fsz, 2);
        float tsz_x = 4 * emsz_x;  /* "× 10" = 4 chars in Courier */
        char expbuf[16];
        snprintf(expbuf, sizeof(expbuf), "%d", exponent);
        pdf_text(w, ux + tsz_x, uy - exp_fsz * 0.4f, expbuf, exp_fsz, 2);
        float esz_x = (float)strlen(expbuf) * 0.6f * exp_fsz;
        ux += tsz_x + esz_x + 2;
    }
    pdf_text(w, ux, uy, " m", fsz, 2);
    float msz_x = 2 * emsz_x;  /* " m" = 2 chars */
    pdf_text(w, ux + msz_x, uy - exp_fsz * 0.4f, "-1", exp_fsz, 2);
}

static void emit_init_overlay(PdfWriter *w, const struct AppState *app,
                              float scene_x, float scene_y,
                              float scene_w, float scene_h)
{
    if (!app->show_init_conditions || app->n_particles <= 0) return;

    InitOverlayLine ovr[24];
    int n = init_overlay_build(app, ovr, 24);
    if (n <= 0) return;

    float iz = app->init_zoom;
    float fsz = 11 * iz;
    float row_sp = 14 * iz;
    float sub_fsz = fsz * 0.75f;
    float sub_dy = fsz * 0.3f;

    /* Width: take the max of screen MeasureTextEx and a Courier-em estimate
     * for mono lines so PDF text (Standard 14 fonts, not the loaded TTFs)
     * doesn't overflow the page edge on the right. */
    float em_courier = 0.6f * fsz;
    float em_sub_courier = 0.6f * sub_fsz;
    float width = 0;
    for (int i = 0; i < n; i++) {
        if (ovr[i].blank) continue;
        Font f = (ovr[i].font_idx == 0) ? app->font_ui : app->font_mono;
        float w_pre_screen = MeasureTextEx(f, ovr[i].pre, fsz, 1).x;
        float w_sub_screen = ovr[i].sub[0]
            ? MeasureTextEx(f, ovr[i].sub, sub_fsz, 1).x : 0;
        float w_suf_screen = ovr[i].suf[0]
            ? MeasureTextEx(f, ovr[i].suf, fsz, 1).x : 0;
        int n_pre = 0, n_sub = 0, n_suf = 0;
        for (const char *p = ovr[i].pre; *p; ) {
            unsigned int cp; int nb = utf8_decode(p, &cp);
            if (nb <= 0) break; n_pre++; p += nb;
        }
        for (const char *p = ovr[i].sub; *p; ) {
            unsigned int cp; int nb = utf8_decode(p, &cp);
            if (nb <= 0) break; n_sub++; p += nb;
        }
        for (const char *p = ovr[i].suf; *p; ) {
            unsigned int cp; int nb = utf8_decode(p, &cp);
            if (nb <= 0) break; n_suf++; p += nb;
        }
        float w_pre_pdf = (ovr[i].font_idx == 2) ? n_pre * em_courier
                                                  : w_pre_screen;
        float w_sub_pdf = (ovr[i].font_idx == 2) ? n_sub * em_sub_courier
                                                  : w_sub_screen;
        float w_suf_pdf = (ovr[i].font_idx == 2) ? n_suf * em_courier
                                                  : w_suf_screen;
        float w_pre = w_pre_screen > w_pre_pdf ? w_pre_screen : w_pre_pdf;
        float w_sub = w_sub_screen > w_sub_pdf ? w_sub_screen : w_sub_pdf;
        float w_suf = w_suf_screen > w_suf_pdf ? w_suf_screen : w_suf_pdf;
        float lw = w_pre + w_sub + w_suf;
        if (lw > width) width = lw;
    }
    float total_h = n * row_sp;

    float ix = scene_x + scene_w - width - 14;
    if (ix < scene_x) ix = scene_x;
    float iy = (scene_y > 10) ? scene_y + 10 : 10;
    float scene_bottom = scene_y + scene_h;
    if (iy + total_h > scene_bottom) {
        iy = scene_bottom - total_h;
        if (iy < 0) iy = 0;
    }

    pdf_set_fill_color(w, app->theme.text_dim);
    for (int i = 0; i < n; i++) {
        if (ovr[i].blank) continue;
        float ly = iy + i * row_sp;
        Font f = (ovr[i].font_idx == 0) ? app->font_ui : app->font_mono;
        pdf_text(w, ix, ly, ovr[i].pre, fsz, ovr[i].font_idx);
        /* Use Courier-correct width for mono so the subscript that follows
         * doesn't overlap the prefix's last glyph (PDF Courier advances
         * slightly more per char than the screen font's measurement). */
        float w_pre = (ovr[i].font_idx == 2)
            ? pdf_mono_width(ovr[i].pre, fsz)
            : MeasureTextEx(f, ovr[i].pre, fsz, 1).x;
        float x_run = ix + w_pre;
        if (ovr[i].sub[0]) {
            pdf_text(w, x_run, ly + sub_dy, ovr[i].sub,
                     sub_fsz, ovr[i].font_idx);
            float w_sub = (ovr[i].font_idx == 2)
                ? pdf_mono_width(ovr[i].sub, sub_fsz)
                : MeasureTextEx(f, ovr[i].sub, sub_fsz, 1).x;
            x_run += w_sub;
        }
        if (ovr[i].suf[0])
            pdf_text(w, x_run, ly, ovr[i].suf, fsz, ovr[i].font_idx);
    }
}

static void emit_overlays(PdfWriter *w, const struct AppState *app)
{
    float sx, sy, sw, sh;
    app_scene_rect(app, &sx, &sy, &sw, &sh);
    emit_gij_overlay(w, app, sx, sy);
    emit_init_overlay(w, app, sx, sy, sw, sh);
}

/* ---- 3D scene emission ---- */

typedef struct {
    Matrix mvp;        /* projection * view */
    Matrix view;
    float page_x, page_y, page_w, page_h;  /* full window for projection */
} Projector;

/* Project a world point. Returns 1 on success (in front of camera).
 * out_sx, out_sy = screen pixel coords; out_vz = view-space z (negative = in front). */
static int project(const Projector *pr, Vec3 p, float *out_sx, float *out_sy, float *out_vz)
{
    float x = (float)p.x, y = (float)p.y, z = (float)p.z;
    /* clip = mvp * (x,y,z,1) */
    const Matrix *m = &pr->mvp;
    float cx = m->m0*x + m->m4*y + m->m8 *z + m->m12;
    float cy = m->m1*x + m->m5*y + m->m9 *z + m->m13;
    float cw = m->m3*x + m->m7*y + m->m11*z + m->m15;
    if (cw < 1e-6f) return 0;
    float nx = cx / cw, ny = cy / cw;
    *out_sx = pr->page_x + (nx + 1.0f) * 0.5f * pr->page_w;
    *out_sy = pr->page_y + (1.0f - ny) * 0.5f * pr->page_h;
    /* view-space z (depth) */
    const Matrix *v = &pr->view;
    *out_vz = v->m2*x + v->m6*y + v->m10*z + v->m14;
    return 1;
}

/* Painter's algorithm item types */
typedef enum {
    PI_POLYLINE,
    PI_POLYLINE_DASHED,
    PI_TRIANGLE,
    PI_QUAD,
    PI_CIRCLE,
} PdfItemKind;

typedef struct {
    PdfItemKind kind;
    float depth;            /* sorting key (more negative = farther; we sort ascending) */
    Color color;
    float lw;
    int n_pts;              /* polyline length */
    int pts_off;            /* index into shared point buffer (xy pairs) */
    float r;                /* circle radius */
    float cx, cy;           /* circle center / triangle/quad first point */
    float qpts[8];          /* triangle/quad: up to 4 points */
    int dash_phase_offset;  /* for dashed polylines, starting phase byte index */
} PdfItem;

typedef struct {
    PdfItem *items;
    int n_items, cap_items;
    float *pts;            /* shared xy buffer for polylines */
    int n_pts, cap_pts;
} ItemList;

static int items_reserve(ItemList *il, int extra_items, int extra_pts)
{
    if (il->n_items + extra_items > il->cap_items) {
        int nc = il->cap_items ? il->cap_items * 2 : 256;
        while (nc < il->n_items + extra_items) nc *= 2;
        PdfItem *p = realloc(il->items, nc * sizeof(PdfItem));
        if (!p) return 0;
        il->items = p; il->cap_items = nc;
    }
    if (il->n_pts + extra_pts > il->cap_pts) {
        int nc = il->cap_pts ? il->cap_pts * 2 : 4096;
        while (nc < il->n_pts + extra_pts) nc *= 2;
        float *p = realloc(il->pts, nc * sizeof(float) * 2);
        if (!p) return 0;
        il->pts = p; il->cap_pts = nc;
    }
    return 1;
}

static int compare_items(const void *a, const void *b)
{
    const PdfItem *ia = a, *ib = b;
    /* depth here = view-space z. Camera looks down -z, so points "in front"
     * have negative z. We want to draw far first (most negative first). */
    if (ia->depth < ib->depth) return -1;
    if (ia->depth > ib->depth) return  1;
    return 0;
}

static void item_polyline(ItemList *il, const Projector *pr,
                          const Vec3 *world_pts, int n,
                          Color c, float lw)
{
    if (n < 2) return;
    /* Project all; if any point has negative w, skip the line (simple). */
    if (!items_reserve(il, 1, n)) return;
    int off = il->n_pts;
    float depth_sum = 0;
    int valid = 0;
    for (int i = 0; i < n; i++) {
        float sx, sy, vz;
        if (!project(pr, world_pts[i], &sx, &sy, &vz)) {
            il->pts[(off + i)*2 + 0] = NAN;
            il->pts[(off + i)*2 + 1] = NAN;
            continue;
        }
        il->pts[(off + i)*2 + 0] = sx;
        il->pts[(off + i)*2 + 1] = sy;
        depth_sum += vz;
        valid++;
    }
    if (valid < 2) return;
    PdfItem *it = &il->items[il->n_items++];
    il->n_pts += n;
    it->kind = PI_POLYLINE;
    it->depth = depth_sum / valid;
    it->color = c;
    it->lw = lw;
    it->n_pts = n;
    it->pts_off = off;
}

/* Polyline whose color/alpha varies along the path. We split into sub-runs of
 * constant alpha (quantized), each becoming its own PI_POLYLINE item with the
 * same depth (so they stay together when sorted). */
static void item_polyline_alpha_fade(ItemList *il, const Projector *pr,
                                     const Vec3 *world_pts, int n,
                                     Color base_color, float lw, float fade,
                                     int n_sections)
{
    if (n < 2 || n_sections < 1) return;
    int seg_size = n / n_sections;
    if (seg_size < 1) seg_size = 1;
    int s_start = 0;
    while (s_start < n - 1) {
        int s_end = s_start + seg_size;
        if (s_end >= n) s_end = n - 1;
        float age = (float)(s_start + s_end) * 0.5f / (float)n;
        float age_fade = 1.0f - age * fade;
        if (age_fade < 0) age_fade = 0;
        Color c = base_color;
        c.a = (unsigned char)(base_color.a * age_fade);
        item_polyline(il, pr, &world_pts[s_start],
                      s_end - s_start + 1, c, lw);
        s_start = s_end;
    }
}

static void item_circle(ItemList *il, const Projector *pr,
                        Vec3 center, float world_radius, Color c)
{
    float sx, sy, vz;
    if (!project(pr, center, &sx, &sy, &vz)) return;
    /* projected radius: r_world * |proj_y_scale| / |w|. Approximate by
     * projecting an offset point and measuring distance. */
    Vec3 off = { center.x + world_radius, center.y, center.z };
    float ox, oy, oz;
    if (!project(pr, off, &ox, &oy, &oz)) return;
    float r = sqrtf((ox - sx)*(ox - sx) + (oy - sy)*(oy - sy));
    if (r < 0.5f) r = 0.5f;
    if (!items_reserve(il, 1, 0)) return;
    PdfItem *it = &il->items[il->n_items++];
    it->kind = PI_CIRCLE;
    it->depth = vz;
    it->color = c;
    it->lw = 0;
    it->n_pts = 0;
    it->pts_off = 0;
    it->cx = sx; it->cy = sy; it->r = r;
}

static void item_arrow(ItemList *il, const Projector *pr,
                       Vec3 from, Vec3 to, Color c, float lw, float head_world)
{
    /* Shaft polyline: from → cone_base */
    Vec3 dir = vec3_sub(to, from);
    double L = vec3_len(dir);
    if (L < 1e-12) return;
    Vec3 dhat = vec3_scale(1.0/L, dir);
    double cone_h = head_world;
    if (cone_h > L) cone_h = L;
    double shaft_len = L - cone_h;
    Vec3 cone_base = vec3_add(from, vec3_scale(shaft_len, dhat));
    Vec3 shaft[2] = { from, cone_base };
    item_polyline(il, pr, shaft, 2, c, lw);

    /* Head as projected triangle: cone_base + perpendicular to dhat in screen */
    float bx, by, bz, tx, ty, tz;
    if (!project(pr, cone_base, &bx, &by, &bz)) return;
    if (!project(pr, to, &tx, &ty, &tz)) return;
    float dx = tx - bx, dy = ty - by;
    float mag = sqrtf(dx*dx + dy*dy);
    if (mag < 1e-3f) return;
    float ux = dx / mag, uy = dy / mag;
    float px = -uy, py = ux;
    float hw = mag * 0.3f;  /* base half-width matches cone_r/cone_h ratio (0.3) */
    if (!items_reserve(il, 1, 0)) return;
    PdfItem *it = &il->items[il->n_items++];
    it->kind = PI_TRIANGLE;
    it->depth = (bz + tz) * 0.5f;
    it->color = c;
    it->lw = 0;
    it->n_pts = 3;
    it->pts_off = 0;
    it->qpts[0] = tx;        it->qpts[1] = ty;
    it->qpts[2] = bx + px*hw; it->qpts[3] = by + py*hw;
    it->qpts[4] = bx - px*hw; it->qpts[5] = by - py*hw;
}

static void emit_items(PdfWriter *w, ItemList *il)
{
    qsort(il->items, il->n_items, sizeof(PdfItem), compare_items);
    Color last_color = (Color){0,0,0,0};
    int color_set = 0, fill_set = 0;
    float last_lw = -1.0f;
    Color last_fill = (Color){0,0,0,0};

    for (int i = 0; i < il->n_items; i++) {
        PdfItem *it = &il->items[i];
        switch (it->kind) {
        case PI_POLYLINE: {
            if (!color_set ||
                last_color.r != it->color.r || last_color.g != it->color.g ||
                last_color.b != it->color.b || last_color.a != it->color.a) {
                pdf_set_stroke_color(w, it->color);
                last_color = it->color; color_set = 1;
            }
            if (last_lw != it->lw) {
                pdf_set_line_width(w, it->lw);
                last_lw = it->lw;
            }
            const float *xy = &il->pts[it->pts_off * 2];
            int started = 0;
            for (int j = 0; j < it->n_pts; j++) {
                float x = xy[2*j], y = xy[2*j+1];
                if (isnan(x) || isnan(y)) {
                    if (started) cs_append(w, "S\n", 2);
                    started = 0;
                    continue;
                }
                if (!started) {
                    cs_printf(w, "%.4f %.4f m\n", x, PDFY(w, y));
                    started = 1;
                } else {
                    cs_printf(w, "%.4f %.4f l\n", x, PDFY(w, y));
                }
            }
            if (started) cs_append(w, "S\n", 2);
            break;
        }
        case PI_CIRCLE:
            if (!fill_set ||
                last_fill.r != it->color.r || last_fill.g != it->color.g ||
                last_fill.b != it->color.b || last_fill.a != it->color.a) {
                pdf_set_fill_color(w, it->color);
                last_fill = it->color; fill_set = 1;
            }
            pdf_circle_fill(w, it->cx, it->cy, it->r);
            break;
        case PI_TRIANGLE:
            if (!fill_set ||
                last_fill.r != it->color.r || last_fill.g != it->color.g ||
                last_fill.b != it->color.b || last_fill.a != it->color.a) {
                pdf_set_fill_color(w, it->color);
                last_fill = it->color; fill_set = 1;
            }
            pdf_triangle_fill(w,
                it->qpts[0], it->qpts[1],
                it->qpts[2], it->qpts[3],
                it->qpts[4], it->qpts[5]);
            break;
        case PI_QUAD:
            if (!fill_set ||
                last_fill.r != it->color.r || last_fill.g != it->color.g ||
                last_fill.b != it->color.b || last_fill.a != it->color.a) {
                pdf_set_fill_color(w, it->color);
                last_fill = it->color; fill_set = 1;
            }
            pdf_quad_fill(w,
                it->qpts[0], it->qpts[1],
                it->qpts[2], it->qpts[3],
                it->qpts[4], it->qpts[5],
                it->qpts[6], it->qpts[7]);
            break;
        default: break;
        }
    }
}

static void emit_3d_scene(PdfWriter *w, const struct AppState *app)
{
    float sx, sy, sw, sh;
    app_scene_rect(app, &sx, &sy, &sw, &sh);

    /* The on-screen 3D view sets a custom rlViewport for the scene rect and
     * projects with aspect = scene_w / scene_h (see the non-stereo branch in
     * app_render_inner). Mirror that here so the PDF perspective matches the
     * window exactly: project NDC into the scene rect's screen-pixel range. */
    Projector pr;
    pr.page_x = sx;
    pr.page_y = sy;
    pr.page_w = sw;
    pr.page_h = sh;

    pr.view = MatrixLookAt(app->camera.position, app->camera.target,
                           app->camera.up);
    float aspect = sw / sh;
    Matrix proj;
    if (app->camera.projection == CAMERA_ORTHOGRAPHIC) {
        Vector3 dv = { app->camera.position.x - app->camera.target.x,
                       app->camera.position.y - app->camera.target.y,
                       app->camera.position.z - app->camera.target.z };
        double cd = sqrt(dv.x*dv.x + dv.y*dv.y + dv.z*dv.z);
        double otop = cd * tan(app->camera.fovy * 0.5 * DEG2RAD);
        double oright = otop * aspect;
        proj = MatrixOrtho(-oright, oright, -otop, otop, 0.01, 1000.0);
    } else {
        proj = MatrixPerspective(app->camera.fovy * DEG2RAD, aspect,
                                 0.01, 1000.0);
    }
    pr.mvp = MatrixMultiply(pr.view, proj);

    pdf_save(w);
    pdf_clip_rect(w, sx, sy, sw, sh);

    /* Background fill for the scene region */
    pdf_set_fill_color(w, app->theme.bg);
    pdf_rect_fill(w, sx, sy, sw, sh);

    ItemList il = {0};
    const FieldModel *fm = &app->models[app->current_model];

    /* Coordinate axes */
    if (app->axis_scale > 1e-6f) {
        float s = app->axis_scale;
        Color ac = app->color_axes;
        Color ax = {(unsigned char)fminf(ac.r * 1.5f, 255),
                    (unsigned char)(ac.g * 0.4f),
                    (unsigned char)(ac.b * 0.4f), ac.a};
        Color ay = {(unsigned char)(ac.r * 0.4f),
                    (unsigned char)fminf(ac.g * 1.5f, 255),
                    (unsigned char)(ac.b * 0.4f), ac.a};
        Color az = {(unsigned char)(ac.r * 0.4f),
                    (unsigned char)(ac.g * 0.4f),
                    (unsigned char)fminf(ac.b * 1.5f, 255), ac.a};
        Vec3 o = {0,0,0};
        Vec3 lx[2] = {o, {s,0,0}};
        Vec3 ly[2] = {o, {0,s,0}};
        Vec3 lz[2] = {o, {0,0,s}};
        item_polyline(&il, &pr, lx, 2, ax, (float)app->lw_axes);
        item_polyline(&il, &pr, ly, 2, ay, (float)app->lw_axes);
        item_polyline(&il, &pr, lz, 2, az, (float)app->lw_axes);
    }

    /* Field lines (if cache is current — rebuilt by render3d when shown) */
    if (app->show_field_lines) {
        int nl = render3d_field_line_count();
        Color flc = app->color_field_lines;
        for (int li = 0; li < nl; li++) {
            int npts;
            const Vec3 *pts = render3d_field_line_get(li, &npts);
            if (!pts || npts < 2) continue;
            /* Subsample to keep count manageable in PDF */
            int step = (npts > 1024) ? npts / 1024 : 1;
            Vec3 sub[1100];
            int kk = 0;
            for (int i = 0; i < npts && kk < 1100; i += step) sub[kk++] = pts[i];
            if (kk > 0 && (kk-1)*step < npts-1) {
                if (kk < 1100) sub[kk++] = pts[npts-1];
            }
            item_polyline(&il, &pr, sub, kk, flc, (float)app->lw_field_lines);
        }
    }

    /* GC field line through selected particle */
    if (app->show_gc_field_line && app->n_particles > 0) {
        int si = app->selected_particle;
        Vec3 pos = app->particles[si].pos;
        Vec3 vel = app->particles[si].vel;
        Vec3 B = fm->eval_B(fm->params, pos);
        double Bm = vec3_len(B);
        if (Bm > 1e-30) {
            double B2 = Bm * Bm;
            double mq = app->particles[si].m / app->particles[si].q;
            Vec3 vxB = vec3_cross(vel, B);
            Vec3 gc = vec3_add(pos, vec3_scale(mq / B2, vxB));
            double ds = 0.02;
            double half = app->gc_fl_length / 2.0;
            int max_pts = 1024;
            Vec3 *fwd = malloc(sizeof(Vec3) * max_pts);
            Vec3 *bwd = malloc(sizeof(Vec3) * max_pts);
            int nf = 0, nb = 0;
            if (fwd && bwd) {
                Vec3 p = gc;
                fwd[nf++] = p;
                double arc = 0.0;
                while (arc < half && nf < max_pts) {
                    Vec3 BB = fm->eval_B(fm->params, p);
                    double mm = vec3_len(BB);
                    if (mm < 1e-15) break;
                    Vec3 bh = vec3_scale(1.0/mm, BB);
                    p = vec3_add(p, vec3_scale(ds, bh));
                    fwd[nf++] = p; arc += ds;
                    if (vec3_len(p) > 100.0) break;
                }
                p = gc;
                bwd[nb++] = p; arc = 0.0;
                while (arc < half && nb < max_pts) {
                    Vec3 BB = fm->eval_B(fm->params, p);
                    double mm = vec3_len(BB);
                    if (mm < 1e-15) break;
                    Vec3 bh = vec3_scale(1.0/mm, BB);
                    p = vec3_sub(p, vec3_scale(ds, bh));
                    bwd[nb++] = p; arc += ds;
                    if (vec3_len(p) > 100.0) break;
                }
                Color gcc = app->color_gc_fl;
                item_polyline(&il, &pr, fwd, nf, gcc, (float)app->lw_gc_fl);
                item_polyline(&il, &pr, bwd, nb, gcc, (float)app->lw_gc_fl);
            }
            free(fwd); free(bwd);
        }
    }

    /* Position field line */
    if (app->show_pos_field_line && app->n_particles > 0) {
        int si = app->selected_particle;
        Vec3 pos = app->particles[si].pos;
        double ds = 0.02;
        double half = app->gc_fl_length / 2.0;
        int max_pts = 1024;
        Vec3 *fwd = malloc(sizeof(Vec3) * max_pts);
        Vec3 *bwd = malloc(sizeof(Vec3) * max_pts);
        int nf = 0, nb = 0;
        if (fwd && bwd) {
            Vec3 p = pos;
            fwd[nf++] = p;
            double arc = 0.0;
            while (arc < half && nf < max_pts) {
                Vec3 BB = fm->eval_B(fm->params, p);
                double mm = vec3_len(BB);
                if (mm < 1e-15) break;
                Vec3 bh = vec3_scale(1.0/mm, BB);
                p = vec3_add(p, vec3_scale(ds, bh));
                fwd[nf++] = p; arc += ds;
                if (vec3_len(p) > 100.0) break;
            }
            p = pos; bwd[nb++] = p; arc = 0.0;
            while (arc < half && nb < max_pts) {
                Vec3 BB = fm->eval_B(fm->params, p);
                double mm = vec3_len(BB);
                if (mm < 1e-15) break;
                Vec3 bh = vec3_scale(1.0/mm, BB);
                p = vec3_sub(p, vec3_scale(ds, bh));
                bwd[nb++] = p; arc += ds;
                if (vec3_len(p) > 100.0) break;
            }
            Color pc = app->color_pos_fl;
            item_polyline(&il, &pr, fwd, nf, pc, (float)app->lw_pos_fl);
            item_polyline(&il, &pr, bwd, nb, pc, (float)app->lw_pos_fl);
        }
        free(fwd); free(bwd);
    }

    /* Trails */
    int trail_table[] = {7680, 76800, 768000};
    int pr_idx = app->plot_range; if (pr_idx < 0) pr_idx = 0; if (pr_idx > 2) pr_idx = 2;
    int max_trail = trail_table[pr_idx];
    float fade = app->trail_fade;

    Vec3 *tbuf = malloc(sizeof(Vec3) * 12000);
    if (tbuf) {
        for (int pi = 0; pi < app->n_particles; pi++) {
            int sp = app->particles[pi].species;
            if (sp < 0 || sp >= NUM_SPECIES) sp = NUM_SPECIES - 1;
            Color tc = app->species_colors[sp];

            /* Orbit trails */
            if (app->show_trail) {
                int tcount = app->trails[pi].count < max_trail
                             ? app->trails[pi].count : max_trail;
                if (tcount >= 2) {
                    int step = (tcount > 10000) ? tcount / 10000 : 1;
                    int kk = 0;
                    for (int i = 0; i < tcount && kk < 12000; i += step) {
                        Vec3 p; trail_get(&app->trails[pi], i, &p);
                        tbuf[kk++] = p;
                    }
                    /* Reverse so oldest is first (visual order matches rendering). */
                    for (int a = 0, b = kk - 1; a < b; a++, b--) {
                        Vec3 t = tbuf[a]; tbuf[a] = tbuf[b]; tbuf[b] = t;
                    }
                    item_polyline_alpha_fade(&il, &pr, tbuf, kk,
                                             tc, (float)app->trail_thickness,
                                             fade, 32);
                }
            }

            /* GC trails */
            if (app->show_gc_trajectory) {
                int gc_cnt = app->gc_trails[pi].count < max_trail
                             ? app->gc_trails[pi].count : max_trail;
                if (gc_cnt >= 2) {
                    int step = (gc_cnt > 10000) ? gc_cnt / 10000 : 1;
                    int kk = 0;
                    for (int i = 0; i < gc_cnt && kk < 12000; i += step) {
                        Vec3 p; trail_get(&app->gc_trails[pi], i, &p);
                        tbuf[kk++] = p;
                    }
                    for (int a = 0, b = kk - 1; a < b; a++, b--) {
                        Vec3 t = tbuf[a]; tbuf[a] = tbuf[b]; tbuf[b] = t;
                    }
                    Color gcc = tc;
                    gcc.r = (unsigned char)(gcc.r * 0.55f);
                    gcc.g = (unsigned char)(gcc.g * 0.55f);
                    gcc.b = (unsigned char)(gcc.b * 0.55f);
                    item_polyline_alpha_fade(&il, &pr, tbuf, kk,
                                             gcc, (float)app->trail_thickness,
                                             fade, 32);
                    /* GC sphere */
                    item_circle(&il, &pr, app->gc_particles[pi].pos, 0.003f, gcc);
                }
            }
        }
        free(tbuf);
    }

    /* Particle spheres */
    for (int pi = 0; pi < app->n_particles; pi++) {
        int sp = app->particles[pi].species;
        if (sp < 0 || sp >= NUM_SPECIES) sp = NUM_SPECIES - 1;
        Color pc = app->species_colors[sp];
        float r = (pi == app->selected_particle) ? 0.006f : 0.004f;
        item_circle(&il, &pr, app->particles[pi].pos, r, pc);
    }

    /* Arrows for selected particle */
    {
        int si = app->selected_particle;
        if (si >= 0 && si < app->n_particles) {
            Vec3 pos = app->particles[si].pos;
            Vec3 vel = app->particles[si].vel;
            double v_mag = vec3_len(vel);
            double unit_len = 0.06;
            if (app->show_velocity_vec && v_mag > 1e-30) {
                double L = app->vec_scaled
                    ? v_mag * pow(10.0, (double)app->vec_scale_v) : unit_len;
                Vec3 dh = vec3_scale(1.0/v_mag, vel);
                Vec3 tip = vec3_add(pos, vec3_scale(L, dh));
                item_arrow(&il, &pr, pos, tip, app->color_vel,
                           (float)app->lw_arrows, app->arrow_head_size);
            }
            if (app->show_B_vec || app->show_F_vec) {
                Vec3 B = fm->eval_B(fm->params, pos);
                double Bm = vec3_len(B);
                if (app->show_B_vec && Bm > 1e-30) {
                    double L = app->vec_scaled
                        ? Bm * pow(10.0, (double)app->vec_scale_B) : unit_len;
                    Vec3 dh = vec3_scale(1.0/Bm, B);
                    Vec3 tip = vec3_add(pos, vec3_scale(L, dh));
                    item_arrow(&il, &pr, pos, tip, app->color_B,
                               (float)app->lw_arrows, app->arrow_head_size);
                }
                if (app->show_F_vec && Bm > 1e-30 && v_mag > 1e-30) {
                    double q = app->particles[si].q;
                    Vec3 F = vec3_scale(q, vec3_cross(vel, B));
                    double Fm = vec3_len(F);
                    if (Fm > 1e-30) {
                        double L = app->vec_scaled
                            ? Fm * pow(10.0, (double)app->vec_scale_F) : unit_len;
                        Vec3 dh = vec3_scale(1.0/Fm, F);
                        Vec3 tip = vec3_add(pos, vec3_scale(L, dh));
                        item_arrow(&il, &pr, pos, tip, app->color_F,
                                   (float)app->lw_arrows, app->arrow_head_size);
                    }
                }
            }
            if (app->cam_field_aligned && app->frame_e1_init) {
                double L = 0.06;
                Vec3 e1tip = vec3_add(pos, vec3_scale(L, app->frame_e1));
                Vec3 e2tip = vec3_add(pos, vec3_scale(L, app->frame_e2));
                item_arrow(&il, &pr, pos, e1tip, app->color_kappa,
                           (float)app->lw_triad, app->arrow_head_size);
                item_arrow(&il, &pr, pos, e2tip, app->color_binormal,
                           (float)app->lw_triad, app->arrow_head_size);
            }
        }
    }

    emit_items(w, &il);
    free(il.items);
    free(il.pts);

    pdf_restore(w);
}

/* ---- Finalize ---- */

static int pdf_end(PdfWriter *w)
{
    /* Reserve all object numbers */
    w->catalog_obj  = pdf_next_obj(w);
    w->pages_obj    = pdf_next_obj(w);
    w->page_obj     = pdf_next_obj(w);
    w->content_obj  = pdf_next_obj(w);
    w->font_helv    = pdf_next_obj(w);
    w->font_helv_b  = pdf_next_obj(w);
    w->font_cour    = pdf_next_obj(w);
    w->font_sym     = pdf_next_obj(w);
    w->gs_opaque_obj = pdf_next_obj(w);
    for (int i = 0; i < w->n_alphas; i++) {
        int n = pdf_next_obj(w);
        if (n < 0) { w->n_alphas = i; break; }
        w->gs_objs[i] = n;
    }

    /* Catalog */
    pdf_begin_obj(w, w->catalog_obj);
    pdf_writef(w, "<< /Type /Catalog /Pages %d 0 R >>\n", w->pages_obj);
    pdf_end_obj(w);

    /* Pages */
    pdf_begin_obj(w, w->pages_obj);
    pdf_writef(w, "<< /Type /Pages /Count 1 /Kids [%d 0 R] >>\n", w->page_obj);
    pdf_end_obj(w);

    /* Page */
    pdf_begin_obj(w, w->page_obj);
    pdf_writef(w, "<< /Type /Page /Parent %d 0 R /MediaBox [0 0 %.4f %.4f] "
                  "/Contents %d 0 R /Resources << "
                  "/Font << /F0 %d 0 R /F1 %d 0 R /F2 %d 0 R /F3 %d 0 R >> "
                  "/ExtGState << /GSO %d 0 R",
                  w->pages_obj, w->page_w, w->page_h, w->content_obj,
                  w->font_helv, w->font_helv_b, w->font_cour, w->font_sym,
                  w->gs_opaque_obj);
    for (int i = 0; i < w->n_alphas; i++)
        pdf_writef(w, " /GS%d %d 0 R", i, w->gs_objs[i]);
    pdf_writef(w, " >> >> >>\n");
    pdf_end_obj(w);

    /* Content stream */
    pdf_begin_obj(w, w->content_obj);
    pdf_writef(w, "<< /Length %zu >>\nstream\n", w->cs_len);
    if (w->cs_len > 0) {
        fwrite(w->cs, 1, w->cs_len, w->fp);
        w->bytes += w->cs_len;
    }
    pdf_writef(w, "\nendstream\n");
    pdf_end_obj(w);

    /* Fonts */
    const char *names[4] = {"Helvetica", "Helvetica-Bold", "Courier", "Symbol"};
    int objs[4] = {w->font_helv, w->font_helv_b, w->font_cour, w->font_sym};
    for (int i = 0; i < 4; i++) {
        pdf_begin_obj(w, objs[i]);
        if (i == 3) {
            /* Symbol uses its own built-in encoding (StandardEncoding mapping) */
            pdf_writef(w, "<< /Type /Font /Subtype /Type1 /BaseFont /%s >>\n",
                       names[i]);
        } else {
            pdf_writef(w, "<< /Type /Font /Subtype /Type1 /BaseFont /%s "
                          "/Encoding /WinAnsiEncoding >>\n", names[i]);
        }
        pdf_end_obj(w);
    }

    /* ExtGState: opaque */
    pdf_begin_obj(w, w->gs_opaque_obj);
    pdf_writef(w, "<< /Type /ExtGState /CA 1 /ca 1 >>\n");
    pdf_end_obj(w);

    /* ExtGState: per alpha */
    for (int i = 0; i < w->n_alphas; i++) {
        pdf_begin_obj(w, w->gs_objs[i]);
        double a = w->alphas_q[i] / 255.0;
        pdf_writef(w, "<< /Type /ExtGState /CA %.4f /ca %.4f >>\n", a, a);
        pdf_end_obj(w);
    }

    /* xref */
    long xref_off = w->bytes;
    pdf_writef(w, "xref\n0 %d\n", w->n_objects + 1);
    pdf_writef(w, "0000000000 65535 f \n");
    for (int i = 1; i <= w->n_objects; i++)
        pdf_writef(w, "%010ld 00000 n \n", w->obj_off[i]);

    /* Trailer */
    pdf_writef(w,
        "trailer\n<< /Size %d /Root %d 0 R >>\nstartxref\n%ld\n%%%%EOF\n",
        w->n_objects + 1, w->catalog_obj, xref_off);

    fclose(w->fp);
    free(w->cs);
    free(w);
    return 0;
}

/* ---- Public entry point ---- */

int pdf_export(const struct AppState *app, const char *path)
{
    if (!app || !path) return -1;

    /* PDF requires '.' as decimal separator regardless of locale. */
    char *prev_locale = setlocale(LC_NUMERIC, NULL);
    char saved[64] = {0};
    if (prev_locale) {
        strncpy(saved, prev_locale, sizeof(saved) - 1);
    }
    setlocale(LC_NUMERIC, "C");

    /* Page = visible area only (excludes UI panel column). The 3D scene
     * remains projected with the full-window aspect ratio that raylib uses
     * on screen, but the page is trimmed to scene_w wide so the UI-panel
     * column doesn't appear as empty white space. A CTM translation at the
     * start of the content stream shifts emit-time screen coordinates
     * (which span [0, win_w]) into the trimmed page space [0, scene_w]. */
    float sx, sy, sw, sh;
    app_scene_rect(app, &sx, &sy, &sw, &sh);
    (void)sy; (void)sh;
    float page_w = sw;
    float page_h = (float)app->win_h;

    PdfWriter *w = pdf_begin(path, page_w, page_h);
    if (!w) {
        if (saved[0]) setlocale(LC_NUMERIC, saved);
        return -1;
    }

    /* Translate so the visible scene's left edge lands at PDF X = 0.
     * Anything to the left of scene_x (i.e. the UI panel area) is shifted
     * outside the page and gets cropped naturally. */
    cs_printf(w, "1 0 0 1 %.4f 0 cm\n", -sx);

    /* Use round line joins and caps so polylines look smooth */
    cs_append(w, "1 J 1 j\n", 8);

    /* Background fill covering the visible window region (incl. plot strip) */
    pdf_set_fill_color(w, app->theme.bg);
    pdf_rect_fill(w, sx, 0, sw, page_h);

    emit_3d_scene(w, app);
    emit_overlays(w, app);
    emit_plot_strip(w, app);

    int rc = pdf_end(w);

    if (saved[0]) setlocale(LC_NUMERIC, saved);
    return rc;
}
