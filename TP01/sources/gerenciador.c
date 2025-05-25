#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gerenciador.h"
#include "../headers/sistema.h"

void gerenciador() {
    Sistema sistema;
    inicializa_sistema(&sistema);
    criar_processo_inicial(&sistema, "programas/init.txt");

    char comando[128];

    while (fgets(comando, sizeof(comando), stdin)) {
        // Remove qualquer \n ou \r do final da string
        size_t len = strlen(comando);
        while (len > 0 && (comando[len - 1] == '\n' || comando[len - 1] == '\r')) {
            comando[--len] = 0;
        }

        if (strcmp(comando, "U") == 0) {
            sistema.tempo++;
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

        fflush(stdout);
    }
}
