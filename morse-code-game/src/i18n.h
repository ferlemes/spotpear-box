// ===========================================================================
//  i18n.h — Idiomas (EN padrao / PT / ES). Afeta os textos da interface e a
//  lista de palavras do modo "Words". As letras e o codigo morse sao
//  universais; so os rotulos e as palavras mudam.
// ===========================================================================
#pragma once
#include <stdint.h>

namespace i18n {
    enum Lang { EN = 0, PT = 1, ES = 2 };
    enum Key {
        TITLE = 0, MENU_LEARN, MENU_LETTERS, MENU_WORDS, MENU_LISTEN, MENU_SETTINGS,
        SET_TITLE, SET_HINT, BACK_HINT,
        HOWTO_1, HOWTO_2, HOWTO_3, TAP_CONTINUE,
        KEY_LABEL, HINT_LABEL, CORRECT, TRY_AGAIN, LEVEL, SCORE,
        LEARN_TRY, NEXT, HEAR, LISTEN_PROMPT, REPLAY,
        WORD_DONE, LETTERS_LABEL, SPEED_TITLE,
        SLEEP_BYE, SLEEP_HINT,
        WORDLEN, CHECKMODE, CHK_LETTER, CHK_WORD, START, SPELL_HINT,
        KEY_COUNT
    };

    extern Lang lang;
    void setLang(Lang l);

    const char* t(Key k);        // texto no idioma atual
    const char* langName(Lang l);// "English" / "Portugues" / "Espanol"

    // Palavras do idioma atual (maiusculas, so A-Z, <= 4 letras).
    //  OBS: chamado wordAt (nao "word") porque o Arduino define a macro word().
    int         wordCount();
    const char* wordAt(int i);

    // Indices das palavras validas p/ o nivel de dificuldade ATUAL (todas as
    // letras dentro do teto do nivel). Preenche out[] e retorna a quantidade.
    int         levelWords(int* out, int maxOut);

    // Palavras LONGAS (5-6 letras) do idioma atual — o "nivel 2" do jogo de
    // palavras. Usam o alfabeto todo (nao filtram por teto de letras).
    int         wordLongCount();
    const char* wordLongAt(int i);
}
