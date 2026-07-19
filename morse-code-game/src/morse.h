// ===========================================================================
//  morse.h — Codigo Morse (tabela A-Z) + ordem de aprendizado + audio.
// ===========================================================================
#pragma once
#include <stdint.h>

namespace morse {
    // Padrao de uma letra ('A'..'Z', maiuscula) como string de '.' e '-'.
    // Retorna "" se o caractere nao for uma letra A-Z.
    const char* code(char c);

    // Ordem pedagogica de introducao das letras (vogais primeiro).
    // ORDER[0..len) sao letras maiusculas. len == 26.
    extern const char* ORDER;
    int  orderLen();

    // Posicao de uma letra em ORDER (0..25), ou 99 se nao for A-Z.
    // Usado p/ saber se a letra esta dentro do "teto" de um nivel.
    int  rank(char c);

    // Decodifica um padrao ('.'/'-') na letra correspondente, ou '?' se nao
    // for um codigo A-Z valido. Inverso de code().
    char decode(const char* pat);

    // --- Reproducao sonora (bloqueante) -----------------------------------
    // Uma "unidade" morse = dotMs(). Ponto=1u, traco=3u, gap intra-letra=1u,
    // gap entre letras=3u, gap entre palavras=7u. Tom padrao ~700 Hz.
    int  dotMs();
    void playSymbol(char sym);          // '.' ou '-' (sem gap depois)
    void playPattern(const char* pat);  // toca a letra inteira com gaps intra
    void playLetter(char c);            // playPattern(code(c))
    void playWord(const char* word);    // letras com gap entre-letras
}
