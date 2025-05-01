#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gerenciador.h"  // Incluindo o arquivo de cabeçalho
#include "../headers/sistema.h"  // Incluindo a estrutura do sistema

void gerenciador() {
    Sistema sistema;
    inicializa_sistema(&sistema);
    criar_processo_inicial(&sistema, "programas/init.txt");

    char comando[128];

    while (fgets(comando, sizeof(comando), stdin)) {
        // Remove \n
        comando[strcspn(comando, "\n")] = 0;

        if (strcmp(comando, "U") == 0) {
            executar_proxima_instrucao(&sistema);
            printf("\n[G] Tempo avançado para %d\n", sistema.tempo);
        } else if (strcmp(comando, "I") == 0) {
            imprimir_estado(&sistema);
        } else if (strcmp(comando, "M") == 0) {
            printf("\n[G] Finalizando sistema.\n");
            imprimir_estado(&sistema);
            break;
        } else {
            printf("\n[G] Comando inválido: %s\n", comando);
        }
        
        // Limpa o buffer de saída para garantir que tudo foi impresso
        fflush(stdout);
    }
}