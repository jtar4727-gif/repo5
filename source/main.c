/* Quero Acabar Logo Com Isso — Nintendo 3DS (etapa 1: motor, menu, dialogos, prologo)
   Tela de cima: arte / cena.  Tela de baixo: dialogo, escolhas, menu (toque ou botoes). */
#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include "script.h"
#include "chapters.h"
#include "playsegs.h"

#define HEX(h) C2D_Color32(((h) >> 16) & 255, ((h) >> 8) & 255, (h) & 255, 255)
#include "g3d.h"
#define SAVE_DIR "sdmc:/3ds/qalci"

/* ---------- estado global ---------- */
typedef enum { ST_MENU, ST_GAME, ST_EXTRAS, ST_CFG } State;
typedef enum { M_SAY, M_NAR, M_CARD, M_CHOICE, M_SERVE, M_MASH, M_QTE, M_PLAY, M_RPS, M_ROLL, M_KEYPAD } Mode;
typedef struct { int music, voice, speed; } Cfg;   /* music/voice 0..10, speed 0..2 */
typedef struct { int ch, kind, rage, moto; } Save;
static int Fl[8];   /* Fl[0]=pontos de convencimento, Fl[1]=evento do motoboy */

static C3D_RenderTarget *top, *topR, *bot;
static C2D_TextBuf tbuf;
static State state = ST_MENU;
static Cfg cfg = { 7, 7, 1 };
static int kind = 0, rage = 0;
static int menuSel = 0, cfgSel = 0, extrasPage = 0;

static const Node *chap;       /* capitulo atual */
static int chapIdx, ni;
static Mode mode;
static const char *curWho, *curText;
static int rpsRound, rpsSel, kpI;
static float shown, cardT, shakeT, flashT, clockT;
static int lastBlipB, letters, artId, sel;
static int curVoice = 200;
static int qtFail = -1, qtFailCh = -1, qtOk = -1, chType, chSwap;

/* ---------- audio (ndsp) ---------- */
#define RATE 22050
#define BLIPN 2000
#define MUSN (RATE * 4)
static int audioOK = 0, musPlaying = 0, romOK = 0;
static s16 *pcmBlip, *pcmMus;
static ndspWaveBuf wbBlip[8], wbMus;
static int blipIdx = 0;

static void setMix(int ch, float v) {
  float mix[12] = { 0 };
  mix[0] = mix[1] = v;
  ndspChnSetMix(ch, mix);
}
static void applyVolumes(void) {
  if (!audioOK) return;
  setMix(0, cfg.music / 10.0f * 0.6f);
  setMix(1, cfg.voice / 10.0f * 0.6f);
  setMix(2, 0.8f);
}
static const char *SFN[SF_N] = { 0, "shot", "door", "hit", "thud", "stinger", "pick", "cut", "ring", "heart", "car", "splash", "gore", "glitch", "saw", "crash", "splat", "dial", "wind", "siren" };
static s16 *sfxD[SF_N]; static int sfxL[SF_N]; static ndspWaveBuf sfxW[SF_N];
static void sfxPlay(int id) {
  if (!audioOK || id <= 0 || id >= SF_N || !sfxD[id]) return;
  ndspChnWaveBufClear(2);
  memset(&sfxW[id], 0, sizeof sfxW[id]); sfxW[id].data_vaddr = sfxD[id]; sfxW[id].nsamples = sfxL[id] / 2;
  ndspChnSetRate(2, 16000.0f); ndspChnWaveBufAdd(2, &sfxW[id]);
}
static void sfxLoad(void) {
  if (!romOK) return;
  for (int i = 1; i < SF_N; i++) {
    char p[64]; snprintf(p, sizeof p, "romfs:/s_%s.pcm", SFN[i]); FILE *f = fopen(p, "rb"); if (!f) continue;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    sfxD[i] = (s16 *)linearAlloc(sz);
    if (sfxD[i] && fread(sfxD[i], 1, sz, f) == (size_t)sz) { DSP_FlushDataCache(sfxD[i], sz); sfxL[i] = (int)sz; } else { if (sfxD[i]) linearFree(sfxD[i]); sfxD[i] = NULL; }
    fclose(f);
  }
}
static const char *MUSF[MUS_N] = { 0, "calm", "chaos", "tense", "menu", "sad", "rock", "credits", "purple", "retro", "isl", "pdg", "ybf", "hey" };
static const int MUSR[MUS_N] = { 0, 16000, 16000, 16000, 16000, 16000, 16000, 16000, 16000, 16000, 12000, 12000, 12000, 12000 };
static char audDiag[128];
static void audDiagMake(void) {
  int nm = 0, ns = 0; char p[64];
  if (romOK) {
    for (int i = 1; i < MUS_N; i++) if (MUSF[i]) { snprintf(p, sizeof p, "romfs:/m_%s.pcm", MUSF[i]); FILE *f = fopen(p, "rb"); if (f) { nm++; fclose(f); } }
    for (int i = 1; i < SF_N; i++) if (sfxD[i]) ns++;
  }
  snprintf(audDiag, sizeof audDiag, "Áudio: %s | arquivos: %s | músicas %d/%d | efeitos %d/%d", audioOK ? "ok" : "FALHOU", romOK ? "ok" : "NÃO ENCONTRADOS (pasta romfs)", nm, MUS_N - 1, ns, SF_N - 1);
}
static void audioInit(void) {
  romOK = R_SUCCEEDED(romfsInit());
  if (R_FAILED(ndspInit())) return;            /* sem dspfirm.cdc: segue sem som */
  ndspSetOutputMode(NDSP_OUTPUT_STEREO);
  for (int c = 0; c < 3; c++) {
    ndspChnReset(c);
    ndspChnSetInterp(c, NDSP_INTERP_LINEAR);
    ndspChnSetRate(c, RATE);
    ndspChnSetFormat(c, NDSP_FORMAT_MONO_PCM16);
  }
  pcmBlip = (s16 *)linearAlloc(sizeof(s16) * BLIPN * 8);
  pcmMus  = (s16 *)linearAlloc(sizeof(s16) * MUSN);
  if (!pcmBlip || !pcmMus) { audioOK = 0; return; }
  audioOK = 1; sfxLoad();
  /* drone: 55 Hz + 58,25 Hz (ciclos inteiros em 4 s = loop sem estalo) */
  for (int i = 0; i < MUSN; i++) {
    float t = (float)i / RATE;
    float a = sinf(6.2831853f * 55.0f * t) + 0.5f * sinf(6.2831853f * 110.0f * t) + 0.8f * sinf(6.2831853f * 58.25f * t);
    float m = 0.6f + 0.4f * sinf(6.2831853f * 0.25f * t);
    pcmMus[i] = (s16)(a * m * 2600.0f);
  }
  DSP_FlushDataCache(pcmMus, sizeof(s16) * MUSN);
  memset(&wbMus, 0, sizeof wbMus);
  wbMus.data_vaddr = pcmMus; wbMus.nsamples = MUSN; wbMus.looping = true;
  memset(wbBlip, 0, sizeof wbBlip);
  audioOK = 1;
  applyVolumes();
}
static s16 *pcmTrk; static ndspWaveBuf wbTrk; static int curMus = -1;
static void setMusic(int id) {
  if (!audioOK) return;
  if (id == curMus && musPlaying) return;
  ndspChnWaveBufClear(0); musPlaying = 0; curMus = id;
  if (pcmTrk) { linearFree(pcmTrk); pcmTrk = NULL; }
  if (id == MUS_OFF) return;
  if (romOK && id > 0 && id < MUS_N && MUSF[id]) {
    char path[64]; snprintf(path, sizeof path, "romfs:/m_%s.pcm", MUSF[id]);
    FILE *f = fopen(path, "rb");
    if (f) {
      fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
      pcmTrk = (s16 *)linearAlloc(sz);
      if (pcmTrk && fread(pcmTrk, 1, sz, f) == (size_t)sz) {
        fclose(f);
        DSP_FlushDataCache(pcmTrk, sz);
        memset(&wbTrk, 0, sizeof wbTrk);
        wbTrk.data_vaddr = pcmTrk; wbTrk.nsamples = sz / 2; wbTrk.looping = (id != MUS_PDG && id != MUS_ISL && id != MUS_YBF && id != MUS_HEY);
        ndspChnSetRate(0, (float)MUSR[id]);
        ndspChnWaveBufAdd(0, &wbTrk); musPlaying = 1; return;
      }
      fclose(f); if (pcmTrk) { linearFree(pcmTrk); pcmTrk = NULL; }
    }
  }
  /* sem arquivo: drone sintetico */
  ndspChnSetRate(0, id == MUS_CHAOS ? 32000.0f : id == MUS_TENSE ? 25000.0f : (float)RATE);
  ndspChnWaveBufAdd(0, &wbMus); musPlaying = 1;
}
/* "voz" estilo Sims: um blip curto de serra com tom aleatorio */
static void blip(void) {
  if (!audioOK) return;
  int i = blipIdx++ & 7;
  if (wbBlip[i].status == NDSP_WBUF_QUEUED || wbBlip[i].status == NDSP_WBUF_PLAYING) return;
  s16 *p = pcmBlip + i * BLIPN;
  float f = curVoice * (0.85f + (rand() % 100) / 200.0f);
  for (int k = 0; k < BLIPN; k++) {
    float t = (float)k / RATE, ph = fmodf(t * f, 1.0f);
    float env = expf(-t * 28.0f);
    p[k] = (s16)((ph * 2.0f - 1.0f) * env * 9000.0f);
  }
  DSP_FlushDataCache(p, sizeof(s16) * BLIPN);
  wbBlip[i].data_vaddr = p; wbBlip[i].nsamples = BLIPN; wbBlip[i].looping = false;
  ndspChnWaveBufAdd(1, &wbBlip[i]);
}
static int voiceFor(const char *w) {
  int hz = 200;
  if (!strncmp(w, "LUCAS", 5)) hz = 240;
  else if (!strncmp(w, "MATEUS", 6)) hz = 140;
  else if (!strncmp(w, "MARCOS", 6)) hz = 105;
  else if (!strncmp(w, "DONA", 4)) hz = 390;
  if (strstr(w, "CRIAN")) hz = hz * 9 / 5;
  else if (strstr(w, "ADOLE")) hz = hz * 13 / 10;
  return hz;
}

/* ---------- salvamento ---------- */
static void ensureDir(void) { mkdir("sdmc:/3ds", 0777); mkdir(SAVE_DIR, 0777); }
static void saveGame(void) {
  ensureDir();
  FILE *f = fopen(SAVE_DIR "/save.bin", "wb");
  if (!f) return;
  Save s = { chapIdx, kind, rage, Fl[1] };
  fwrite(&s, sizeof s, 1, f); fclose(f);
}
static int loadSave(Save *s) {
  FILE *f = fopen(SAVE_DIR "/save.bin", "rb");
  if (!f) return 0;
  int ok = fread(s, sizeof *s, 1, f) == 1; fclose(f);
  return ok;
}
static void saveCfg(void) {
  ensureDir();
  FILE *f = fopen(SAVE_DIR "/cfg.bin", "wb");
  if (f) { fwrite(&cfg, sizeof cfg, 1, f); fclose(f); }
}
static void loadCfg(void) {
  FILE *f = fopen(SAVE_DIR "/cfg.bin", "rb");
  if (!f) return;
  Cfg c; if (fread(&c, sizeof c, 1, f) == 1 && c.music >= 0 && c.music <= 10 && c.voice >= 0 && c.voice <= 10 && c.speed >= 0 && c.speed <= 2) cfg = c;
  fclose(f);
}

static const Node *CHS[11] = { CH0, CH1, CH2, CH3, CH4, CH5, CH6, CH7, CH8, CH9, CH10 };
static int locId = 0;   /* 0 nenhum,1 sala,2 porao,3 garagem,4 cozinha,5 banheiro,6 quarto,7 rua,8 beco/fast food,9 apto,10 patio */
static void setLoc(const char *t) {
  if (strncmp(t, "Int.", 4) && strncmp(t, "Ext.", 4)) return;
  static const struct { const char *k; int id; } L[] = { {"orão",2},{"Porão",2},{"Sala",1},{"sala",1},{"Garagem",3},{"garagem",3},{"Cozinha",4},{"cozinha",4},{"Banheiro",5},{"banheiro",5},{"Quarto",6},{"quarto",6},{"Rua",7},{"rua",7},{"Estacion",7},{"Beco",8},{"Fundos",8},{"fast food",8},{"Fast",8},{"Apart",9},{"apart",9},{"Pátio",10},{"pátio",10},{"Casa",1} };
  for (unsigned i = 0; i < sizeof L / sizeof *L; i++) if (strstr(t, L[i].k)) { locId = L[i].id; return; }
}
/* ---------- desenho ---------- */
static void txt(const char *s, float x, float y, float sc, u32 col, float wrap, u32 flags) {
  C2D_Text t;
  C2D_TextParse(&t, tbuf, s);
  C2D_TextOptimize(&t);
  if (wrap > 0) C2D_DrawText(&t, C2D_WithColor | C2D_WordWrap | flags, x, y, 0.5f, sc, sc, col, wrap);
  else          C2D_DrawText(&t, C2D_WithColor | flags, x, y, 0.5f, sc, sc, col);
}
/* arte em 160x120 logicos, ampliada 2x e centralizada na tela de cima (400x240) */
static float shx, shy;
static void R(float x, float y, float w, float h, u32 hex) { C2D_DrawRectSolid(40 + shx + x * 2, shy + y * 2, 0, w * 2, h * 2, HEX(hex)); }
static void Ci(float x, float y, float r, u32 hex) { C2D_DrawCircleSolid(40 + shx + x * 2, shy + y * 2, 0, r * 2, HEX(hex)); }
static void person(float x, float y, float s, int sit, u32 top, u32 hair) {
  if (sit) { R(x - 4 * s, y - 6 * s, 11 * s, 3 * s, 0x333333); R(x - 4 * s, y - 17 * s, 8 * s, 11 * s, top); Ci(x, y - 21 * s, 4 * s, 0xd2a98a); R(x - 4 * s, y - 26 * s, 8 * s, 3 * s, hair); }
  else { R(x - 3 * s, y - 10 * s, 3 * s, 10 * s, 0x333333); R(x + s, y - 10 * s, 3 * s, 10 * s, 0x333333); R(x - 4 * s, y - 20 * s, 9 * s, 10 * s, top); Ci(x, y - 24 * s, 4 * s, 0xd2a98a); R(x - 4 * s, y - 29 * s, 9 * s, 4 * s, hair); }
}
/* arte 2D do PC: quadros capturados do proprio jogo (romfs a_<nome>.bin, 4 quadros de 256x128 RGB565) */
static C3D_Tex artTex; static int artTexOK = 0, artLoaded = -1, artFrame = -1; static unsigned char *artBuf;
static int artLoad(int id) {
  if (artLoaded == id) return artBuf != NULL;
  artLoaded = id; artFrame = -1; if (artBuf) { linearFree(artBuf); artBuf = NULL; }
  if (!romOK || id <= 0 || id >= ART_COUNT) return 0;
  char p[64]; snprintf(p, sizeof p, "romfs:/a_%s.bin", ARTN[id]); FILE *f = fopen(p, "rb"); if (!f) return 0;
  artBuf = (unsigned char *)linearAlloc(4 * 65536);
  if (!artBuf || fread(artBuf, 1, 4 * 65536, f) != 4 * 65536) { if (artBuf) linearFree(artBuf); artBuf = NULL; fclose(f); return 0; }
  fclose(f);
  if (!artTexOK) { if (!C3D_TexInit(&artTex, 256, 128, GPU_RGB565)) { linearFree(artBuf); artBuf = NULL; return 0; } C3D_TexSetFilter(&artTex, GPU_NEAREST, GPU_NEAREST); C3D_TexSetWrap(&artTex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE); artTexOK = 1; }
  return 1;
}
static int drawArtImg(int id) {
  if (!artLoad(id)) return 0;
  float t = clockT; int fr = t < .9f ? 0 : t < 2.2f ? 1 : t < 3.7f ? 2 : 3; if (t > 4.5f) fr = 1 + ((int)((t - 4.5f) * 1.5f)) % 3;
  if (fr != artFrame) { artFrame = fr; memcpy(artTex.data, artBuf + fr * 65536, 65536); GSPGPU_FlushDataCache(artTex.data, 65536); }
  static Tex3DS_SubTexture sub; sub.width = 160; sub.height = 120; sub.left = 0.0f; sub.right = 160.0f / 256.0f; sub.top = 1.0f; sub.bottom = 1.0f - 120.0f / 128.0f;
  C2D_Image img = { &artTex, &sub };
  C2D_DrawRectSolid(0, 0, 0, 400, 240, HEX(0x000000));
  C2D_DrawImageAt(img, 40 + shx, shy, 0, NULL, 2.0f, 2.0f);
  if (t < .6f) C2D_DrawRectSolid(0, 0, 0, 400, 240, C2D_Color32(0, 0, 0, (int)(255 * (1 - t / .6f))));
  return 1;
}
static void drawArt(int id) {
  if (drawArtImg(id)) return;
  C2D_DrawRectSolid(0, 0, 0, 400, 240, HEX(0x000000));
  float t = clockT;
  if (id == ART_PATEO) {
    R(0, 0, 160, 60, 0xa8cde0); R(0, 38, 160, 42, 0x8a6e58);
    for (int i = 0; i < 8; i++) R(8 + i * 19, 46, 10, 10, 0xcfe1e8);
    R(0, 80, 160, 40, 0xb59a6a);
    static const u32 cl[5] = { 0xcc4444, 0x4488cc, 0x44aa44, 0xcccc44, 0xaa55aa };
    for (int i = 0; i < 5; i++) person(50 + i * 20 + sinf(t * 3 + i) * 5, 84 + (i % 2) * 4, 0.55f, 0, cl[i], 0x222222);
    R(10, 92, 3, 16, 0x4a3a2a); R(10, 92, 16, 3, 0x4a3a2a); R(24, 92, 3, 16, 0x4a3a2a);
    person(18, 112, 1, 1, 0x7a8a9a, 0x332211); R(26, 98, 10, 8, 0xe8e0c8);
  } else if (id == ART_BAILE) {
    R(0, 0, 160, 120, 0x1a0a2e); R(0, 84, 160, 36, 0x2a1048); Ci(80, 14, 8, 0xd8b8f8);
    for (int i = 0; i < 3; i++) { float sw = sinf(t * 1.3f + i) * 2; person(40 + i * 40 + sw - 7, 106, 0.8f, 0, 0x7a2a9a, 0x222222); person(40 + i * 40 + sw + 7, 106, 0.8f, 0, 0xe0a0e8, 0x8a5a2a); }
    for (int i = 0; i < 40; i++) R(fmodf(i * 37 + t * 40, 160), fmodf(i * 23 + t * 90, 120), 1, 5, 0xbe82ff);
    R(0, 0, 160, 6, 0xe8a8f0);
  } else if (id == ART_MACA) {
    R(0, 0, 160, 120, 0x2a1c14); R(30, 60, 100, 40, 0x8a6a48);
    Ci(80, 64, 22, 0xb3201a); Ci(80, 64, 18, 0xd83a2a); R(78, 38, 3, 10, 0x3a2a14);
    float k = t * 0.3f; if (k > 1) k = 1;
    R(62, 64, 36, 28, k > 0.5f ? 0xb40a0a : 0x8a1a14);
    for (int i = 0; i < 6; i++) R(60 + i * 7, 84 + fmodf(t * 30 + i * 9, 24), 2, 4, 0xaa0000);
    R(100, 50, 22, 3, 0xcfd0d4); R(118, 48, 12, 6, 0x5a3a22);
    person(36, 110, 1.1f, 0, 0x7a8a9a, 0x332211);
  }
}
/* ---- cenas 3D (mesmos modelos do jogo de PC) ---- */
static int sevCnt;
static void blood(float x, float z, float w, float d) { g3_add(x, .02f, z, w, .03f, d, 0, 0x6a0a0a, 0, 0); }
static float sevStart;
static void limbAt(float x, float z, float ry, int kind, int idx, float ox, float oz) {
  float q = idx == sevCnt ? fminf(1.0f, (clockT - sevStart) / .7f) : 1.0f; if (q < 0) q = 0;
  float fx = ox + (x - ox) * q, fz = oz + (z - oz) * q, fy = (1 - q) * .8f + sinf(q * 3.14159f) * .45f;
  float R[9]; g3_ry(ry + (1 - q) * 5.0f, R); const Cs *s = &CSP[1];
  if (kind == 0) g3_add(fx, .13f + fy, fz, .85f, .25f, .3f, R, s->leg, 0, 0);
  else if (kind == 1) g3_add(fx, .09f + fy, fz, .7f, .17f, .19f, R, s->skin, 0, 0);
  else { g3_add(fx, .19f + fy, fz, .34f, .38f, .34f, R, s->skin, 0, 0); g3_add(fx, .4f + fy, fz, .37f, .1f, .37f, R, s->hair, 0, 0); }
}
#include "stage.h"
static int castMode; /* 0 nada, 1 dialogo, 2 exploracao */
static float plX, plZ, plYaw; static int plMove, plWho, plHold; static void drawSegNpcs(void);
static int hasChar(const char *w, const char *n) { return !strncmp(w, n, strlen(n)); }
static int emoPose(const char *w) {
  char b[64]; int n = 0; for (; w && w[n] && n < 63; n++) b[n] = (w[n] >= 'A' && w[n] <= 'Z') ? w[n] + 32 : w[n]; b[n] = 0;
  if (strstr(b, "raiva") || strstr(b, "gritando") || strstr(b, "furi")) return 5;
  if (strstr(b, "chor") || strstr(b, "tristeza") || strstr(b, "desespero") || strstr(b, "dor)") || strstr(b, "(dor")) return 3;
  if (strstr(b, "rindo") || strstr(b, "risada") || strstr(b, "gargalh")) return 8;
  return 7;
}
static void scene3(int id) {
  const Rm *r = &RMS[id];
  C2D_DrawRectSolid(0, 0, 0, 400, 240, HEX(id == 7 ? 0x0a0e18 : 0x0c0a09));
  g3_begin(id);
  g3T = clockT; float tx = r->cam[3], tz = r->cam[5];
  if (id == 2 && chapIdx >= 5) { /* Carlos esquartejado */
    const Cs *k = &CSP[5]; float R1[9], R2[9], R3[9]; g3_ry(.1f, R1); g3_ry(.5f, R2); g3_ry(-.3f, R3);
    blood(0, -.2f, 3.6f, 2.8f); blood(-1.1f, .5f, 1.2f, .9f); blood(.9f, .6f, 1.1f, .9f);
    g3_add(0, .13f, -.2f, .9f, .22f, .5f, R1, k->top, 0, 0); g3_add(0, .25f, -.2f, .92f, .03f, .52f, R1, 0x7a0a0c, 0, 0);
    g3_add(1.0f, .18f, -.5f, .34f, .34f, .34f, 0, k->skin, 0, 0); g3_add(1.0f, .38f, -.5f, .36f, .1f, .36f, 0, k->hair, 0, 0);
    g3_add(-1.1f, .09f, .5f, .7f, .14f, .14f, R2, k->top, 0, 0); g3_add(.9f, .09f, .6f, .7f, .14f, .14f, R3, k->top, 0, 0);
    g3_add(-.7f, .1f, -1.0f, .8f, .17f, .17f, R1, k->leg, 0, 0); g3_add(.3f, .1f, 1.0f, .8f, .17f, .17f, R2, k->leg, 0, 0);
  }
  if (chapIdx == 7 && id == 3 && sevCnt > 0 && castMode == 1) {
    g3Miss = (1 << sevCnt) - 1; blood(tx + .7f, tz + 1.0f, 1.4f + sevCnt * .7f, 1.0f + sevCnt * .5f);
    { float ox = tx + .7f, oz = tz + 1.0f;
      if (sevCnt >= 1) limbAt(tx - .2f, tz + 2.0f, .5f, 0, 1, ox, oz); if (sevCnt >= 2) limbAt(tx + 2.3f, tz + 1.8f, -.4f, 1, 2, ox, oz);
      if (sevCnt >= 3) limbAt(tx + 1.7f, tz + 2.5f, 1.1f, 0, 3, ox, oz); if (sevCnt >= 4) limbAt(tx + .2f, tz + 2.2f, .3f, 2, 4, ox, oz); }
    g3_char(0, tx - 1.9f, tz + 1.2f, .5f, 0, clockT, 1); g3_char(1, tx + .7f, tz + 1.0f, 1.2f, 4, clockT, 1); g3Miss = 0;
    g3_draw(); C2D_DrawRectSolid(40, 0, 0, 320, 240, C2D_Color32(0, 0, 0, 34)); return;
  }
  if (id == 3 && chapIdx >= 6) g3_add(.2f, .02f, .4f, 2.8f, .03f, 1.8f, 0, 0x6a0a0a, 0, 0);
  if (castMode == 1 && stageOn) {
    int spk = -1, typing = curText && (int)shown < (int)strlen(curText);
    if (mode == M_SAY) { if (hasChar(curWho, "LUCAS")) spk = 0; else if (hasChar(curWho, "MATEUS")) spk = 1; else if (hasChar(curWho, "MARCOS")) spk = 2; else if (hasChar(curWho, "MOTOBOY")) spk = 16; }
    for (int i = 0; i < 8; i++) { Act *a = &ACT[i]; if (!a->on || a->fade < .5f) continue;
      if (a->who == 5 && a->pose == 4 && id == 2 && chapIdx >= 5) continue;
      if (a->who == 16) { float bx = a->x, bz = a->z + 1.5f; /* moto */
        g3_add(bx, .3f, bz - .6f, .14f, .6f, .6f, 0, 0x151515, 0, 0); g3_add(bx, .3f, bz + .6f, .14f, .6f, .6f, 0, 0x151515, 0, 0);
        g3_add(bx, .58f, bz, .3f, .3f, 1.3f, 0, 0x8a1010, 0, 0); g3_add(bx, .8f, bz - .1f, .3f, .1f, .55f, 0, 0x1a1a1a, 0, 0);
        g3_add(bx, .98f, bz + .55f, .75f, .06f, .06f, 0, 0x2a2a2a, 0, 0); g3_add(bx, .8f, bz + .72f, .16f, .16f, .1f, 0, 0xf0e090, 0, 0); }
      g3Talk = typing && spk >= 0 && stFam(a->who) == spk; g3Gest = a->gest; g3GT = a->gT; g3Hold = a->hold;
      g3_char(a->who, a->x, a->z, a->yaw, a->moving ? 1 : a->pose, clockT + a->off, 1);
      g3Talk = 0; g3Gest = 0; g3Hold = 0; }
  } else if (castMode == 1) {
    int spk = -1, typing = curText && (int)shown < (int)strlen(curText); if (mode == M_SAY) { if (hasChar(curWho, "LUCAS")) spk = 0; else if (hasChar(curWho, "MATEUS")) spk = 1; else if (hasChar(curWho, "MARCOS")) spk = 2; else if (hasChar(curWho, "LÚCIA") || hasChar(curWho, "LUCIA")) spk = 3; else if (hasChar(curWho, "TIO")) spk = 4; else if (hasChar(curWho, "CARLOS")) spk = 5; }
    int pm = chapIdx >= 1 || spk == 1, pc = spk == 2 || chapIdx >= 3;
    int ep = spk >= 0 ? emoPose(curWho) : 0;
    g3Talk = spk == 0 && typing; g3_char(0, tx - 1.7f, tz + 1.5f, .35f + (spk == 0 ? .08f : 0), spk == 0 ? ep : 0, clockT, 1);
    g3Talk = spk == 1 && typing; if (pm) g3_char(1, tx + .1f, tz + .7f, 0, spk == 1 ? ep : 0, clockT + 1, 1);
    g3Talk = spk == 2 && typing; if (pc) g3_char(2, tx + 1.9f, tz + 1.5f, -.35f, spk == 2 ? ep : 0, clockT + 3, 1); g3Talk = 0;
    if (spk >= 3) { g3Talk = 1; g3_char(spk, tx + .1f, tz + 2.2f, 0, 7, clockT, 1); g3Talk = 0; }
  } else if (castMode == 2) {
    drawSegNpcs(); g3Hold = plHold; g3_char(plWho, plX, plZ, plYaw, plMove ? 1 : 0, clockT, 1); g3Hold = 0;
  }
  g3_draw();
  C2D_DrawRectSolid(40, 0, 0, 320, 240, C2D_Color32(0, 0, 0, id == 7 ? 20 : 34));
}
static void drawRoom(int id) {
  if (g3_has(id)) { scene3(id); return; }
  static const u32 wall[11] = { 0x0a0807, 0x6b5a46, 0x4a4a46, 0x5a5448, 0x9a9684, 0x8a9a98, 0x5a5248, 0x10131c, 0x3a2a24, 0x6a6258, 0xa8cde0 };
  static const u32 flr[11]  = { 0x0a0807, 0x4a3a28, 0x34342f, 0x3a3830, 0x6a645a, 0x6a7270, 0x3a2a1c, 0x1a1c22, 0x2a2420, 0x4a4238, 0xb59a6a };
  R(0, 0, 160, 120, wall[id]); R(0, 78, 160, 42, flr[id]);
  float fl = 0.85f + 0.15f * sinf(clockT * 7.0f);
  if (id == 1) { R(40, 30, 50, 30, 0x1a1a1e); R(44, 34, 42, 22, 0x2a5a5a); R(30, 84, 80, 18, 0x7a5a42); R(30, 76, 80, 10, 0x8a6a52); R(120, 40, 24, 36, 0x3a2a1c); }
  else if (id == 2) { R(0, 0, 160, 120, chapIdx >= 5 ? 0x3a2a28 : wall[2]); R(0, 78, 160, 42, 0x2a2a26); R(70, 0, 2, 26, 0x1a1a1a); Ci(71, 28, 4, 0xe8d890 + 0); R(100, 50, 40, 28, 0x5a4a3a); R(20, 60, 24, 24, 0x6a5a48); if (chapIdx >= 5) { R(50, 94, 60, 12, 0x6a0a0a); R(42, 96, 12, 6, 0x6a0a0a); } }
  else if (id == 3) { R(10, 24, 60, 54, 0x2a2a2e); R(100, 60, 44, 20, 0x4a4a52); R(104, 52, 36, 10, 0x6a2a2a); if (chapIdx >= 6) R(60, 96, 40, 10, 0x6a0a0a); }
  else if (id == 4) { R(0, 60, 160, 20, 0xd8d4c8); R(10, 20, 60, 20, 0xbfb69c); R(100, 30, 28, 50, 0xe8e8e8); R(60, 86, 40, 16, 0x7a6a52); }
  else if (id == 5) { R(0, 0, 160, 78, 0x8a9a98); R(100, 34, 30, 44, 0xe8e8e8); R(20, 50, 28, 28, 0xcfd0d4); R(24, 20, 20, 24, 0x9ab0c0); }
  else if (id == 6) { R(86, 70, 60, 30, 0x33333f); R(20, 40, 34, 40, 0x2a2420); R(26, 44, 22, 18, 0x2a5a5a); }
  else if (id == 7) { R(0, 0, 160, 50, 0x1a2030); for (int i = 0; i < 20; i++) R((i * 37) % 160, (i * 19) % 40, 1, 1, 0xd8e0ff); R(10, 40, 40, 50, 0x1a1a22); R(100, 60, 50, 20, 0x3a3a42); R(104, 52, 36, 12, 0x4a4a54); R(120, 80, 6, 6, 0x111111); }
  else if (id == 8) { R(0, 0, 160, 78, 0x4a3a34); for (int i = 0; i < 9; i++) for (int j = 0; j < 4; j++) R(i * 18, j * 12, 17, 11, (i + j) & 1 ? 0x5a443a : 0x4a3830); R(110, 30, 26, 48, 0x2a2a2e); }
  else if (id == 9) { R(100, 24, 40, 56, 0xe8e8e8); R(20, 80, 60, 20, 0x5a4a3a); R(30, 30, 30, 24, 0x7a9ab0); }
  else if (id == 10) { R(0, 0, 160, 60, 0xa8cde0); R(0, 38, 160, 42, 0x8a6e58); }
  /* iluminacao e vinheta */
  C2D_DrawRectSolid(40, 0, 0, 320, 240, C2D_Color32(0, 0, 0, (int)((id == 7 ? 70 : 105) * (2 - fl))));
}
static u32 whoColor(const char *w) {
  if (!strncmp(w, "LUCAS", 5)) return HEX(0xc8d0c0);
  if (!strncmp(w, "MATEUS", 6)) return HEX(0xd8b8b0);
  if (!strncmp(w, "MARCOS", 6)) return HEX(0xe0b880);
  return HEX(0xd8ceb4);
}
static int utf8len(const char *s, int nb) { int n = 0; for (int i = 0; i < nb; i++) if ((s[i] & 0xC0) != 0x80) n++; return n; }

static void drawPlayTop(void); static void drawPlayBottom(void); static void drawServeTop(void); static void drawServeBottom(void);
static void drawMiniTop(int); static void drawMiniBottom(int); static void drawRpsTop(void); static void drawRollTop(void); static void drawKeypadTop(void); static void drawKeypadBottom(void); static void drawRpsBottom(void);
static void drawTop(void) {
  shx = shy = 0;
  if (flashT > 0) { shx = 0; }
  if (shakeT > 0) { shx = (rand() % 9 - 4) * shakeT * 2; shy = (rand() % 9 - 4) * shakeT * 2; }
  if (state == ST_MENU || state == ST_EXTRAS || state == ST_CFG) {
    C2D_DrawRectSolid(0, 0, 0, 400, 240, HEX(0x060403));
    { /* garagem ensanguentada, camera orbitando devagar (como o menu do PC) */
      float t = clockT; float cam[6] = { sinf(t * .2f) * 1.8f, 3.3f, 6.2f, 0, .6f, .2f };
      g3Ov = cam; g3_begin(3); g3Ov = NULL;
      g3_add(.2f, .02f, .5f, 3.0f, .03f, 2.0f, 0, 0x6a0a0a, 0, 0); g3_add(-1.8f, .02f, 1.3f, 1.4f, .03f, 1.0f, 0, 0x560808, 0, 0); g3_add(1.9f, .02f, -.6f, 1.1f, .03f, 1.2f, 0, 0x5a0808, 0, 0);
      g3_char(5, .1f, .3f, .5f, 4, t, 1); g3_char(1, -1.9f, 1.3f, 2.2f, 4, t, 1);
      g3_draw();
      int fl = (int)(70 + 50 * sinf(t * 9) * sinf(t * 2.3f)); if (fl < 40) fl = 40;
      C2D_DrawRectSolid(0, 0, 0, 400, 240, C2D_Color32(0, 0, 0, fl + 40));
      for (int i = 0; i < 6; i++) C2D_DrawRectSolid(0, i * 20, 0, 400, 20, C2D_Color32(0, 0, 0, 120 - i * 20));
    }
    txt("QUERO ACABAR", 30 + 2, 40 + 2, 1.25f, HEX(0x7a100c), 0, 0);
    txt("QUERO ACABAR", 30, 40, 1.25f, HEX(0xd8ceb4), 0, 0);
    txt("LOGO COM ISSO", 30 + 2, 90 + 2, 1.25f, HEX(0x7a100c), 0, 0);
    txt("LOGO COM ISSO", 30, 90, 1.25f, HEX(0xd8ceb4), 0, 0);
    txt("Roteiro: Arthur Abreu", 30, 200, 0.5f, HEX(0x6b6352), 0, 0);
    return;
  }
  if (mode == M_PLAY) { drawPlayTop(); return; }
  if (mode == M_SERVE) { drawServeTop(); return; }
  if (mode == M_KEYPAD) { drawKeypadTop(); return; }
  if (mode == M_ROLL) { drawRollTop(); return; }
  if (mode == M_RPS) { drawRpsTop(); return; }
  if (mode == M_MASH || mode == M_QTE) { drawMiniTop(mode == M_QTE); return; }
  if (mode == M_CARD) {
    C2D_DrawRectSolid(0, 0, 0, 400, 240, HEX(0x000000));
    txt(curWho, 200, 100, 0.9f, HEX(0xd8ceb4), 0, C2D_AlignCenter);
    return;
  }
  if (artId) drawArt(artId);
  else if (locId) {
    C2D_DrawRectSolid(0, 0, 0, 400, 240, HEX(0x000000)); castMode = 1; drawRoom(locId); castMode = 0;
    if (!g3_has(locId) && (mode == M_SAY || mode == M_NAR)) {
      /* retrato do personagem que fala */
      u32 top = 0x4a5a6a, hair = 0x222222; int show = 0;
      if (mode == M_SAY) {
        if (!strncmp(curWho, "LUCAS", 5)) { top = 0x6a7a8a; hair = 0x332211; show = 1; }
        else if (!strncmp(curWho, "MATEUS", 6)) { top = 0x20242a; hair = 0x111111; show = 1; }
        else if (!strncmp(curWho, "MARCOS", 6)) { top = 0x8a4a3a; hair = 0x0c0907; show = 1; }
        else if (strlen(curWho)) { top = 0x6a5a7a; show = 1; }
      }
      if (show) { float bob = sinf(clockT * 6.0f) * 1.2f; person(80, 118 + bob, 2.2f, 0, top, hair); }
    }
  } else { C2D_DrawRectSolid(0, 0, 0, 400, 240, HEX(0x0a0807)); }
  C2D_DrawRectSolid(0, 0, 0, 40, 240, HEX(0x000000));
  C2D_DrawRectSolid(360, 0, 0, 40, 240, HEX(0x000000));
  if (flashT > 0) C2D_DrawRectSolid(0, 0, 0, 400, 240, C2D_Color32(190, 0, 0, (int)(flashT * 255)));
}

static void drawBottom(void) {
  C2D_DrawRectSolid(0, 0, 0, 320, 240, HEX(0x0c0907));
  if (state == ST_MENU) {
    static const char *it[4] = { "Novo jogo", "Carregar jogo", "Extras", "Configurações" };
    for (int i = 0; i < 4; i++) {
      if (i == menuSel) { C2D_DrawRectSolid(30, 40 + i * 40, 0, 260, 34, HEX(0x241a10)); txt("▶", 40, 44 + i * 40, 0.8f, HEX(0xc2ae6a), 0, 0); }
      txt(it[i], 70, 44 + i * 40, 0.8f, i == menuSel ? HEX(0xc2ae6a) : HEX(0x8a8170), 0, 0);
    }
    txt("START: sair", 8, 224, 0.4f, HEX(0x6b6352), 0, 0);
    return;
  }
  if (state == ST_EXTRAS) {
    txt("Extras", 16, 10, 0.8f, HEX(0xc2ae6a), 0, 0);
    txt("Créditos\n\nRoteiro: Arthur Abreu\nDirigido por: Arthur Abreu\nProgramado por: Heitor S", 16, 44, 0.6f, HEX(0xbdb49c), 290, 0);
    txt("B voltar", 16, 220, 0.45f, HEX(0x6b6352), 0, 0);
    return;
  }
  if (state == ST_CFG) {
    static const char *nm[3] = { "Música", "Voz dos personagens", "Velocidade do texto" };
    int v[3] = { cfg.music, cfg.voice, cfg.speed };
    txt("Configurações", 16, 10, 0.8f, HEX(0xc2ae6a), 0, 0);
    for (int i = 0; i < 3; i++) {
      char b[64];
      if (i < 2) snprintf(b, sizeof b, "%s:  ◀ %d ▶", nm[i], v[i]);
      else snprintf(b, sizeof b, "%s:  ◀ %s ▶", nm[i], v[i] == 0 ? "lenta" : v[i] == 1 ? "normal" : "rápida");
      txt(b, 24, 56 + i * 44, 0.62f, i == cfgSel ? HEX(0xc2ae6a) : HEX(0x8a8170), 0, 0);
    }
    audDiagMake(); txt(audDiag, 16, 176, 0.38f, HEX(audioOK && romOK ? 0x6a9a5a : 0xc05a4a), 290, 0);
    txt("▲▼ escolher   ◀ ▶ mudar   B voltar", 16, 220, 0.45f, HEX(0x6b6352), 0, 0);
    return;
  }
  /* jogo */
  if (mode == M_PLAY) { drawPlayBottom(); return; }
  if (mode == M_SERVE) { drawServeBottom(); return; }
  if (mode == M_KEYPAD) { drawKeypadBottom(); return; }
  if (mode == M_ROLL) { C2D_DrawRectSolid(0, 0, 0, 320, 240, HEX(0x000000)); txt("START: pular", 160, 220, 0.45f, HEX(0x5a5448), 0, C2D_AlignCenter); return; }
  if (mode == M_RPS) { drawRpsBottom(); return; }
  if (mode == M_MASH || mode == M_QTE) { drawMiniBottom(mode == M_QTE); return; }
  if (mode == M_CARD) {
    if (curText && curText[0]) txt(curText, 14, 80, 0.55f, HEX(0x8a8170), 292, 0);
    return;
  }
  if (mode == M_CHOICE) {
    txt(curWho, 14, 12, 0.6f, HEX(0xd8ceb4), 292, 0);
    const char *o[2] = { curText, 0 };
    (void)o;
    return;
  }
  C2D_DrawRectSolid(6, 6, 0, 308, 228, HEX(0x16110d));
  C2D_DrawRectSolid(6, 6, 0, 308, 2, HEX(0x5a4a38)); C2D_DrawRectSolid(6, 232, 0, 308, 2, HEX(0x5a4a38));
  if (mode == M_SAY) txt(curWho, 14, 10, 0.6f, whoColor(curWho), 292, 0);
  int len = (int)strlen(curText), nb = (int)shown;
  if (nb > len) nb = len;
  while (nb < len && (curText[nb] & 0xC0) == 0x80) nb++;
  char buf[1200];
  if (nb > (int)sizeof buf - 1) nb = (int)sizeof buf - 1;
  memcpy(buf, curText, nb); buf[nb] = 0;
  float sc = len > 330 ? 0.42f : len > 220 ? 0.48f : 0.56f;
  txt(buf, 14, mode == M_SAY ? 34 : 14, sc, mode == M_NAR ? HEX(0xc9c4b2) : HEX(0xe6dcc4), 292, 0);
  if (nb >= len && ((int)(clockT * 2) & 1)) txt("▼  A", 276, 216, 0.5f, HEX(0xc2ae6a), 0, 0);
}

static void drawChoice(void) {
  const Node *n = &chap[ni];
  C2D_DrawRectSolid(0, 0, 0, 320, 240, HEX(0x0c0907));
  txt(n->a, 14, 14, 0.55f, HEX(0xd8ceb4), 292, 0);
  const char *o[2] = { chSwap ? n->c : n->b, chSwap ? n->b : n->c };
  for (int i = 0; i < 2; i++) {
    C2D_DrawRectSolid(12, 84 + i * 64, 0, 296, 54, i == sel ? HEX(0x2e2214) : HEX(0x16110d));
    C2D_DrawRectSolid(12, 84 + i * 64, 0, 296, 2, i == sel ? HEX(0xc2ae6a) : HEX(0x5a4a38));
    txt(o[i], 20, 92 + i * 64, 0.5f, i == sel ? HEX(0xffffff) : HEX(0xbdb49c), 280, 0);
  }
}

/* ---------- motor da historia ---------- */
static void toMenu(void) { locId = 0; state = ST_MENU; setMusic(MUS_MENU); artId = 0; }
static const Node *retChap; static int retNi, pushed, exploreDone;
static void gotoChapter(int c);
static void svInit(void); static void exInit(void); static void exInitSeg(int s); static void mashInit(const Node *n); static void qteInit(const Node *n);
static void enter(void) {
  for (;;) {
    const Node *n = &chap[ni];
    switch (n->t) {
      case N_SAY: case N_NAR:
        mode = n->t == N_SAY ? M_SAY : M_NAR;
        curWho = n->t == N_SAY ? n->a : ""; curText = n->t == N_SAY ? n->b : n->a;
        curVoice = voiceFor(curWho); shown = 0; lastBlipB = 0; letters = 0; return;
      case N_CARD:
        setLoc(n->a); mode = M_CARD; curWho = n->a; curText = n->b; cardT = n->n > 0 ? (float)n->n : -1.0f; clockT = 0; return;
      case N_CHOICE: mode = M_CHOICE; chType = 0; chSwap = 0; sel = 0; curWho = n->a; return;
      case N_ART: artId = n->n; clockT = 0; ni++; break;
      case N_MUS: setMusic(n->n); ni++; break;
      case N_FX: if (n->n == FX_SHAKE) { shakeT = 0.5f; sfxPlay(SF_HIT); } else { flashT = 0.9f; sfxPlay(SF_STINGER); } ni++; break;
      case N_SEV: sevCnt = n->n; sevStart = clockT; sfxPlay(SF_SAW); sfxPlay(SF_GORE); flashT = 0.5f; shakeT = 0.4f; ni++; break;
      case N_KEYPAD: mode = M_KEYPAD; kpI = 0; clockT = 0; return;
      case N_STG: stageSet(n->a); ni++; break;
      case N_DIR: stageDir(n->a); ni++; break;
      case N_SFX: sfxPlay(n->n); ni++; break;
      case N_SETF: Fl[n->n] = n->m; ni++; break;
      case N_IFJ: if (Fl[n->m]) ni = n->n; else ni++; break;
      case N_IFN: if (!Fl[n->m]) ni = n->n; else ni++; break;
      case N_JMP: ni = n->n; break;
      case N_IFPTS: if (Fl[0] < n->m) ni = n->n; else ni++; break;
      case N_CHAP: gotoChapter(n->n); return;
      case N_QTEF: mode = M_QTE; qteInit(n); qtFail = n->n; qtFailCh = -1; qtOk = -1; return;
      case N_QTEFC: mode = M_QTE; qteInit(n); qtFail = -1; qtFailCh = n->n; qtOk = -1; return;
      case N_QTES: if (kind < n->m) { ni++; break; } mode = M_QTE; qteInit(n); qtFail = ni + 1; qtFailCh = -1; qtOk = n->n; return;
      case N_CHP: mode = M_CHOICE; chType = 1; chSwap = rand() & 1; sel = 0; curWho = n->a; return;
      case N_CHCH: mode = M_CHOICE; chType = 2; chSwap = 0; sel = 0; curWho = n->a; return;
      case N_SERVE: mode = M_SERVE; svInit(); return;
      case N_MASH: mode = M_MASH; mashInit(n); return;
      case N_ROLL: mode = M_ROLL; clockT = 0; return;
      case N_RPS: mode = M_RPS; rpsRound = n->n; rpsSel = 1; clockT = 0; return;
      case N_QTE: mode = M_QTE; qteInit(n); return;
      case N_PLAY: mode = M_PLAY; exInit(); return;
      case N_PLAYS: mode = M_PLAY; exInitSeg(n->n); return;
      case N_END: if (pushed) { pushed = 0; chap = retChap; ni = retNi; if (exploreDone) { exploreDone = 0; ni++; break; } mode = M_PLAY; return; }
        if (chapIdx < 6) { chap = CHS[++chapIdx]; ni = 0; artId = 0; saveGame(); break; }
        if (chapIdx == 6) { gotoChapter(10); return; }
        toMenu(); return;
    }
  }
}
static void gotoChapter(int c) { chap = CHS[c]; chapIdx = c; ni = 0; artId = 0; pushed = 0; if (c <= 6) saveGame(); enter(); }
static void startChapter(const Node *c, int idx) {
  chap = c; sevCnt = 0; stageOn = 0; chapIdx = idx; ni = 0; artId = 0; pushed = 0; state = ST_GAME; saveGame(); enter();
}
static void advance(void) { ni++; enter(); }

#include "explore.h"
static void updateGame(float dt, u32 kd, u32 kh, touchPosition tp) {
  (void)kh;
  if (mode == M_PLAY) { updatePlay(dt, kd, kh); return; }
  if (mode == M_SERVE) { updateServe(dt, kd, tp); return; }
  if (mode == M_KEYPAD) { updateKeypad(dt, kd, tp); return; }
  if (mode == M_ROLL) { clockT += dt; if (kd & KEY_START || clockT > 56) advance(); return; }
  if (mode == M_RPS) { updateRps(dt, kd, tp); return; }
  if (mode == M_MASH) { updateMash(dt, kd); return; }
  if (mode == M_QTE) { updateQte(dt, kd); return; }
  clockT += dt;
  if (shakeT > 0) shakeT -= dt;
  if (flashT > 0) flashT -= dt;
  if (kd & KEY_SELECT) { toMenu(); return; }
  int press = (kd & (KEY_A | KEY_TOUCH)) != 0;
  if (mode == M_CARD) {
    if (cardT > 0) { cardT -= dt; if (cardT <= 0) advance(); }
    else if (press) advance();
    return;
  }
  if (mode == M_CHOICE) {
    if (kd & (KEY_DOWN | KEY_UP)) sel ^= 1;
    if (kd & KEY_TOUCH) { if (tp.py >= 84 && tp.py < 138) sel = 0; else if (tp.py >= 148 && tp.py < 202) sel = 1; else return; kd |= KEY_A; }
    if (kd & KEY_A) {
      if (chType == 0) { if (sel == 0) kind++; else rage++; advance(); }
      else if (chType == 1) { if ((sel ^ chSwap) == 0) Fl[0]++; advance(); }
      else { if (sel == 0) gotoChapter(chap[ni].n); else advance(); }
    }
    return;
  }
  if (stageOn) { int mv = 0; for (int i = 0; i < 8; i++) if (ACT[i].on && ACT[i].moving) mv = 1;
    if (mv) { if (press) for (int i = 0; i < 8; i++) if (ACT[i].moving) { ACT[i].x = ACT[i].tx; ACT[i].z = ACT[i].tz; ACT[i].moving = 0; } return; } }
  int len = (int)strlen(curText);
  if ((int)shown < len) {
    shown += dt * (cfg.speed == 0 ? 30.0f : cfg.speed == 1 ? 55.0f : 110.0f);
    int nb = (int)shown; if (nb > len) nb = len;
    for (; lastBlipB < nb; lastBlipB++) {
      unsigned char c = (unsigned char)curText[lastBlipB];
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c >= 0xC3) { if (++letters % 2 == 0 && mode == M_SAY) blip(); }
    }
    if (press) shown = (float)len;
  } else if (press) advance();
  (void)utf8len;
}

/* ---------- menus ---------- */
static void startNew(void) { kind = rage = 0; startChapter(CH0, 0); setMusic(MUS_OFF); enter(); }
static void updateMenu(u32 kd, touchPosition tp, int *quit) {
  if (kd & KEY_DOWN) menuSel = (menuSel + 1) % 4;
  if (kd & KEY_UP) menuSel = (menuSel + 3) % 4;
  int go = (kd & KEY_A) != 0;
  if (kd & KEY_TOUCH) for (int i = 0; i < 4; i++) if (tp.py >= 40 + i * 40 && tp.py < 74 + i * 40) { menuSel = i; go = 1; }
  if (kd & KEY_START) *quit = 1;
  if (!go) return;
  if (menuSel == 0) { kind = rage = 0; locId = 0; memset(Fl, 0, sizeof Fl); Fl[1] = (rand() % 10) < 6; setMusic(MUS_OFF); startChapter(CH0, 0); }
  else if (menuSel == 1) { Save s; if (loadSave(&s) && s.ch >= 0 && s.ch <= 6) { kind = s.kind; rage = s.rage; memset(Fl, 0, sizeof Fl); Fl[1] = s.moto; locId = 0; startChapter(CHS[s.ch], s.ch); } }
  else if (menuSel == 2) { state = ST_EXTRAS; extrasPage = 0; }
  else { state = ST_CFG; cfgSel = 0; }
}
static void updateCfg(u32 kd) {
  if (kd & KEY_DOWN) cfgSel = (cfgSel + 1) % 3;
  if (kd & KEY_UP) cfgSel = (cfgSel + 2) % 3;
  int d = ((kd & KEY_RIGHT) ? 1 : 0) - ((kd & KEY_LEFT) ? 1 : 0);
  if (d) {
    if (cfgSel == 0) { cfg.music += d; if (cfg.music < 0) cfg.music = 0; if (cfg.music > 10) cfg.music = 10; }
    if (cfgSel == 1) { cfg.voice += d; if (cfg.voice < 0) cfg.voice = 0; if (cfg.voice > 10) cfg.voice = 10; curVoice = 240; blip(); }
    if (cfgSel == 2) { cfg.speed += d; if (cfg.speed < 0) cfg.speed = 0; if (cfg.speed > 2) cfg.speed = 2; }
    applyVolumes();
  }
  if (kd & KEY_B) { saveCfg(); state = ST_MENU; }
}

int main(int argc, char **argv) {
  (void)argc; (void)argv;
  gfxInitDefault();
  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(16000);
  C2D_Prepare();
  top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
  topR = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
  bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
  tbuf = C2D_TextBufNew(16384);
  loadCfg();
  audioInit();
  setMusic(MUS_MENU);
  applyVolumes();
  srand((unsigned)osGetTime());
  mode = M_NAR; curWho = ""; curText = "";
  u64 last = osGetTime();
  int quit = 0;
  while (aptMainLoop() && !quit) {
    hidScanInput();
    u32 kd = hidKeysDown(), kh = hidKeysHeld();
    touchPosition tp; hidTouchRead(&tp);
    u64 now = osGetTime(); float dt = (now - last) / 1000.0f; last = now; if (dt > 0.1f) dt = 0.1f; stageUpdate(dt);

    if (state == ST_GAME) updateGame(dt, kd, kh, tp);
    else {
      clockT += dt;
      if (state == ST_MENU) updateMenu(kd, tp, &quit);
      else if (state == ST_CFG) updateCfg(kd);
      else { if (kd & KEY_B) state = ST_MENU; if (kd & (KEY_LEFT | KEY_RIGHT)) extrasPage ^= 1; }
    }

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TextBufClear(tbuf);
    C2D_TargetClear(top, HEX(0x000000));
    C2D_TargetClear(bot, HEX(0x000000));
    { float sl = osGet3DSliderState(); int st3 = sl > .03f; gfxSet3D(st3);
      if (st3) { /* 3D estereoscopico: cada olho com a camera deslocada */
        unsigned fs = (unsigned)rand(); C2D_TargetClear(topR, HEX(0x000000));
        g3Eye = -.14f * sl; srand(fs); C2D_SceneBegin(top); drawTop();
        g3Eye = .14f * sl; srand(fs); C2D_SceneBegin(topR); drawTop(); g3Eye = 0;
      } else { C2D_SceneBegin(top); drawTop(); } }
    C2D_SceneBegin(bot);
    if (state == ST_GAME && mode == M_CHOICE) drawChoice(); else drawBottom();
    C3D_FrameEnd(0);
  }
  if (audioOK) { ndspChnWaveBufClear(0); ndspChnWaveBufClear(1); linearFree(pcmBlip); linearFree(pcmMus); ndspExit(); romfsExit(); }
  C2D_TextBufDelete(tbuf);
  C2D_Fini(); C3D_Fini(); gfxExit();
  return 0;
}
