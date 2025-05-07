#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sistema.h"

// Função para escalonar próximo processo
void escalonar_proximo_processo(Sistema *sistema);

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

// Implementação da função escalonar_proximo_processo
void escalonar_proximo_processo(Sistema *sistema) {
    for (int i = 0; i < MAX_PRIORIDADE; i++) {
        if (!esta_vazia(&sistema->estado_pronto[i])) {
            int pid = dequeue(&sistema->estado_pronto[i]);
            ProcessoSimulado *p = &sistema->tabela[pid];

            if (p->estado == TERMINADO) continue;

            sistema->cpu.processo_id = p->id;
            sistema->cpu.programa = p->programa;
            sistema->cpu.pc = p->pc;
            sistema->cpu.memoria = p->memoria;
            sistema->cpu.tamanho_memoria = 100;
            sistema->cpu.quantum_usado = 0;
            sistema->cpu.prioridade = p->prioridade;

            p->estado = EXECUTANDO;
            printf("\n[G] Processo P%d escalonado para execução\n", pid);
            return;
        }
    }

    sistema->cpu.processo_id = -1;  // Nenhum processo disponível
}

void executar_proxima_instrucao(Sistema *sistema) {
    // Verifica processos bloqueados que devem ser desbloqueados
    FilaDeProcessos temp_fila;
    inicializa_fila(&temp_fila);
    
    while (!esta_vazia(&sistema->estado_bloqueado)) {
        int pid = dequeue(&sistema->estado_bloqueado);
        ProcessoSimulado *p = &sistema->tabela[pid];
        
        if (p->tempo_bloqueio <= sistema->tempo) {
            p->estado = PRONTO;
            enqueue(&sistema->estado_pronto[p->prioridade], pid);
            printf("\n[G] Processo P%d desbloqueado (tempo %d)\n", pid, sistema->tempo);
        } else {
            enqueue(&temp_fila, pid);
        }
    }
    
    // Restaura a fila de bloqueados
    while (!esta_vazia(&temp_fila)) {
        enqueue(&sistema->estado_bloqueado, dequeue(&temp_fila));
    }

    // Escalona novo processo se necessário
    if (sistema->cpu.processo_id == -1) {
        escalonar_proximo_processo(sistema);
    }

    // Verifica se há processo para executar
    int pid = sistema->cpu.processo_id;
    if (pid == -1) {
        printf("\n[G] Nenhum processo disponível para execução.\n");
        return;
    }

    ProcessoSimulado *proc = &sistema->tabela[pid];
    if (proc->estado == TERMINADO) {
        sistema->cpu.processo_id = -1;
        printf("\n[G] Tentativa de executar processo P%d já terminado\n", pid);
        return;
    }

    // Executa a próxima instrução
    char *instrucao = proc->programa[sistema->cpu.pc];
    printf("\n[G] Executando instrução: %s (P%d)\n", instrucao, pid);

    char tipo;
    int x, n;
    if (sscanf(instrucao, "%c %d %d", &tipo, &x, &n) >= 1) {
        switch (tipo) {
            case 'D':
                if (x >= 0 && x < 100) sistema->cpu.memoria[x] = 0;
                break;
            case 'V':
                if (x >= 0 && x < 100) sistema->cpu.memoria[x] = n;
                break;
            case 'A':
                if (x >= 0 && x < 100) sistema->cpu.memoria[x] += n;
                break;
            case 'S':
                if (x >= 0 && x < 100) sistema->cpu.memoria[x] -= n;
                break;
            case 'B': {
                int tempo_bloqueio;
                if (sscanf(instrucao, "B %d", &tempo_bloqueio) == 1) {
                    printf("\n[G] Processo P%d bloqueado por %d unidades de tempo\n", pid, tempo_bloqueio);
                    proc->estado = BLOQUEADO;
                    proc->tempo_bloqueio = sistema->tempo + tempo_bloqueio;
                    enqueue(&sistema->estado_bloqueado, pid);
                    proc->pc++; 
                    sistema->cpu.processo_id = -1;
                    escalonar_proximo_processo(sistema);
                    return;
                }
                break;
            }
            case 'T': {
                printf("\n[G] Processo P%d terminado.\n", pid);
                
                // Libera recursos
                for (int i = 0; i < proc->tamanho_memoria; i++) {
                    free(proc->programa[i]);
                }
                free(proc->programa);
                free(proc->memoria);
                
                // Atualiza estado
                proc->estado = TERMINADO;
                sistema->cpu.processo_id = -1;
                
                // Retorna para o pai se existir
                if (proc->id_pai != -1) {
                    ProcessoSimulado *pai = &sistema->tabela[proc->id_pai];
                    if (pai->estado != TERMINADO && pai->estado != EXECUTANDO) {
                        pai->estado = PRONTO;
                        enqueue(&sistema->estado_pronto[pai->prioridade], proc->id_pai);
                        printf("\n[G] Processo pai P%d movido para fila de prontos\n", proc->id_pai);
                    }
                }
                
                // Escalona próximo processo
                escalonar_proximo_processo(sistema);
                return;
            }
            case 'N': {
                char nome_programa[128];
                if (sscanf(instrucao, "N %s", nome_programa) == 1) {
                    char caminho[256];
                    snprintf(caminho, sizeof(caminho), "programas/%s.txt", nome_programa);
                    
                    if (sistema->total_processos >= MAX_PROCESSOS) {
                        printf("\n[G] Limite de processos atingido.\n");
                        break;
                    }
                    
                    int novo_pid = sistema->total_processos;
                    ProcessoSimulado *novo = &sistema->tabela[novo_pid];
                    
                    novo->id = novo_pid;
                    novo->id_pai = pid;
                    novo->estado = PRONTO;
                    novo->pc = 0;
                    novo->prioridade = proc->prioridade;
                    novo->tempo_inicio = sistema->tempo;
                    novo->tempo_cpu = 0;
                    novo->tempo_bloqueio = 0;
                    
                    if (!carregar_programa(caminho, &novo->programa, &novo->tamanho_memoria)) {
                        printf("\n[G] Falha ao carregar o programa %s\n", caminho);
                        break;
                    }
                    
                    novo->memoria = calloc(100, sizeof(int));
                    enqueue(&sistema->estado_pronto[novo->prioridade], novo_pid);
                    sistema->total_processos++;
                    
                    printf("\n[G] Processo P%d criado a partir de %s\n", novo_pid, nome_programa);
                }
                break;
            }
            case 'F': {
                int n;
                if (sscanf(instrucao, "F %d", &n) == 1) {
                    if (sistema->total_processos >= MAX_PROCESSOS) {
                        printf("\n[G] Limite de processos atingido.\n");
                        break;
                    }
                    
                    int novo_pid = sistema->total_processos;
                    ProcessoSimulado *novo = &sistema->tabela[novo_pid];
                    
                    // Copia do processo atual
                    *novo = *proc;
                    novo->id = novo_pid;
                    novo->id_pai = pid;
                    novo->estado = PRONTO;
                    novo->pc = sistema->cpu.pc + 1;
                    novo->tempo_inicio = sistema->tempo;
                    novo->tempo_cpu = 0;
                    novo->tempo_bloqueio = 0;
                    
                    // Copia programa e memória
                    novo->programa = malloc(MAX_INSTRUCOES * sizeof(char*));
                    for (int i = 0; i < proc->tamanho_memoria; i++) {
                        novo->programa[i] = strdup(proc->programa[i]);
                    }
                    
                    novo->memoria = malloc(100 * sizeof(int));
                    memcpy(novo->memoria, proc->memoria, 100 * sizeof(int));
                    
                    // Adiciona à fila de pronto
                    enqueue(&sistema->estado_pronto[novo->prioridade], novo_pid);
                    sistema->total_processos++;
                    
                    // Ajusta PC do processo pai
                    sistema->cpu.pc += n;
                }
                break;
            }
            case 'R': {
                char nome_arquivo[128];
                if (sscanf(instrucao, "R %s", nome_arquivo) == 1) {
                    char caminho[256];
                    snprintf(caminho, sizeof(caminho), "programas/%s.txt", nome_arquivo);
                    
                    printf("\n[G] Substituindo imagem do processo P%d com programa de %s\n", pid, caminho);
                    
                    // Libera programa antigo na CPU
                    for (int i = 0; i < proc->tamanho_memoria; i++) {
                        if (sistema->cpu.programa[i] != NULL) {
                            free(sistema->cpu.programa[i]);
                        }
                    }
                    free(sistema->cpu.programa);
                    
                    // Carrega novo programa na CPU
                    char **novo_programa;
                    int novo_tamanho;
                    
                    if (!carregar_programa(caminho, &novo_programa, &novo_tamanho)) {
                        printf("\n[G] Falha ao carregar %s, processo continua com programa atual\n", caminho);
                        break;
                    }
                    
                    // Atualiza estrutura CPU com novo programa
                    sistema->cpu.programa = novo_programa;
                    sistema->cpu.pc = 0;
                    
                    // Libera e recria memória na CPU
                    if (sistema->cpu.memoria != NULL) {
                        free(sistema->cpu.memoria);
                    }
                    sistema->cpu.memoria = calloc(100, sizeof(int));
                    sistema->cpu.tamanho_memoria = 100;
                    
                    // Reflete essas mudanças no processo atual
                    proc->programa = novo_programa;
                    proc->pc = 0;
                    proc->memoria = sistema->cpu.memoria;
                    proc->tamanho_memoria = novo_tamanho;
                    
                    printf("\n[G] Imagem do processo P%d substituída com sucesso\n", pid);
                }
                break;
            }
            default:
                printf("\n[G] Instrução inválida: %s\n", instrucao);
                break;
        }

        // Avança PC e atualiza contadores
        sistema->cpu.pc++;
        proc->pc = sistema->cpu.pc;
        proc->tempo_cpu++;
        sistema->cpu.quantum_usado++;
        
        // Verifica quantum
        int quantum_maximo = 1 << proc->prioridade;
        if (sistema->cpu.quantum_usado >= quantum_maximo) {
            //printf("\n[G] Quantum expirado para P%d\n", pid);
            proc->estado = PRONTO;
            if (proc->prioridade < MAX_PRIORIDADE - 1) {
                proc->prioridade++;
            }
            enqueue(&sistema->estado_pronto[proc->prioridade], pid);
            sistema->cpu.processo_id = -1;
            escalonar_proximo_processo(sistema);
        }
    }
}