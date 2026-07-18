// ===========================================================================
//  game.h — Regras do jogo da velha + IA (minimax). Sem nada de hardware.
// ===========================================================================
#pragma once
#include <stdint.h>

// Conteúdo de cada célula do tabuleiro (índices 0..8, da esq→dir, cima→baixo).
enum Cell : uint8_t { EMPTY = 0, X = 1, O = 2 };

// Nível da IA.
enum Difficulty : uint8_t { AI_EASY = 0, AI_MEDIUM = 1, AI_HARD = 2 };

// Estado da partida após uma jogada.
enum Result : uint8_t { ONGOING = 0, WIN_X = 1, WIN_O = 2, DRAW = 3 };

struct Game {
    Cell    board[9];
    Cell    turn;       // de quem é a vez (X sempre começa)
    Result  result;     // situação atual
    int8_t  winLine;    // 0..7 = índice da linha vencedora; -1 = nenhuma

    void reset();
    bool place(int idx);              // joga na célula; false se inválida
    static Result winnerOf(const Cell b[9], int8_t* line = nullptr);
};

// Escolhe a melhor jogada para 'me'. Retorna índice 0..8 (ou -1 se cheio).
int aiChooseMove(const Cell board[9], Cell me, Difficulty diff);
