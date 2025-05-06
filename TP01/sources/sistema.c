#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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

const char* estado_str(Estado estado) {
    switch (estado) {
        case PRONTO: return "PRONTO";
        case EXECUTANDO: return "EXECUTANDO";
        case BLOQUEADO: return "BLOQUEADO";
        case TERMINADO: return "TERMINADO";
        default: return "DESCONHECIDO";
    }
}

void imprimir_estado(const Sistema *sistema) {
    printf("\n[INFO] Estado atual do sistema\n");
    printf("--------------------------------------------------\n");
    printf("Tempo atual: %d\n", sistema->tempo);
    printf("Total de processos ativos: %d\n\n", sistema->total_processos);
    printf("ID  | Pai | Estado     | Prioridade | PC  | CPU\n");
    printf("----|-----|------------|------------|-----|-----\n");

    for (int i = 0; i < sistema->total_processos; i++) {
        const ProcessoSimulado *p = &sistema->tabela[i];

        printf("P%-3d| %-4d| %-11s| %-10d| %-3d | %-3d\n",
               p->id,
               p->id_pai,
               estado_str(p->estado),
               p->prioridade,
               p->pc,
               p->tempo_cpu);
    }

    printf("--------------------------------------------------\n");
}



int carregar_programa(const char *nome_arquivo, char ***programa, int *tamanho) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) {
        perror("Erro ao abrir o arquivo do programa");
        return 0;
    }

    *programa = malloc(MAX_INSTRUCOES * sizeof(char *));
    char linha[128];
    int count = 0;

    while (fgets(linha, sizeof(linha), arquivo) && count < MAX_INSTRUCOES) {
        linha[strcspn(linha, "\n")] = 0; // Remove \n
        (*programa)[count] = strdup(linha); // Copia a linha
        count++;
    }

    *tamanho = count;
    fclose(arquivo);
    return 1;
}

void criar_processo_inicial(Sistema *sistema, const char *nome_arquivo) {
    ProcessoSimulado *p = &sistema->tabela[0];
    p->id = 0;
    p->id_pai = -1;
    p->estado = PRONTO;
    p->pc = 0;
    p->prioridade = 0;
    p->tempo_inicio = sistema->tempo;
    p->tempo_cpu = 0;

    if (!carregar_programa(nome_arquivo, &p->programa, &p->tamanho_memoria)) {
        fprintf(stderr, "Erro ao carregar o programa inicial.\n");
        exit(EXIT_FAILURE);
    }

    // Inicializa memória (ex: 100 posições com zero)
    p->memoria = calloc(100, sizeof(int));

    // Adiciona à fila de prioridade 0 (mais alta)
    enqueue(&sistema->estado_pronto[0], 0);

    sistema->total_processos = 1;

    printf("\n[G] Processo init (P0) criado com sucesso.\n");
    fflush(stdout);
}


void executar_proxima_instrucao(Sistema *sistema) {
    // Se não há processo na CPU, escalona o próximo da fila de prioridade
    if (sistema->cpu.processo_id == -1) {
        for (int i = 0; i < MAX_PRIORIDADE; i++) {
            if (!esta_vazia(&sistema->estado_pronto[i])) {
                int pid = dequeue(&sistema->estado_pronto[i]);
                ProcessoSimulado *p = &sistema->tabela[pid];

                sistema->cpu.processo_id = p->id;
                sistema->cpu.programa = p->programa;
                sistema->cpu.pc = p->pc;
                sistema->cpu.memoria = p->memoria;
                sistema->cpu.tamanho_memoria = 100;
                sistema->cpu.quantum_usado = 0;
                sistema->cpu.prioridade = p->prioridade;

                p->estado = EXECUTANDO;
                break;
            }
        }
    }

    int pid = sistema->cpu.processo_id;
    if (pid == -1) {
        printf("\n[G] Nenhum processo disponível para execução.\n");
        return;
    }

    ProcessoSimulado *proc = &sistema->tabela[pid];
    char *instrucao = proc->programa[sistema->cpu.pc];

    printf("\n[G] Executando instrução: %s (P%d)\n", instrucao, pid);

    char tipo;
    int x, n;
    if (sscanf(instrucao, "%c %d %d", &tipo, &x, &n) >= 1) {
        switch (tipo) {
            case 'D':
                if (x >= 0 && x < 100)
                    sistema->cpu.memoria[x] = 0;
                break;
            case 'V':
                if (x >= 0 && x < 100)
                    sistema->cpu.memoria[x] = n;
                break;
            case 'A':
                if (x >= 0 && x < 100)
                    sistema->cpu.memoria[x] += n;
                break;
            case 'S':
                if (x >= 0 && x < 100)
                    sistema->cpu.memoria[x] -= n;
                break;
            case 'T':
                printf("\n[G] Processo P%d terminado.\n", pid);
                proc->estado = TERMINADO;
                free(proc->programa);
                free(proc->memoria);
                sistema->cpu.processo_id = -1;
                return;
            // Futuramente: F, B, R

            case 'N': {
                // Extrai o nome do programa a partir da string original
                char nome_programa[128];
                if (sscanf(instrucao, "N %s", nome_programa) != 1) {
                    printf("\n[G] Instrução N inválida: %s\n", instrucao);
                    break;
                }
            
                // Monta o caminho do arquivo
                char caminho[256];
                snprintf(caminho, sizeof(caminho), "programas/%s.txt", nome_programa);
            
                // Verifica se ainda há espaço na tabela de processos
                if (sistema->total_processos >= MAX_PROCESSOS) {
                    printf("\n[G] Limite de processos atingido.\n");
                    break;
                }
            
                int novo_pid = sistema->total_processos;
                ProcessoSimulado *novo_proc = &sistema->tabela[novo_pid];
            
                novo_proc->id = novo_pid;
                novo_proc->id_pai = pid;
                novo_proc->estado = PRONTO;
                novo_proc->pc = 0;
                novo_proc->prioridade = proc->prioridade; // ou prioridade fixa, ex: 1
                novo_proc->tempo_inicio = sistema->tempo;
                novo_proc->tempo_cpu = 0;
            
                if (!carregar_programa(caminho, &novo_proc->programa, &novo_proc->tamanho_memoria)) {
                    printf("\n[G] Falha ao carregar o programa %s\n", caminho);
                    break;
                }
            
                novo_proc->memoria = calloc(100, sizeof(int));
            
                // Adiciona à fila de pronto com a prioridade do novo processo
                enqueue(&sistema->estado_pronto[novo_proc->prioridade], novo_pid);
            
                sistema->total_processos++;
            
                printf("\n[G] Processo P%d criado a partir de %s (filho de P%d)\n", novo_pid, nome_programa, pid);
                break;
            }
            
        }

        // Avança PC e tempo de CPU
        sistema->cpu.pc++;
        proc->pc = sistema->cpu.pc;
        proc->tempo_cpu++;
        sistema->tempo++;
    } else {
        printf("\n[G] Instrução inválida: %s\n", instrucao);
    }
}
