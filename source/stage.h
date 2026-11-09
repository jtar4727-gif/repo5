/* Palco: posicoes, movimentos, poses, gestos e objetos nas maos dos personagens (como o dir() do PC) */
#pragma once
enum { HD_NONE, HD_SAW, HD_AXE, HD_MACHETE, HD_CHAIN, HD_GUN, HD_PHONE, HD_KNIFE };
enum { GS_NONE, GS_HANDS, GS_WIPE, GS_UP, GS_SWING, GS_SAW, GS_CHEER, GS_DANCE, GS_WAVE, GS_CROSS, GS_RECOIL, GS_POINT, GS_NOD, GS_LAUGH };
typedef struct { int on, who; char key; float x, z, yaw, tx, tz; int moving, pose, hold, gest; float gT, gD, fade, off; } Act;
static Act ACT[8]; static int stageOn;
static int stRoomId(const char *s) {
  static const struct { const char *n; int id; } T[] = { {"sala",1},{"porao",2},{"garagem",3},{"cozinha",4},{"banheiro",5},{"quarto",6},{"rua",7},{"apto",9} };
  for (unsigned i = 0; i < sizeof T / sizeof *T; i++) if (!strcmp(s, T[i].n)) return T[i].id;
  return 0;
}
static int stWho(const char *n) { for (int i = 0; i < NCS; i++) if (!strcmp(CSN[i], n)) return i; return -1; }
static int stPose(const char *s) {
  if (!strcmp(s, "walk")) return 1; if (!strcmp(s, "sit")) return 2; if (!strcmp(s, "cry")) return 3; if (!strcmp(s, "dead")) return 4;
  if (!strcmp(s, "rage")) return 5; if (!strcmp(s, "aim")) return 6; return 0;
}
static int stHold(const char *s) {
  static const char *N[] = { "", "saw", "axe", "machete", "chainsaw", "gun", "phone", "knife" };
  for (int i = 1; i < 8; i++) if (!strcmp(s, N[i])) return i; return 0;
}
static int stFam(int w) { if (w < 0) return -1; if (!strncmp(CSN[w], "lucas", 5)) return 0; if (!strncmp(CSN[w], "mateus", 6)) return 1; if (!strncmp(CSN[w], "marcos", 6)) return 2; return w; }
static void stageSet(const char *spec) {
  char buf[512]; strncpy(buf, spec, sizeof buf - 1); buf[sizeof buf - 1] = 0;
  memset(ACT, 0, sizeof ACT); stageOn = 1;
  char *p = buf; char *bar = strchr(p, '|'); if (bar) *bar = 0;
  int rid = stRoomId(p); if (rid) locId = rid;
  int n = 0;
  while (bar && n < 8) {
    p = bar + 1; bar = strchr(p, '|'); if (bar) *bar = 0;
    char *f[7] = { "", "", "", "", "", "", "" }; int k = 0; char *q = p;
    while (k < 7) { f[k++] = q; char *c = strchr(q, ','); if (!c) break; *c = 0; q = c + 1; }
    int w = stWho(f[1]); if (w < 0) continue;
    Act *a = &ACT[n++]; a->on = 1; a->key = f[0][0]; a->who = w; a->x = atof(f[2]); a->z = atof(f[3]); a->yaw = atof(f[4]); a->pose = stPose(f[5]); a->hold = stHold(f[6]); a->fade = 1; a->off = n * 1.7f;
  }
}
static Act *stFind(char k) { for (int i = 0; i < 8; i++) if (ACT[i].on && ACT[i].key == k) return &ACT[i]; return 0; }
static void stageDir(const char *spec) {
  if (!stageOn) { /* ainda sem palco: so efeitos */ }
  char buf[256]; strncpy(buf, spec, sizeof buf - 1); buf[sizeof buf - 1] = 0;
  for (char *cmd = strtok(buf, ";"); cmd; cmd = strtok(NULL, ";")) {
    while (*cmd == ' ') cmd++;
    if (!strncmp(cmd, "sfx=", 4)) { for (int i = 1; i < SF_N; i++) if (SFN[i] && !strcmp(cmd + 4, SFN[i])) sfxPlay(i); continue; }
    if (!strcmp(cmd, "shake")) { shakeT = .5f; continue; }
    if (!strcmp(cmd, "flash")) { flashT = .9f; continue; }
    if (!strncmp(cmd, "w=", 2)) continue;
    Act *a = stFind(cmd[0]); if (!a) continue;
    if (cmd[1] == '>') {
      const char *c = cmd + (cmd[2] == '>' ? 3 : 2); a->tx = atof(c); const char *cm = strchr(c, ','); if (!cm) continue; a->tz = atof(cm + 1); a->moving = 1; continue;
    }
    if (cmd[1] != '.') continue;
    char *at = cmd + 2, *eq = strchr(at, '='); const char *v = ""; if (eq) { *eq = 0; v = eq + 1; }
    if (!strcmp(at, "hold")) a->hold = stHold(v);
    else if (!strcmp(at, "pose")) a->pose = stPose(v);
    else if (!strcmp(at, "look")) { Act *o = stFind(v[0]); if (o) a->yaw = atan2f(o->x - a->x, o->z - a->z); }
    else if (!strcmp(at, "rot")) a->yaw = atof(v);
    else if (!strcmp(at, "fade")) a->fade = atof(v) > .5f ? 1 : 0;
    else if (!strcmp(at, "hold2") || !strcmp(at, "face")) { }
    else {
      static const struct { const char *n; int g; } G[] = { {"hands",GS_HANDS},{"wipe",GS_WIPE},{"up",GS_UP},{"swing",GS_SWING},{"saw",GS_SAW},{"cheer",GS_CHEER},{"dance",GS_DANCE},{"wave",GS_WAVE},{"cross",GS_CROSS},{"recoil",GS_RECOIL},{"point",GS_POINT},{"nod",GS_NOD},{"laugh",GS_LAUGH} };
      for (unsigned i = 0; i < sizeof G / sizeof *G; i++) if (!strcmp(at, G[i].n)) { a->gest = G[i].g; a->gT = 0; a->gD = *v ? atof(v) : 2.0f; if (a->gD > 30) a->gD = 30; }
    }
  }
}
static void stageUpdate(float dt) {
  if (!stageOn) return;
  for (int i = 0; i < 8; i++) { Act *a = &ACT[i]; if (!a->on) continue;
    if (a->gest) { a->gT += dt; if (a->gT > a->gD) a->gest = 0; }
    if (a->moving) { float dx = a->tx - a->x, dz = a->tz - a->z, d = sqrtf(dx*dx + dz*dz), s = 2.2f * dt;
      if (d <= s) { a->x = a->tx; a->z = a->tz; a->moving = 0; } else { a->x += dx / d * s; a->z += dz / d * s; a->yaw = atan2f(dx, dz); } }
  }
}
