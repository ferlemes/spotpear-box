// ===========================================================================
//  challenge.h — Nucleo de "digitar uma letra na tecla KEY".
//
//  Compartilhado pelos modos Letters e Words. Cuida de: ler a tecla (toque
//  curto = ponto, segurar = traco) com sidetone ao vivo, mostrar os
//  pontos/tracos digitados, exibir uma dica se o jogador demorar, avaliar
//  contra a letra-alvo e dar feedback de erro (deixando tentar de novo).
// ===========================================================================
#pragma once

namespace challenge {
    struct Ctx {
        char  target;              // letra-alvo ('A'..'Z')
        int   glyphCx, glyphCy;    // onde desenhar os glifos digitados / a dica
        void (*redraw)(void*);     // repinta a tela-base do modo (letra/palavra)
        void* user;                // passado de volta em redraw()
    };

    // Bloqueia ate o jogador acertar a letra ou pedir p/ sair.
    // Retorna 1 = acertou; 0 = saiu (botao de cima).
    int run(const Ctx& c);
}
