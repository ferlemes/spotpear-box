// ===========================================================================
//  i18n.cpp — Tabelas de texto (EN/PT/ES) e listas de palavras.
//
//  IMPORTANTE: a fonte 6x8 do Arduino_GFX so tem ASCII, entao os textos e as
//  palavras sao SEM acento (nada de a-til, c-cedilha, n-til, acentos). O
//  codigo morse padrao (ITU) tambem so cobre A-Z, entao as palavras usam
//  apenas letras A-Z maiusculas.
// ===========================================================================
#include "i18n.h"
#include "settings.h"
#include "morse.h"

namespace i18n {

Lang lang = EN;
void setLang(Lang l) { lang = l; }

// [Key][Lang]  (Lang: 0=EN, 1=PT, 2=ES)
static const char* STR[KEY_COUNT][3] = {
    /* TITLE         */ { "MORSE",           "MORSE",            "MORSE"            },
    /* MENU_LEARN    */ { "Learn",           "Aprender",         "Aprender"         },
    /* MENU_LETTERS  */ { "Letters",         "Letras",           "Letras"           },
    /* MENU_WORDS    */ { "Words",           "Palavras",         "Palabras"         },
    /* MENU_LISTEN   */ { "Listen",          "Ouvir",            "Escuchar"         },
    /* MENU_SETTINGS */ { "Settings",        "Ajustes",          "Ajustes"          },
    /* SET_TITLE     */ { "Language",        "Idioma",           "Idioma"           },
    /* SET_HINT      */ { "Tap a language",  "Toque um idioma",  "Toca un idioma"   },
    /* BACK_HINT     */ { "[-] key = back",  "botao [-] = volta","boton [-] = volver"},
    /* HOWTO_1       */ { "Press [+] = dot",  "Aperta [+]=ponto", "Pulsa [+]=punto"  },
    /* HOWTO_2       */ { "Hold [+] = dash",  "Segura [+]=traco", "Manten [+]=raya"  },
    /* HOWTO_3       */ { "Slow? hint shows","Demorou? sai dica","Lento? sale pista" },
    /* TAP_CONTINUE  */ { "tap to continue", "toque p/ seguir",  "toca p/ seguir"   },
    /* KEY_LABEL     */ { "KEY",             "KEY",              "KEY"              },
    /* HINT_LABEL    */ { "hint",            "dica",             "pista"            },
    /* CORRECT       */ { "Correct!",        "Acertou!",         "Correcto!"        },
    /* TRY_AGAIN     */ { "Try again",       "Tente de novo",    "Intenta otra"     },
    /* LEVEL         */ { "Level",           "Nivel",            "Nivel"            },
    /* SCORE         */ { "Score",           "Placar",           "Puntos"           },
    /* LEARN_TRY     */ { "Type it on KEY",  "Digite na KEY",    "Escribe en KEY"   },
    /* NEXT          */ { "Next",            "Proxima",          "Siguiente"        },
    /* HEAR          */ { "Hear",            "Ouvir",            "Oir"              },
    /* LISTEN_PROMPT */ { "Which word?",     "Qual palavra?",    "Que palabra?"     },
    /* REPLAY        */ { "Replay",          "Repetir",          "Repetir"          },
    /* WORD_DONE     */ { "Word done!",      "Palavra ok!",      "Palabra ok!"      },
    /* LETTERS_LABEL */ { "Letters",         "Letras",           "Letras"           },
    /* SPEED_TITLE   */ { "Speed",           "Velocidade",       "Velocidad"        },
    /* SLEEP_BYE     */ { "See you!",        "Ate mais!",        "Hasta pronto!"    },
    /* SLEEP_HINT    */ { "PWR to turn on",  "PWR p/ ligar",     "PWR p/ encender"  },
    /* WORDLEN       */ { "Length",          "Tamanho",          "Longitud"         },
    /* CHECKMODE     */ { "Check",           "Conferir",         "Revisar"          },
    /* CHK_LETTER    */ { "Letter",          "Letra",            "Letra"            },
    /* CHK_WORD      */ { "Word",            "Palavra",          "Palabra"          },
    /* START         */ { "Start",           "Comecar",          "Empezar"          },
    /* SPELL_HINT    */ { "pause btwn letters","pause entre letras","pausa entre letras"},
};

const char* t(Key k) {
    if (k < 0 || k >= KEY_COUNT) return "";
    return STR[k][lang];
}

const char* langName(Lang l) {
    switch (l) {
        case EN: return "English";
        case PT: return "Portugues";
        case ES: return "Espanol";
    }
    return "?";
}

// --- Palavras por idioma (maiusculas, A-Z, <= 4 letras, sem acento) -------
//  Ordenadas por dificuldade crescente: as primeiras usam so as letras
//  iniciais de morse::ORDER (nivel Beg), depois entram letras de Int e Adv.
//  O filtro levelWords() seleciona, em tempo real, as validas p/ o nivel.
static const char* WORDS_EN[] = {
    // Beginner (letras A E I O U T N M S R)
    "SUN","TEA","NET","SUM","RAT","TIN","SIT","SEA","ART","RUN",
    // Intermediate (+ D K G W H B L F)
    "DOG","BED","BUS","HAT","RED","LOG",
    // Advanced (demais letras)
    "CAT","FOX","CUP","YES",
};
static const char* WORDS_PT[] = {
    "SOM","MAR","RIO","RUA","TIA","SIM","TOM","ANO","RATO","ARTE",
    "DADO","BOLA","GATO","LUA","BOI","FLOR",
    "CASA","PATO","VACA","UVA",
};
static const char* WORDS_ES[] = {
    "MAR","RIO","SUR","OSO","ANO","TIA","ORO","ROSA","MESA","MANO",
    "SOL","DEDO","GATO","BOLA","FLOR","LADO",
    "CASA","PATO","PAN","UVA",
};

static const int WCOUNT = 20;

int wordCount() { return WCOUNT; }

const char* wordAt(int i) {
    if (i < 0 || i >= WCOUNT) i = 0;
    switch (lang) {
        case PT: return WORDS_PT[i];
        case ES: return WORDS_ES[i];
        default: return WORDS_EN[i];
    }
}

int levelWords(int* out, int maxOut) {
    int cap = settings::letterCap();
    int n = 0;
    for (int i = 0; i < WCOUNT && n < maxOut; i++) {
        const char* w = wordAt(i);
        bool ok = true;
        for (int j = 0; w[j]; j++)
            if (morse::rank(w[j]) >= cap) { ok = false; break; }
        if (ok) out[n++] = i;
    }
    return n;
}

// --- Palavras longas (5-6 letras), sem acento, A-Z ------------------------
static const char* LONG_EN[] = {
    "WATER","LIGHT","MUSIC","PLANT","RIVER","STONE",
    "BREAD","HOUSE","TIGER","MONEY","CLOUD","HORSE",
};
static const char* LONG_PT[] = {
    "GARFO","LIVRO","PRATO","VERDE","PONTE","NOITE",
    "TERRA","FRUTA","PEDRA","GRAMA","AMIGO","CAVALO",
};
static const char* LONG_ES[] = {
    "VERDE","NOCHE","MUNDO","LIBRO","PLATO","PERRO",
    "MONEDA","FUEGO","TIGRE","NIEVE","GRANDE","FLORES",
};

static const int WLCOUNT = 12;

int wordLongCount() { return WLCOUNT; }

const char* wordLongAt(int i) {
    if (i < 0 || i >= WLCOUNT) i = 0;
    switch (lang) {
        case PT: return LONG_PT[i];
        case ES: return LONG_ES[i];
        default: return LONG_EN[i];
    }
}

} // namespace i18n
