// ===========================================================================
//  game.cpp — Implementação das regras e da IA.
// ===========================================================================
#include "game.h"
#include <esp_random.h>   // esp_random() — RNG de hardware do ESP32

// As 8 linhas vencedoras possíveis (3 horizontais, 3 verticais, 2 diagonais).
static const uint8_t LINES[8][3] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8},   // linhas
    {0, 3, 6}, {1, 4, 7}, {2, 5, 8},   // colunas
    {0, 4, 8}, {2, 4, 6},              // diagonais
};

void Game::reset() {
    for (int i = 0; i < 9; i++) board[i] = EMPTY;
    turn    = X;          // X sempre começa
    result  = ONGOING;
    winLine = -1;
}

// Verifica vitória/empate para um tabuleiro qualquer.
Result Game::winnerOf(const Cell b[9], int8_t* line) {
    for (int i = 0; i < 8; i++) {
        Cell a = b[LINES[i][0]];
        if (a != EMPTY && a == b[LINES[i][1]] && a == b[LINES[i][2]]) {
            if (line) *line = (int8_t)i;
            return (a == X) ? WIN_X : WIN_O;
        }
    }
    if (line) *line = -1;
    for (int i = 0; i < 9; i++)
        if (b[i] == EMPTY) return ONGOING;   // ainda há casa vazia
    return DRAW;
}

bool Game::place(int idx) {
    if (idx < 0 || idx > 8) return false;
    if (board[idx] != EMPTY) return false;
    if (result != ONGOING)   return false;

    board[idx] = turn;
    result = winnerOf(board, &winLine);
    if (result == ONGOING)
        turn = (turn == X) ? O : X;          // passa a vez
    return true;
}

// --- IA -------------------------------------------------------------------

static Cell other(Cell c) { return (c == X) ? O : X; }

// Minimax com poda por profundidade (prefere vitórias rápidas e derrotas
// tardias). Retorna a pontuação do tabuleiro do ponto de vista de 'me'.
static int minimax(Cell b[9], Cell me, Cell toMove, int depth) {
    Result r = Game::winnerOf(b);
    if (r == WIN_X) return (me == X ? 10 - depth : depth - 10);
    if (r == WIN_O) return (me == O ? 10 - depth : depth - 10);
    if (r == DRAW)  return 0;

    bool maximizing = (toMove == me);
    int best = maximizing ? -1000 : 1000;
    for (int i = 0; i < 9; i++) {
        if (b[i] != EMPTY) continue;
        b[i] = toMove;
        int s = minimax(b, me, other(toMove), depth + 1);
        b[i] = EMPTY;
        best = maximizing ? (s > best ? s : best)
                          : (s < best ? s : best);
    }
    return best;
}

int aiChooseMove(const Cell board[9], Cell me, Difficulty diff) {
    // Lista as casas vazias.
    int empties[9], n = 0;
    for (int i = 0; i < 9; i++)
        if (board[i] == EMPTY) empties[n++] = i;
    if (n == 0) return -1;

    // Fácil: joga aleatório.
    if (diff == AI_EASY)
        return empties[esp_random() % n];

    // Médio: 45% das vezes joga aleatório (dá brechas), senão joga o melhor
    // lance. Fica competitivo mas batível.
    if (diff == AI_MEDIUM && (esp_random() % 100) < 45)
        return empties[esp_random() % n];

    // Difícil (e o lance "esperto" do médio): minimax, jogo perfeito.
    Cell b[9];
    for (int i = 0; i < 9; i++) b[i] = board[i];

    int bestScore = -1000, bestMove = empties[0];
    for (int k = 0; k < n; k++) {
        int i = empties[k];
        b[i] = me;
        int s = minimax(b, me, other(me), 1);
        b[i] = EMPTY;
        if (s > bestScore) { bestScore = s; bestMove = i; }
    }
    return bestMove;
}
