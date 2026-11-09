/* Modos jogaveis do 3DS: exploracao 2D da casa, minigame do balcao, apertar botao e QTE.
   Incluido por main.c depois das funcoes de desenho basicas. */
#ifndef EXPLORE_H
#define EXPLORE_H
#include "script.h"

/* ---- acoes (dialogos curtos que voltam para a exploracao) ---- */
static const Node A_GAVETA_TRANC[] = { NAR("A gaveta está trancada. A fechadura é pequena, precisa de uma chavinha."), END() };
static const Node A_GAVETA_OK[]    = { NAR("A chavinha gira. Dentro da gaveta, luvas de borracha e um rolo de fita adesiva."), END() };
static const Node A_BANCADA[]      = { NAR("A gaveta da bancada está emperrada."), MASH("ABRIR A GAVETA !", 8), NAR("LUCAS força a gaveta até ela ceder. Lá dentro, um serrote enferrujado."), END() };
static const Node A_QUADRO[]       = { NAR("Entre várias chaves penduradas, uma com etiqueta escrita à mão: “QUARTO”."), END() };
static const Node A_ESCRIV[]       = { NAR("Embaixo de um caderno de capa preta, uma chave grande e pesada: a chave do porão. No caderno, a letra do MATEUS: “ninguém pode saber”."), END() };
static const Node A_SOFA[]         = { NAR("Entre as almofadas do sofá, LUCAS encontra uma chavinha pequena, daquelas de gaveta."), END() };
static const Node A_QUARTO_TRANC[] = { NAR("A porta lá em cima está trancada."), END() };
static const Node A_PORAO_TRANC[]  = { NAR("A porta do porão está trancada. Faltam o serrote, as luvas e a chave."), END() };
static const Node A_PORAO_OK[]     = { NAR("A chave gira devagar. A porta do porão se abre sozinha, soltando um cheiro forte de produto químico."), END() };
static const Node A_TV[]           = { ART(ART_BAILE), NAR("LUCAS liga a TV. Passa a reprise de um filme adolescente: um baile escolar, luzes roxas e casais dançando devagar, ao som de uma balada em 16 bits."),
  SAY("LUCAS (rindo baixo)", "Nossa… que clichê. Purple Rain em um baile escolar, com tudo roxo."), NAR("Por um instante, o barulho da casa some. Ele quase sorri. Depois a TV chia e ele desliga."), ART(ART_NONE), END() };

enum { I_SERROTE = 1, I_LUVAS = 2, I_FITA = 4, I_CQ = 8, I_CG = 16, I_CP = 32 };
enum { H_TV = 1, H_SOFA, H_ESCADA, H_PORAO, H_GAVETA, H_BANCADA, H_QUADRO, H_ESCRIV };
typedef struct { int room; float x, z, rad; const char *lbl; int id; } Hot;
/* salas: 1 sala, 2 porao, 3 garagem, 4 cozinha, 5 banheiro, 6 quarto — posicoes no mundo (iguais ao jogo de PC) */
static const Hot HOTS[] = {
  { 1, -.6f, -1.9f, 1.7f, "Ver a televisão", H_TV }, { 1, 1.2f, .9f, 1.5f, "Procurar no sofá", H_SOFA }, { 1, 4.0f, 1.2f, 1.2f, "Destrancar a porta do porão", H_PORAO },
  { 4, 0, -1.9f, 1.7f, "Abrir a gaveta", H_GAVETA }, { 3, 2.4f, -2.3f, 1.8f, "Revirar a bancada", H_BANCADA }, { 3, -2.8f, -2.3f, 1.8f, "Quadro de chaves", H_QUADRO }, { 6, -2.2f, -1.7f, 1.8f, "Revirar a escrivaninha", H_ESCRIV },
};
static int exRoom, inv, tvDone;
static float wx, wz, doorLock;
static const char *exMsg; static float exMsgT;

static void enter(void);
static void pushNodes(const Node *a) { if (!pushed) { retChap = chap; retNi = ni; } pushed = 1; sfxPlay(SF_PICK); chap = a; ni = 0; enter(); }
static int exSeg = -1, segSt, segUsed; static const char *segObj = "";
static void exInit(void) { exSeg = -1; plWho = 0; plHold = 0; exRoom = 1; inv = 0; tvDone = 0; wx = RMS[1].sx; wz = RMS[1].sz; doorLock = 1; pushed = 0; exploreDone = 0; locId = 1; }
static void exInitSeg(int s) {
  const PSeg *g = &PSEG[s]; exSeg = s; segSt = 0; segUsed = 0; segObj = g->obj; inv = 0; tvDone = 0; pushed = 0; exploreDone = 0; stageOn = 0;
  exRoom = g->room; locId = g->room; wx = g->hasStart ? g->sx : RMS[g->room].sx; wz = g->hasStart ? g->sz : RMS[g->room].sz; doorLock = 1; plMove = 0; plYaw = 0;
  int w = stWho(g->who); plWho = w < 0 ? 0 : w; plHold = stHold(g->hold);
}
static int pVis(int i) { const PHot *h = &PSEG[exSeg].hots[i]; if (h->room != exRoom) return 0; if (h->once && (segUsed & (1 << i))) return 0; if (h->need >= 0 && segSt != h->need) return 0; return 1; }
static int pNear(void) { const PSeg *g = &PSEG[exSeg]; for (int i = 0; i < g->nh; i++) if (pVis(i) && hypotf(wx - g->hots[i].x, wz - g->hots[i].z) < g->hots[i].rad) return i; return -1; }
static void pDo(int i) {
  const PHot *h = &PSEG[exSeg].hots[i];
  if (h->once) segUsed |= 1 << i; if (h->set >= 0) segSt = h->set; if (h->obj && h->obj[0]) segObj = h->obj;
  if (h->act) { if (h->done) exploreDone = 1; pushNodes(h->act); }
  else if (h->done) { sfxPlay(SF_PICK); ni++; enter(); }
  else sfxPlay(SF_PICK);
}
static void drawSegNpcs(void) {
  if (exSeg < 0) return; const PSeg *g = &PSEG[exSeg];
  for (int i = 0; i < g->nn; i++) { const PNpc *n = &g->npcs[i]; if (!n->name || n->room != exRoom) continue; int w = stWho(n->name); if (w < 0) continue;
    g3Hold = stHold(n->hold); g3_char(w, n->x, n->z, n->yaw, stPose(n->pose), clockT, 1); g3Hold = 0; }
}
static int hotVisible(const Hot *h) {
  switch (h->id) {
    case H_TV: return !tvDone;
    case H_SOFA: return !(inv & I_CG);
    case H_GAVETA: return !(inv & I_LUVAS);
    case H_BANCADA: return !(inv & I_SERROTE);
    case H_QUADRO: return !(inv & I_CQ);
    case H_ESCRIV: return !(inv & I_CP);
  }
  return 1;
}
static const Hot *nearHot(void) {
  for (unsigned i = 0; i < sizeof HOTS / sizeof *HOTS; i++) { const Hot *h = &HOTS[i]; if (h->room == exRoom && hotVisible(h) && hypotf(wx - h->x, wz - h->z) < h->rad) return h; }
  return 0;
}
static void doHot(const Hot *h) {
  switch (h->id) {
    case H_TV: tvDone = 1; pushNodes(A_TV); break;
    case H_SOFA: inv |= I_CG; pushNodes(A_SOFA); break;
    case H_GAVETA: if (inv & I_CG) { inv |= I_LUVAS | I_FITA; pushNodes(A_GAVETA_OK); } else pushNodes(A_GAVETA_TRANC); break;
    case H_BANCADA: inv |= I_SERROTE; pushNodes(A_BANCADA); break;
    case H_QUADRO: inv |= I_CQ; pushNodes(A_QUADRO); break;
    case H_ESCRIV: inv |= I_CP; pushNodes(A_ESCRIV); break;
    case H_PORAO: if ((inv & I_CP) && (inv & I_SERROTE) && (inv & I_LUVAS)) { exploreDone = 1; pushNodes(A_PORAO_OK); } else pushNodes(A_PORAO_TRANC); break;
  }
}
static int exHit(const Rm *r, float x, float z) {
  const float rad = .3f;
  for (int i = 0; i < r->ncols; i++) { const float *c = r->cols[i]; if (fabsf(x - c[0]) < c[2] / 2 + rad && fabsf(z - c[1]) < c[3] / 2 + rad) return 1; }
  return 0;
}
static void updatePlay(float dt, u32 kd, u32 kh) {
  clockT += dt;
  circlePosition cp; hidCircleRead(&cp);
  float vx = cp.dx / 156.0f, vy = -cp.dy / 156.0f;
  if (fabsf(vx) < .12f) vx = 0; if (fabsf(vy) < .12f) vy = 0;
  if (kh & KEY_LEFT) vx = -1; if (kh & KEY_RIGHT) vx = 1; if (kh & KEY_UP) vy = -1; if (kh & KEY_DOWN) vy = 1;
  float l = sqrtf(vx * vx + vy * vy); if (l > 1) { vx /= l; vy /= l; }
  const Rm *r = &RMS[exRoom]; float bw = r->w / 2 - .5f, bd = r->d / 2 - .5f;
  float nx = wx + vx * 3.2f * dt, nz = wz + vy * 3.2f * dt;
  if (nx < -bw) nx = -bw; if (nx > bw) nx = bw; if (nz < -bd) nz = -bd; if (nz > bd) nz = bd;
  if (!exHit(r, nx, wz)) wx = nx; if (!exHit(r, wx, nz)) wz = nz;
  plMove = (vx != 0 || vy != 0); if (plMove) plYaw = atan2f(vx, vy);
  /* portas */
  int nearDoor = 0;
  for (int i = 0; i < r->ndr; i++) {
    const Dr *d = &r->dr[i]; if (d->to == 0 || d->to == 2) continue; if (exSeg >= 0 && !(PSEG[exSeg].allow & (1 << d->to))) continue;
    if (hypotf(wx - d->x, wz - d->z) < .85f) {
      nearDoor = 1;
      if (doorLock) break;
      doorLock = 1;
      if (exSeg < 0 && d->to == 6 && !(inv & I_CQ)) { pushNodes(A_QUARTO_TRANC); return; }
      exRoom = d->to; locId = d->to; wx = d->ax; wz = d->az; sfxPlay(SF_DOOR); break;
    }
  }
  if (!nearDoor) {
    int away = 1; for (int i = 0; i < r->ndr; i++) if (hypotf(wx - r->dr[i].x, wz - r->dr[i].z) < 1.0f) away = 0;
    if (away) doorLock = 0;
  }
  if (kd & KEY_A) { if (exSeg >= 0) { int k = pNear(); if (k >= 0) pDo(k); } else { const Hot *h = nearHot(); if (h) doHot(h); } }
  if (exMsgT > 0) exMsgT -= dt;
  if (kd & KEY_SELECT) { if (exMsgT > 0) toMenu(); else { exMsgT = 2.5f; exMsg = "Aperte SELECT de novo para voltar ao menu"; } }
}
static void drawPlayTop(void) {
  const Rm *r = &RMS[exRoom];
  plX = wx; plZ = wz;
  C2D_DrawRectSolid(0, 0, 0, 400, 240, HEX(0x000000));
  castMode = 2; scene3(exRoom); castMode = 0;
  { int nn = exSeg >= 0 ? PSEG[exSeg].nh : (int)(sizeof HOTS / sizeof *HOTS);
  for (int i = 0; i < nn; i++) { float hx, hz; if (exSeg >= 0) { if (!pVis(i)) continue; hx = PSEG[exSeg].hots[i].x; hz = PSEG[exSeg].hots[i].z; } else { const Hot *h = &HOTS[i]; if (!(h->room == exRoom && hotVisible(h))) continue; hx = h->x; hz = h->z; }
    float b = sinf(clockT * 5) * .06f;
    float v[3] = { hx - g3E[0], 1.9f + b - g3E[1], hz - g3E[2] };
    float xc = v[0]*g3Rt[0] + v[1]*g3Rt[1] + v[2]*g3Rt[2], yc = v[0]*g3Up[0] + v[1]*g3Up[1] + v[2]*g3Up[2], zc = v[0]*g3F[0] + v[1]*g3F[1] + v[2]*g3F[2];
    if (zc > .3f) { float sx = g3Cx + xc / zc * g3Fl, sy = g3Cy - yc / zc * g3Fl; C2D_DrawRectSolid(sx - 5, sy - 5, 0, 10, 10, HEX(0x1a1408)); C2D_DrawRectSolid(sx - 4, sy - 4, 0, 8, 8, HEX(0xe8c040)); } } }
  (void)r;
  C2D_DrawRectSolid(0, 0, 0, 40, 240, HEX(0x000000)); C2D_DrawRectSolid(360, 0, 0, 40, 240, HEX(0x000000));
}
static void drawPlayBottom(void) {
  C2D_DrawRectSolid(0, 0, 0, 320, 240, HEX(0x0c0907));
  if (exMsgT > 0 && exMsg) { C2D_DrawRectSolid(8, 150, 0, 304, 30, HEX(0x4a1a14)); txt(exMsg, 16, 157, 0.5f, HEX(0xffffff), 290, 0); }
  if (exSeg >= 0) {
    txt("Objetivo:", 12, 10, 0.5f, HEX(0x8a8170), 0, 0); txt(segObj, 12, 30, 0.55f, HEX(0xc2ae6a), 296, 0);
    int k = pNear(); if (k >= 0) { C2D_DrawRectSolid(8, 196, 0, 304, 30, HEX(0x2e2214)); char b[96]; snprintf(b, sizeof b, "A: %s", PSEG[exSeg].hots[k].lbl); txt(b, 16, 202, 0.6f, HEX(0xffffff), 0, 0); }
    else txt("Circle Pad / direcional: andar     SELECT: menu", 12, 222, 0.4f, HEX(0x6b6352), 0, 0);
    return;
  }
  txt("Procure as ferramentas e a chave do porão", 12, 10, 0.5f, HEX(0xc2ae6a), 296, 0);
  static const char *nm[6] = { "Serrote", "Luvas", "Fita", "Chave do quarto", "Chave pequena", "Chave do porão" };
  txt("Itens:", 12, 56, 0.5f, HEX(0x8a8170), 0, 0);
  int y = 76;
  for (int i = 0; i < 6; i++) if (inv & (1 << i)) { txt(nm[i], 20, y, 0.5f, HEX(0xd8ceb4), 0, 0); y += 18; }
  if (!inv) txt("nenhum", 20, 76, 0.5f, HEX(0x6b6352), 0, 0);
  const Hot *h = nearHot();
  if (h) { C2D_DrawRectSolid(8, 196, 0, 304, 30, HEX(0x2e2214)); char b[80]; snprintf(b, sizeof b, "A: %s", h->lbl); txt(b, 16, 202, 0.6f, HEX(0xffffff), 0, 0); }
  else txt("Circle Pad / direcional: andar     SELECT: menu", 12, 222, 0.4f, HEX(0x6b6352), 0, 0);
}

/* ---- minigame do balcao ---- */
static const char *SV_IT[6] = { "Hambúrguer", "Cheddar", "Batata frita", "Refrigerante", "Sorvete", "Nuggets" };
static const char *SV_NA[8] = { "Dona Marta", "Seu Jorge", "Tiago", "Camila", "Um senhor apressado", "Uma criança", "Rapaz de fone", "Dona Neusa" };
static int svSel, svOrder, svCust, svServed;
static float svT;
static void svNew(void) { int n = 2 + rand() % 2; svOrder = 0; while (__builtin_popcount(svOrder) < n) svOrder |= 1 << (rand() % 6); svSel = 0; svCust = rand() % 8; svT = 16.0f; }
static void svInit(void) { svServed = 0; svNew(); }
static void svDeliver(void) { if (svSel == svOrder) svServed++; else shakeT = 0.3f; svNew(); }
static void updateServe(float dt, u32 kd, touchPosition tp) {
  clockT += dt; svT -= dt; if (svT <= 0) { shakeT = 0.3f; svNew(); }
  if (shakeT > 0) shakeT -= dt;
  if (kd & KEY_TOUCH) {
    for (int i = 0; i < 6; i++) { int bx = 10 + (i % 2) * 154, by = 6 + (i / 2) * 42; if (tp.px >= bx && tp.px < bx + 146 && tp.py >= by && tp.py < by + 38) svSel ^= 1 << i; }
    if (tp.py >= 136 && tp.py < 180) svDeliver();
    else if (tp.py >= 188 && tp.py < 232 && svServed >= 3) { advance(); return; }
  }
  if (kd & KEY_A) svDeliver();
  if ((kd & KEY_X) && svServed >= 3) advance();
}
static void drawServeTop(void) {
  C2D_DrawRectSolid(0, 0, 0, 400, 240, HEX(0x000000)); drawRoom(8);
  R(0, 78, 160, 8, 0x8a8478); person(60, 80, 1.4f, 0, 0x7a8a6a, 0x332211);
  C2D_DrawRectSolid(60, 6, 0, 280, 90, HEX(0x16110d));
  char b[160]; snprintf(b, sizeof b, "%s pede:", SV_NA[svCust]); txt(b, 70, 10, 0.6f, HEX(0xd8ceb4), 0, 0);
  b[0] = 0; for (int i = 0; i < 6; i++) if (svOrder & (1 << i)) { strcat(b, SV_IT[i]); strcat(b, "   "); }
  txt(b, 70, 36, 0.6f, HEX(0xffe080), 260, 0);
  snprintf(b, sizeof b, "Atendidos: %d%s", svServed, svServed >= 3 ? "  (pode sair)" : " / 3"); txt(b, 70, 74, 0.5f, HEX(0x8a8170), 0, 0);
  C2D_DrawRectSolid(60, 98, 0, 280 * (svT / 16.0f), 6, HEX(0xa8301f));
}
static void drawServeBottom(void) {
  C2D_DrawRectSolid(0, 0, 0, 320, 240, HEX(0x0c0907));
  for (int i = 0; i < 6; i++) { int bx = 10 + (i % 2) * 154, by = 6 + (i / 2) * 42; C2D_DrawRectSolid(bx, by, 0, 146, 38, (svSel >> i) & 1 ? HEX(0x3a5a2a) : HEX(0x241a10)); txt(SV_IT[i], bx + 8, by + 8, 0.55f, HEX(0xe6dcc4), 0, 0); }
  C2D_DrawRectSolid(10, 136, 0, 300, 44, HEX(0x5a3a14)); txt("ENTREGAR   (A)", 90, 148, 0.7f, HEX(0xffffff), 0, 0);
  if (svServed >= 3) { C2D_DrawRectSolid(10, 188, 0, 300, 44, HEX(0x4a1a1a)); txt("Ir para a sala dos fundos  (X)", 40, 202, 0.55f, HEX(0xffffff), 0, 0); }
}

/* ---- apertar botao (mash) e QTE ---- */
static int mhCnt, mhNeed, qtKey; static float mhT, qtT; static const char *mhLabel;
static const char *QK[4] = { "A", "B", "X", "Y" };
static const u32 QKM[4] = { KEY_A, KEY_B, KEY_X, KEY_Y };
static void mashInit(const Node *n) { mhLabel = n->a; mhNeed = n->n ? n->n : 8; mhCnt = 0; mhT = 4.5f; }
static void qteInit(const Node *n) { mhLabel = n->a; qtKey = rand() % 4; qtT = 3.0f; qtFail = -1; qtFailCh = -1; qtOk = -1; }
static void updateMash(float dt, u32 kd) { clockT += dt; mhT -= dt; if (mhT <= 0) { mhCnt = 0; mhT = 4.5f; } if (kd & KEY_A) { if (++mhCnt >= mhNeed) advance(); } }
static void qteFailed(void) {
  if (qtFail >= 0) { ni = qtFail; enter(); return; }
  if (qtFailCh >= 0) { gotoChapter(qtFailCh); return; }
  qtKey = rand() % 4; qtT = 3.0f; shakeT = 0.3f;
}
static void updateQte(float dt, u32 kd) {
  clockT += dt; qtT -= dt;
  if (shakeT > 0) shakeT -= dt;
  if (qtT <= 0) { qteFailed(); return; }
  if (kd & (KEY_A | KEY_B | KEY_X | KEY_Y)) { if (kd & QKM[qtKey]) { if (qtOk >= 0) gotoChapter(qtOk); else advance(); } else qteFailed(); }
}
static void drawMiniTop(int isQte) {
  C2D_DrawRectSolid(0, 0, 0, 400, 240, HEX(0x000000));
  if (locId) drawRoom(locId);
  C2D_DrawRectSolid(0, 0, 0, 400, 240, C2D_Color32(0, 0, 0, 150));
  txt(mhLabel, 200, 40, 0.9f, HEX(0xffe080), 0, C2D_AlignCenter);
  if (isQte) { txt(QK[qtKey], 200, 90, 2.4f, HEX(0xffffff), 0, C2D_AlignCenter); C2D_DrawRectSolid(60, 200, 0, 280 * (qtT / 3.0f), 8, HEX(0xa8301f)); }
  else { C2D_DrawRectSolid(60, 120, 0, 280, 16, HEX(0x241a10)); C2D_DrawRectSolid(60, 120, 0, 280.0f * mhCnt / mhNeed, 16, HEX(0xc2ae6a)); C2D_DrawRectSolid(60, 200, 0, 280 * (mhT / 4.5f), 6, HEX(0xa8301f)); }
}
static void drawMiniBottom(int isQte) {
  C2D_DrawRectSolid(0, 0, 0, 320, 240, HEX(0x0c0907));
  txt(isQte ? "Aperte o botão mostrado na tela de cima !" : "Aperte A várias vezes !", 160, 100, 0.6f, HEX(0xd8ceb4), 0, C2D_AlignCenter);
}
/* ---- Pedra, Papel ou Tesoura ---- */
static const char *RPSN[3] = { "Pedra", "Papel", "Tesoura" };
static char rpsMsg[96];
static void updateRps(float dt, u32 kd, touchPosition tp) {
  clockT += dt; int pick = -1;
  if (kd & KEY_DLEFT) rpsSel = (rpsSel + 2) % 3; if (kd & KEY_DRIGHT) rpsSel = (rpsSel + 1) % 3;
  if (kd & KEY_A) pick = rpsSel;
  if (kd & KEY_TOUCH) { for (int i = 0; i < 3; i++) if (tp.px >= 10 + i * 104 && tp.px < 106 + i * 104 && tp.py >= 50 && tp.py < 190) pick = i; }
  if (pick < 0) return;
  snprintf(rpsMsg, sizeof rpsMsg, "Você: %s   ·   Mateus: %s   —   empate.", RPSN[pick], RPSN[pick]);
  mode = M_NAR; curWho = ""; curText = rpsMsg; curVoice = 0; shown = 0; lastBlipB = 0; letters = 0;
}
static void drawRpsTop(void) {
  C2D_DrawRectSolid(0, 0, 0, 400, 240, HEX(0x000000));
  if (locId) { castMode = 1; drawRoom(locId); castMode = 0; }
  C2D_DrawRectSolid(0, 0, 0, 400, 240, C2D_Color32(0, 0, 0, 130));
  char b[64]; snprintf(b, sizeof b, "Pedra, Papel ou Tesoura !  (rodada %d)", rpsRound + 1);
  txt(b, 200, 90, 0.85f, HEX(0xffe080), 0, C2D_AlignCenter);
  txt("Escolha na tela de baixo", 200, 130, 0.6f, HEX(0xd8ceb4), 0, C2D_AlignCenter);
}
static void drawRpsBottom(void) {
  C2D_DrawRectSolid(0, 0, 0, 320, 240, HEX(0x0c0907));
  for (int i = 0; i < 3; i++) {
    C2D_DrawRectSolid(10 + i * 104, 50, 0, 96, 140, HEX(i == rpsSel ? 0x6a4a1c : 0x241a10));
    C2D_DrawRectSolid(14 + i * 104, 54, 0, 88, 132, HEX(i == rpsSel ? 0x3a2a14 : 0x16100a));
    txt(RPSN[i], 58 + i * 104, 100, 0.8f, HEX(0xffe080), 0, C2D_AlignCenter);
  }
  txt("Toque ou use ← → e A", 160, 205, 0.5f, HEX(0x8a8170), 0, C2D_AlignCenter);
}
/* ---- creditos rolando ---- */
static const char *ROLLT[][2] = {
  { "QUERO ACABAR LOGO COM ISSO", "T" }, { "Roteiro", "" }, { "Arthur Abreu", "N" }, { "Direção", "" }, { "Arthur Abreu", "N" }, { "Programação", "" }, { "Heitor S", "N" },
  { "Elenco", "" }, { "Lucas", "N" }, { "Mateus", "N" }, { "Marcos", "N" }, { "Dona Lúcia", "N" },
  { "Músicas", "" }, { "“Enslaved by Machinery”", "N" }, { "“Bullwinkle, Pt. II”", "N" }, { "“Hey” — Pixies", "N" }, { "“Isn’t She Lovely” — Stevie Wonder", "N" }, { "“You’re My Best Friend” — Queen", "N" }, { "“Please Don’t Go”", "N" },
  { "Obrigado por jogar", "T" } };
static void drawRollTop(void) {
  C2D_DrawRectSolid(0, 0, 0, 400, 240, HEX(0x050403));
  float y = 250 - clockT * 12.0f;
  for (unsigned i = 0; i < sizeof ROLLT / sizeof *ROLLT; i++) {
    int big = ROLLT[i][1][0] == 'T', nm = ROLLT[i][1][0] == 'N';
    if (!nm && !big) y += 14;
    if (y > -20 && y < 250) txt(ROLLT[i][0], 200, y, big ? 0.8f : nm ? 0.6f : 0.45f, big ? HEX(0xe8c040) : nm ? HEX(0xd8ceb4) : HEX(0x8a8170), 0, C2D_AlignCenter);
    y += big ? 40 : nm ? 20 : 18;
  }
}
/* ---- telefone (teclado numerico): disque 1 9 0 e LIGAR ---- */
static const char KPK[12] = { '1','2','3','4','5','6','7','8','9','*','0','#' };
static const char KPSEQ[3] = { '1', '9', '0' };
static void updateKeypad(float dt, u32 kd, touchPosition tp) {
  clockT += dt;
  if (!(kd & KEY_TOUCH)) return;
  if (kpI >= 3) { if (tp.px >= 80 && tp.px < 240 && tp.py >= 190 && tp.py < 232) { sfxPlay(SF_RING); advance(); } return; }
  for (int i = 0; i < 12; i++) {
    int bx = 70 + (i % 3) * 64, by = 20 + (i / 3) * 40;
    if (tp.px >= bx && tp.px < bx + 60 && tp.py >= by && tp.py < by + 36) { if (KPK[i] == KPSEQ[kpI]) { kpI++; sfxPlay(SF_DIAL); } else sfxPlay(SF_HIT); }
  }
}
static void drawKeypadTop(void) {
  C2D_DrawRectSolid(0, 0, 0, 400, 240, HEX(0x000000));
  if (locId) { castMode = 1; drawRoom(locId); castMode = 0; }
  C2D_DrawRectSolid(0, 0, 0, 400, 240, C2D_Color32(0, 0, 0, 170));
  txt("Telefone fixo", 200, 60, 0.9f, HEX(0xffe080), 0, C2D_AlignCenter);
  char b[8] = ""; for (int i = 0; i < kpI; i++) { b[i] = KPSEQ[i]; b[i + 1] = 0; }
  txt(b, 200, 110, 1.6f, HEX(0x9fd08a), 0, C2D_AlignCenter);
  txt("Disque 1 · 9 · 0 na tela de baixo", 200, 170, 0.55f, HEX(0xd8ceb4), 0, C2D_AlignCenter);
}
static void drawKeypadBottom(void) {
  C2D_DrawRectSolid(0, 0, 0, 320, 240, HEX(0x0c0907));
  for (int i = 0; i < 12; i++) {
    int bx = 70 + (i % 3) * 64, by = 20 + (i / 3) * 40; char k[2] = { KPK[i], 0 };
    C2D_DrawRectSolid(bx, by, 0, 60, 36, HEX(0x6a4a1c)); C2D_DrawRectSolid(bx + 3, by + 3, 0, 54, 30, HEX(0x241a10));
    txt(k, bx + 30, by + 4, 0.8f, HEX(0xffe080), 0, C2D_AlignCenter);
  }
  if (kpI >= 3) { C2D_DrawRectSolid(80, 190, 0, 160, 42, HEX(0x1a4a1a)); txt("LIGAR", 160, 198, 0.9f, HEX(0xffffff), 0, C2D_AlignCenter); }
}
#endif
