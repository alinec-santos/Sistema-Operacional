#ifndef SISTEMA_H
#define SISTEMA_H

#define MAX_PROCESSOS 100
#define MAX_INSTRUCOES 100
#define MAX_PRIORIDADE 4

typedef enum {
    PRONTO,
    EXECUTANDO,
    BLOQUEADO,
    TERMINADO
} Estado;

typedef struct {
    int id;
    int id_pai;
    char **programa; // vetor de strings com instruções
    int pc; // contador de programa
    int *memoria;
    int tamanho_memoria;
    int prioridade;
    Estado estado;
    int tempo_inicio;
    int tempo_cpu;
} ProcessoSimulado;

typedef struct {
    char **programa;
    int pc;
    int *memoria;
    int tamanho_memoria;
    int quantum_usado;
    int prioridade;
    int processo_id; // ID do processo que está na CPU
} Cpu;

typedef struct {
    int fila[MAX_PROCESSOS];
    int inicio;
    int fim;
} FilaDeProcessos;

typedef struct {
    int tempo;
    Cpu cpu;
    ProcessoSimulado tabela[MAX_PROCESSOS];
    int total_processos;
    FilaDeProcessos estado_pronto[MAX_PRIORIDADE]; // uma fila para cada prioridade
    FilaDeProcessos estado_bloqueado;
} Sistema;

// Funções de fila
void inicializa_fila(FilaDeProcessos *fila);
int esta_vazia(FilaDeProcessos *fila);
void enqueue(FilaDeProcessos *fila, int id);
int dequeue(FilaDeProcessos *fila);

// Funções de sistema
void inicializa_sistema(Sistema *sistema);
void imprimir_estado(const Sistema *sistema); // usado pelo processo impressão

#endif
