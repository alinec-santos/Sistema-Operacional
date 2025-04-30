#include <stdio.h>
#include <stdlib.h>
#include "sistema.h"

void inicializa_fila(FilaDeProcessos *fila) {
    fila->inicio = 0;
    fila->fim = 0;
}

int esta_vazia(FilaDeProcessos *fila) {
    return fila->inicio == fila->fim;
}

void enqueue(FilaDeProcessos *fila, int id) {
    fila->fila[fila->fim++] = id;
}

int dequeue(FilaDeProcessos *fila) {
    if (esta_vazia(fila)) return -1;
    return fila->fila[fila->inicio++];
}

void inicializa_sistema(Sistema *sistema) {
    sistema->tempo = 0;
    sistema->total_processos = 0;
    sistema->cpu.programa = NULL;
    sistema->cpu.pc = 0;
    sistema->cpu.memoria = NULL;
    sistema->cpu.tamanho_memoria = 0;
    sistema->cpu.quantum_usado = 0;
    sistema->cpu.processo_id = -1;
    sistema->cpu.prioridade = 0;

    for (int i = 0; i < MAX_PRIORIDADE; i++)
        inicializa_fila(&sistema->estado_pronto[i]);

    inicializa_fila(&sistema->estado_bloqueado);
}

void imprimir_estado(const Sistema *sistema) {
    printf("Tempo: %d\n", sistema->tempo);
    printf("Processos ativos: %d\n", sistema->total_processos);

    for (int i = 0; i < sistema->total_processos; i++) {
        const ProcessoSimulado *p = &sistema->tabela[i];
        printf("P%d (pai: %d) | estado: %d | prioridade: %d | PC: %d | tempo CPU: %d\n",
            p->id, p->id_pai, p->estado, p->prioridade, p->pc, p->tempo_cpu);
    }
}
