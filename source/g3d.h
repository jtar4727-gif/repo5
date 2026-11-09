/* Renderizador 3D por software (projecao na CPU + triangulos do citro2d).
   Usa os mesmos cômodos/medidas do jogo de PC (world3d.h, gerado do c_3d.js). */
#pragma once
#include <math.h>
#include <stdlib.h>
#include <string.h>
typedef struct { float x, y, z, w, h, d; unsigned c; unsigned char e; } Bx;
typedef struct { float x, z; int to; float ax, az; } Dr;
typedef struct { const Bx *b; int n; float w, d; float cam[6]; float amb; unsigned wall, floor, sky; const float (*cols)[4]; int ncols; const Dr *dr; int ndr; float sx, sz; } Rm;
typedef struct { unsigned skin, hair, top, leg, shoe; float wd, hh; int big, coat, lng; float s; } Cs;
#include "world3d.h"

#ifndef G3_TRI
#define G3_TRI(x0,y0,x1,y1,x2,y2,rgb) C2D_DrawTriangle((x0),(y0),HEX(rgb),(x1),(y1),HEX(rgb),(x2),(y2),HEX(rgb),0)
#endif

typedef struct { float c[3], h[3], R[9]; unsigned col; unsigned char e; float key; } DB;
#define G3_MAXB 1100
static DB g3q[G3_MAXB]; static int g3n;
static float g3Eye = 0;
static float g3E[3], g3Rt[3], g3Up[3], g3F[3], g3Cx = 200, g3Cy = 120, g3Fl = 269.5f, g3Amb = .6f;
static int g3Gest = 0, g3Hold = 0; static float g3GT = 0;
static int g3Talk = 0; static float g3Look = 0;
static int g3Miss = 0; /* 1 perna esq, 2 braco esq, 4 perna dir, 8 cabeca */
static unsigned g3Fog = 0x000000; static const float *g3Ov = 0;

static void g3_cam(const float *c) {
  float e[3] = { c[0], c[1], c[2] }, f[3] = { c[3] - c[0], c[4] - c[1], c[5] - c[2] };
  float l = sqrtf(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]); for (int i = 0; i < 3; i++) f[i] /= l;
  float r[3] = { f[1]*0 - f[2]*1, f[2]*0 - f[0]*0, f[0]*1 - f[1]*0 }; /* f x up(0,1,0) */
  l = sqrtf(r[0]*r[0] + r[2]*r[2]); r[0] /= l; r[2] /= l;
  float u[3] = { r[1]*f[2] - r[2]*f[1], r[2]*f[0] - r[0]*f[2], r[0]*f[1] - r[1]*f[0] };
  e[0] += r[0] * g3Eye; e[2] += r[2] * g3Eye;
  memcpy(g3E, e, sizeof e); memcpy(g3Rt, r, sizeof r); memcpy(g3Up, u, sizeof u); memcpy(g3F, f, sizeof f);
}
static void g3_add(float x, float y, float z, float w, float h, float d, const float *R, unsigned col, int e, float forcekey) {
  if (g3n >= G3_MAXB) return;
  DB *b = &g3q[g3n++]; b->c[0] = x; b->c[1] = y; b->c[2] = z; b->h[0] = w * .5f; b->h[1] = h * .5f; b->h[2] = d * .5f;
  if (R) memcpy(b->R, R, sizeof b->R); else { memset(b->R, 0, sizeof b->R); b->R[0] = b->R[4] = b->R[8] = 1; }
  b->col = col; b->e = e;
  float dx = x - g3E[0], dy = y - g3E[1], dz = z - g3E[2];
  b->key = forcekey > 0 ? forcekey : dx*dx + dy*dy + dz*dz;
}
static int g3_cmp(const void *a, const void *b) { float x = ((const DB *)a)->key, y = ((const DB *)b)->key; return x < y ? 1 : x > y ? -1 : 0; }
static float g3Lp[3] = { 0, 2.5f, 0 }, g3Lk = .45f, g3T = 0; static int g3Flick = 0;
typedef struct { float x, y, z, r, g, b; } CV; /* vertice no espaco da camera, com cor */
static void g3_vl(const float *p, const float *n, unsigned base, int e, float *o) {
  float k;
  if (e) k = 1.0f;
  else {
    static const float L[3] = { -.35f, .82f, .45f };
    float diff = .5f * fmaxf(0, n[0]*L[0] + n[1]*L[1] + n[2]*L[2]);
    float dx = g3Lp[0] - p[0], dy = g3Lp[1] - p[1], dz = g3Lp[2] - p[2], d2 = dx*dx + dy*dy + dz*dz, dl = sqrtf(d2) + .001f;
    float lam = fmaxf(.3f, (n[0]*dx + n[1]*dy + n[2]*dz) / dl * .6f + .5f);
    float pl = g3Lk * lam / (1 + .09f * d2); if (g3Flick) pl *= .8f + .2f * sinf(g3T * 17) * sinf(g3T * 5.3f);
    float ao = .72f + .28f * fminf(1.0f, p[1] / 2.2f);
    k = (g3Amb + diff) * ao + pl;
  }
  if (k > 1.45f) k = 1.45f;
  float vx = p[0] - g3E[0], vy = p[1] - g3E[1], vz = p[2] - g3E[2], dist = sqrtf(vx*vx + vy*vy + vz*vz);
  float fg = dist / 24.0f; if (fg > .5f) fg = .5f;
  float fr = ((g3Fog >> 16) & 255), fgc = ((g3Fog >> 8) & 255), fb = (g3Fog & 255);
  o[0] = ((base >> 16) & 255) * k * (1 - fg) + fr * fg; o[1] = ((base >> 8) & 255) * k * (1 - fg) + fgc * fg; o[2] = (base & 255) * k * (1 - fg) + fb * fg;
  for (int i = 0; i < 3; i++) { if (o[i] > 255) o[i] = 255; if (o[i] < 0) o[i] = 0; }
}
#ifndef G3_TRI3
#define G3_TRI3(x0,y0,r0,g0,b0,x1,y1,r1,g1,b1,x2,y2,r2,g2,b2) C2D_DrawTriangle((x0),(y0),C2D_Color32((int)(r0),(int)(g0),(int)(b0),255),(x1),(y1),C2D_Color32((int)(r1),(int)(g1),(int)(b1),255),(x2),(y2),C2D_Color32((int)(r2),(int)(g2),(int)(b2),255),0)
#endif
static float g3Shift = 0; /* deslocamento horizontal em pixels (3D estereoscopico) */
static void g3_cell(const float (*p)[3], const float (*c)[3]) {
  CV q[8]; int m = 0; const float nz = .15f; CV v[4];
  for (int i = 0; i < 4; i++) {
    float d[3] = { p[i][0] - g3E[0], p[i][1] - g3E[1], p[i][2] - g3E[2] };
    v[i].x = d[0]*g3Rt[0] + d[1]*g3Rt[1] + d[2]*g3Rt[2]; v[i].y = d[0]*g3Up[0] + d[1]*g3Up[1] + d[2]*g3Up[2]; v[i].z = d[0]*g3F[0] + d[1]*g3F[1] + d[2]*g3F[2];
    v[i].r = c[i][0]; v[i].g = c[i][1]; v[i].b = c[i][2];
  }
  for (int i = 0; i < 4; i++) {
    int j = (i + 1) & 3; int ai = v[i].z > nz, aj = v[j].z > nz;
    if (ai) q[m++] = v[i];
    if (ai != aj) { float t = (nz - v[i].z) / (v[j].z - v[i].z); CV o; o.x = v[i].x + (v[j].x - v[i].x) * t; o.y = v[i].y + (v[j].y - v[i].y) * t; o.z = nz; o.r = v[i].r + (v[j].r - v[i].r) * t; o.g = v[i].g + (v[j].g - v[i].g) * t; o.b = v[i].b + (v[j].b - v[i].b) * t; q[m++] = o; }
  }
  if (m < 3) return;
  float sx[8], sy[8];
  for (int i = 0; i < m; i++) { sx[i] = g3Cx + g3Shift + q[i].x / q[i].z * g3Fl; sy[i] = g3Cy - q[i].y / q[i].z * g3Fl; }
  for (int i = 1; i + 1 < m; i++) G3_TRI3(sx[0], sy[0], q[0].r, q[0].g, q[0].b, sx[i], sy[i], q[i].r, q[i].g, q[i].b, sx[i+1], sy[i+1], q[i+1].r, q[i+1].g, q[i+1].b);
}
static void g3_face(const float (*P)[3], const float *n, unsigned col, int e) {
  /* P: 4 cantos em ordem; subdivide faces grandes para a luz ficar suave */
  float e1[3] = { P[1][0]-P[0][0], P[1][1]-P[0][1], P[1][2]-P[0][2] }, e2[3] = { P[3][0]-P[0][0], P[3][1]-P[0][1], P[3][2]-P[0][2] };
  float l1 = sqrtf(e1[0]*e1[0] + e1[1]*e1[1] + e1[2]*e1[2]), l2 = sqrtf(e2[0]*e2[0] + e2[1]*e2[1] + e2[2]*e2[2]);
  int n1 = (int)(l1 / 2.4f) + 1, n2 = (int)(l2 / 2.4f) + 1; if (n1 > 4) n1 = 4; if (n2 > 4) n2 = 4; if (e) n1 = n2 = 1;
  float G[5][5][3], GC[5][5][3];
  for (int i = 0; i <= n1; i++) for (int j = 0; j <= n2; j++) {
    float u = (float)i / n1, w = (float)j / n2;
    for (int k = 0; k < 3; k++) G[i][j][k] = P[0][k] + e1[k] * u + e2[k] * w;
    g3_vl(G[i][j], n, col, e, GC[i][j]);
  }
  for (int i = 0; i < n1; i++) for (int j = 0; j < n2; j++) {
    float cp[4][3], cc[4][3]; int ii[4] = { i, i + 1, i + 1, i }, jj[4] = { j, j, j + 1, j + 1 };
    for (int k = 0; k < 4; k++) { memcpy(cp[k], G[ii[k]][jj[k]], sizeof cp[k]); memcpy(cc[k], GC[ii[k]][jj[k]], sizeof cc[k]); }
    g3_cell(cp, cc);
  }
}
static void g3_draw(void) {
  qsort(g3q, g3n, sizeof(DB), g3_cmp);
  static const int F[6][4] = { {1,3,7,5}, {0,4,6,2}, {2,6,7,3}, {0,1,5,4}, {4,5,7,6}, {0,2,3,1} };
  static const float N[6][3] = { {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1} };
  for (int bi = 0; bi < g3n; bi++) {
    DB *b = &g3q[bi]; float P[8][3];
    for (int k = 0; k < 8; k++) {
      float lx = (k & 1 ? 1 : -1) * b->h[0], ly = (k & 2 ? 1 : -1) * b->h[1], lz = (k & 4 ? 1 : -1) * b->h[2];
      P[k][0] = b->c[0] + b->R[0]*lx + b->R[1]*ly + b->R[2]*lz;
      P[k][1] = b->c[1] + b->R[3]*lx + b->R[4]*ly + b->R[5]*lz;
      P[k][2] = b->c[2] + b->R[6]*lx + b->R[7]*ly + b->R[8]*lz;
    }
    for (int f = 0; f < 6; f++) {
      float nx = b->R[0]*N[f][0] + b->R[1]*N[f][1] + b->R[2]*N[f][2];
      float ny = b->R[3]*N[f][0] + b->R[4]*N[f][1] + b->R[5]*N[f][2];
      float nz = b->R[6]*N[f][0] + b->R[7]*N[f][1] + b->R[8]*N[f][2];
      float hm = (b->h[0] + b->h[1] + b->h[2]) / 3;
      float fx = b->c[0] + nx*hm - g3E[0], fy = b->c[1] + ny*hm - g3E[1], fz = b->c[2] + nz*hm - g3E[2];
      if (nx*fx + ny*fy + nz*fz >= 0) continue;
      float q[4][3], nn[3] = { nx, ny, nz }; for (int i = 0; i < 4; i++) memcpy(q[i], P[F[f][i]], sizeof q[i]);
      g3_face(q, nn, b->col, b->e);
    }
  }
}
/* ---- personagens (mesma estrutura do mkChar do PC) ---- */
static void g3_mul(const float *a, const float *b, float *o) { for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) o[i*3+j] = a[i*3]*b[j] + a[i*3+1]*b[3+j] + a[i*3+2]*b[6+j]; }
static void g3_rx(float a, float *R) { float c = cosf(a), s = sinf(a); R[0]=1;R[1]=0;R[2]=0;R[3]=0;R[4]=c;R[5]=-s;R[6]=0;R[7]=s;R[8]=c; }
static void g3_ry(float a, float *R) { float c = cosf(a), s = sinf(a); R[0]=c;R[1]=0;R[2]=s;R[3]=0;R[4]=1;R[5]=0;R[6]=-s;R[7]=0;R[8]=c; }
static void g3_rz(float a, float *R) { float c = cosf(a), s = sinf(a); R[0]=c;R[1]=-s;R[2]=0;R[3]=s;R[4]=c;R[5]=0;R[6]=0;R[7]=0;R[8]=1; }
static void g3_mv(const float *R, const float *v, float *o) { o[0] = R[0]*v[0]+R[1]*v[1]+R[2]*v[2]; o[1] = R[3]*v[0]+R[4]*v[1]+R[5]*v[2]; o[2] = R[6]*v[0]+R[7]*v[1]+R[8]*v[2]; }
/* peca: pivo local (no tronco), rotacao do membro, offset dentro do membro, tamanho */
static void g3_part(const float *W, const float *root, const float *pv, const float *Rl, const float *off, float w, float h, float d, unsigned col) {
  float t[3], o[3], R[9], p[3] = { off[0], off[1], off[2] };
  g3_mv(Rl, p, t); t[0] += pv[0]; t[1] += pv[1]; t[2] += pv[2];
  g3_mv(W, t, o); o[0] += root[0]; o[1] += root[1]; o[2] += root[2];
  g3_mul(W, Rl, R); g3_add(o[0], o[1], o[2], w, h, d, R, col, 0, 0);
}
/* pose: 0 idle,1 andar,2 sentado,3 chorando,4 morto,5 raiva,6 mira  */
static void g3_char(int who, float x, float z, float yaw, int pose, float t, float scale) {
  const Cs *s = &CSP[who]; float wd = s->wd, hh = s->hh;
  if (pose != 4) g3_add(x, .012f, z, .72f * wd * scale, .012f, .5f * scale, 0, 0x120e0b, 1, 5e8f);
  float W[9], I[9] = {1,0,0,0,1,0,0,0,1}, T9[9], Wr[9], root[3] = { x, 0, z }, rootOff[3] = { 0, 0, 0 };
  g3_ry(yaw, Wr);
  float la = 0, ra = 0, laz = 0, raz = 0, la_leg = 0, ra_leg = 0, headx = 0, rootY = 0, tilt = 0;
  float bob = sinf(t * 2.0f) * .012f; rootY = bob;
  if (pose == 1) { float w2 = sinf(t * 8) * .7f; la_leg = w2; ra_leg = -w2; la = -w2 * .8f; ra = w2 * .8f; rootY = fabsf(sinf(t * 8)) * .03f; }
  else if (pose == 2) { la_leg = ra_leg = -1.5f; rootY = -.38f; la = ra = -.5f; }
  else if (pose == 3) { headx = .5f; la = ra = -2.4f; laz = .4f; raz = -.4f; rootY = sinf(t * 14) * .01f; }
  else if (pose == 4) { tilt = -1.5708f; rootY = .14f; rootOff[2] = .2f; }
  else if (pose == 5) { la = -2.2f + sinf(t * 20) * .5f; ra = -2.2f - sinf(t * 20) * .5f; rootY = fabsf(sinf(t * 10)) * .04f; la_leg = sinf(t * 10) * .3f; ra_leg = -la_leg; }
  else if (pose == 6) { ra = -1.55f; }
  else if (pose == 8) { headx = -.35f; la = ra = -.5f + sinf(t * 16) * .15f; rootY = bob + fabsf(sinf(t * 16)) * .03f; }
  else if (pose == 7) { ra = -.6f + sinf(t * 4.5f) * .35f; raz = -.25f; la = -.1f; headx = sinf(t * 3.3f) * .05f; rootY = bob + fabsf(sinf(t * 4.5f)) * .012f; }
  if (g3Hold && !g3Gest && pose == 0) { ra = -.35f; }
  { float gt = g3GT;
    switch (g3Gest) {
      case 1: la = -1.1f + sinf(gt * 5) * .25f; ra = -1.2f + sinf(gt * 5 + 1) * .25f; break;
      case 2: ra = -2.6f; raz = -.3f + sinf(gt * 9) * .35f; break;
      case 3: la = ra = -3.0f; break;
      case 4: ra = -1.9f + sinf(gt * 9) * .9f; break;
      case 5: la = ra = -1.4f; ra += sinf(gt * 10) * .25f; break;
      case 6: la = ra = -2.9f + sinf(gt * 10) * .25f; rootY += fabsf(sinf(gt * 7)) * .05f; break;
      case 7: la = -2.4f + sinf(gt * 6) * .6f; ra = -2.4f - sinf(gt * 6) * .6f; rootY += fabsf(sinf(gt * 6)) * .06f; break;
      case 8: ra = -2.8f; raz = -.5f + sinf(gt * 8) * .4f; break;
      case 9: la = ra = -1.1f; laz = .7f; raz = -.7f; break;
      case 10: rootOff[2] = -.15f; headx = .3f; break;
      case 11: ra = -1.55f; break;
      case 12: headx = sinf(gt * 8) * .25f; break;
      case 13: rootY += fabsf(sinf(gt * 14)) * .03f; headx = -.25f + sinf(gt * 14) * .08f; la = ra = -.5f; break;
    } }
  g3_rx(tilt, T9); g3_mul(Wr, T9, W);
  float rt[3] = { root[0] + Wr[2]*rootOff[2], rootY, root[2] + Wr[8]*rootOff[2] };
  (void)I;
  /* escala global (criancas) */
  float S = scale * s->s;
  #define V3(a,b,c) ((float[3]){(a)*S,(b)*S,(c)*S})
  float Rl[9], Rz[9], Rt[9];
  for (int sd = -1; sd <= 1; sd += 2) {
    float ang = sd < 0 ? la_leg : ra_leg;
    g3_rx(ang, Rl);
    if (!(g3Miss & (sd < 0 ? 1 : 4))) {
    g3_part(W, (float[3]){rt[0], rt[1] * S + 0, rt[2]}, V3(sd * .17f * wd, .85f * hh, 0), Rl, V3(0, -.42f * hh, 0), .24f * wd * S, .85f * hh * S, .28f * S, s->leg);
    g3_part(W, (float[3]){rt[0], rt[1] * S, rt[2]}, V3(sd * .17f * wd, .85f * hh, 0), Rl, V3(0, -.82f * hh, .06f), .26f * wd * S, .12f * S, .4f * S, s->shoe);
    }
    float a = sd < 0 ? la : ra, az = sd < 0 ? laz : raz;
    g3_rx(a, Rt); g3_rz(az, Rz); g3_mul(Rz, Rt, Rl);
    float bg = s->big ? 1.15f : 1.0f; unsigned up = s->big ? s->skin : s->top, lo = (s->coat) ? s->top : s->skin;
    float pv[3] = { sd * (.32f * wd + .1f) * S, 1.5f * hh * S, 0 };
    float rr[3] = { rt[0], rt[1] * S, rt[2] };
    if (!(sd < 0 && (g3Miss & 2))) {
    g3_part(W, rr, pv, Rl, V3(0, -.17f, 0), .2f * wd * bg * S, .34f * S, .22f * S, up);
    g3_part(W, rr, pv, Rl, V3(0, -.5f, 0), .17f * wd * S, .34f * S, .19f * S, lo);
    g3_part(W, rr, pv, Rl, V3(0, -.74f, 0), .16f * S, .14f * S, .17f * S, s->skin);
    if (sd > 0 && g3Hold) {
      switch (g3Hold) {
        case 1: g3_part(W, rr, pv, Rl, V3(0, -.74f, .4f), .03f * S, .1f * S, .7f * S, 0xb4b4b4); g3_part(W, rr, pv, Rl, V3(0, -.74f, .02f), .05f * S, .1f * S, .16f * S, 0x5a3a1a); break;
        case 2: g3_part(W, rr, pv, Rl, V3(0, -.74f, .3f), .05f * S, .05f * S, .85f * S, 0x6a4a2a); g3_part(W, rr, pv, Rl, V3(0, -.74f, .66f), .04f * S, .3f * S, .2f * S, 0xa0a0a0); break;
        case 3: g3_part(W, rr, pv, Rl, V3(0, -.74f, .36f), .03f * S, .1f * S, .66f * S, 0xc4c4c4); break;
        case 4: g3_part(W, rr, pv, Rl, V3(0, -.74f, .28f), .16f * S, .22f * S, .5f * S, 0xd06a1a); g3_part(W, rr, pv, Rl, V3(0, -.72f, .78f), .03f * S, .1f * S, .55f * S, 0x909090); break;
        case 5: g3_part(W, rr, pv, Rl, V3(0, -.74f, .14f), .05f * S, .12f * S, .24f * S, 0x151515); break;
        case 6: g3_part(W, rr, pv, Rl, V3(0, -.78f, .04f), .07f * S, .13f * S, .025f * S, 0x111111); break;
        case 7: g3_part(W, rr, pv, Rl, V3(0, -.74f, .16f), .02f * S, .06f * S, .3f * S, 0xcccccc); break;
      }
    }
    }
  }
  float rr[3] = { rt[0], rt[1] * S, rt[2] }; float zero[3] = { 0, 0, 0 };
  g3_rx(0, Rl);
  g3_part(W, rr, zero, Rl, V3(0, 1.2f * hh, 0), .64f * wd * S, .7f * hh * S, .34f * S, s->top);
  if (s->coat) g3_part(W, rr, zero, Rl, V3(0, .78f * hh, 0), .7f * wd * S, .45f * S, .4f * S, s->top);
  g3_part(W, rr, zero, Rl, V3(0, 1.58f * hh, 0), .18f * wd * S, .1f * S, .18f * S, s->skin);
  g3_rx(headx, Rl);
  if (g3Miss & 8) { g3_part(W, rr, zero, Rl, V3(0, 1.62f * hh, 0), .2f * S, .08f * S, .2f * S, 0x8a0a0c); return; }
  g3_part(W, rr, zero, Rl, V3(0, 1.8f * hh, 0), .34f * S, .38f * S, .34f * S, s->skin);
  g3_part(W, rr, zero, Rl, V3(0, 1.8f * hh + .2f, -.01f), .37f * S, .12f * S, .37f * S, s->hair);
  if (s->lng) g3_part(W, rr, zero, Rl, V3(0, 1.8f * hh - .12f, -.16f), .38f * S, .6f * S, .12f * S, s->hair);
  if (who == 0) g3_part(W, rr, zero, Rl, V3(0, 1.8f * hh + .2f, .14f), .36f * S, .1f * S, .12f * S, s->hair);
  /* rosto: olhos com branco e pupila, sobrancelhas, nariz e boca (anima ao falar) */
  float ey = 1.8f * hh + .04f, dk = (s->skin >> 16 & 255) * .8f; unsigned nose = ((int)dk << 16) | ((int)((s->skin >> 8 & 255) * .78f) << 8) | (int)((s->skin & 255) * .78f);
  for (int e = -1; e <= 1; e += 2) {
    g3_part(W, rr, zero, Rl, V3(e * .082f, ey, .172f), .075f * S, .06f * S, .008f * S, 0xe8e4dc);
    g3_part(W, rr, zero, Rl, V3(e * .082f + (g3Look * .012f), ey - .003f, .178f), .035f * S, .045f * S, .008f * S, 0x16110d);
    g3_part(W, rr, zero, Rl, V3(e * .082f, ey + .062f + (pose == 5 ? -.012f : 0), .174f), .095f * S, .018f * S, .008f * S, s->hair);
  }
  g3_part(W, rr, zero, Rl, V3(0, ey - .06f, .185f), .05f * S, .06f * S, .03f * S, nose);
  float mo = g3Talk ? .02f + .04f * (.5f + .5f * sinf(t * 19)) : (pose == 3 || pose == 5 ? .035f : .014f);
  g3_part(W, rr, zero, Rl, V3(0, ey - .15f, .174f), .11f * S, mo * S, .008f * S, pose == 4 ? 0x2a1010 : 0x5a1818);
  #undef V3
}
/* ---- cena completa ---- */
static void g3_begin(int id) {
  const Rm *r = &RMS[id]; g3n = 0; g3_cam(g3Ov ? g3Ov : r->cam); g3Amb = r->amb; g3Fog = r->sky ? r->sky : 0x080606;
  g3Fog = id == 7 ? 0x0a0e18 : (id == 2 ? 0x0c0a09 : 0x0e0a08);
  g3Lp[0] = r->cam[3]; g3Lp[1] = id == 7 ? 4.0f : 2.5f; g3Lp[2] = r->cam[5] + .4f; g3Lk = id == 7 ? .25f : (id == 2 || id == 3 ? .55f : .45f); g3Flick = (id == 2 || id == 3 || id == 5);
  for (int i = 0; i < r->n; i++) {
    const Bx *b = &r->b[i]; float big = fmaxf(b->w, fmaxf(b->h, b->d));
    g3_add(b->x, b->y, b->z, b->w, b->h, b->d, 0, b->c, b->e, big >= 4.0f ? 1e9f - (float)i : 0);
  }
}
static int g3_has(int id) { return id > 0 && id < 11 && RMS[id].n > 0; }
