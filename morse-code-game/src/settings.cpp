// ===========================================================================
//  settings.cpp — Tabela dos niveis de velocidade.
// ===========================================================================
#include "settings.h"

namespace settings {

int speed = SLOW;   // padrao: o mais folgado (mais confiavel p/ iniciante)

//                                  Slow   Med   Fast
static const uint32_t DOTMAX[3]  = { 400,  280,  180 };   // limiar ponto/traco (ms)
static const uint32_t LGAP[3]    = { 1200, 900,  650 };   // gap p/ fechar a letra (ms)
static const int      PLAYDOT[3] = { 160,  120,  90  };   // unidade na reproducao (ms)
static const char*    NAME[3]    = { "Slow", "Med", "Fast" };

void setSpeed(int s) { if (s < 0) s = 0; if (s > 2) s = 2; speed = s; }

uint32_t dotMaxMs()    { return DOTMAX[speed];  }
uint32_t letterGapMs() { return LGAP[speed];    }
int      playDotMs()   { return PLAYDOT[speed]; }

const char* speedName(int s) { return (s >= 0 && s <= 2) ? NAME[s] : "?"; }

// --- Nivel de dificuldade -------------------------------------------------
int level = BEGINNER;

//                              Beg  Int  Adv
static const int   CAP[3]   = { 10,  18,  26 };   // letras cobertas (prefixo de ORDER)
static const char* LNAME[3] = { "Beg", "Int", "Adv" };

void setLevel(int s) { if (s < 0) s = 0; if (s > 2) s = 2; level = s; }

int letterCap() { return CAP[level]; }

const char* levelName(int s) { return (s >= 0 && s <= 2) ? LNAME[s] : "?"; }

} // namespace settings
