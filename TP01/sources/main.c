#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // fork, pipe, read, write
#include <sys/types.h>
#include <sys/wait.h>
#include "gerenciador.h" // Importando o arquivo do gerenciador

#define TAM_BUFFER 128

void modo_interativo(int write_fd);
void modo_arquivo(int write_fd, const char *arquivo);

int main(int argc, char *argv[]) {
    int pipe_fd[2];
    
    if (pipe(pipe_fd) == -1) {
        perror("Erro ao criar pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Erro no fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Processo gerenciador de processos
        close(pipe_fd[1]); // fecha escrita
        dup2(pipe_fd[0], STDIN_FILENO); // redireciona leitura do pipe para STDIN
        close(pipe_fd[0]);

        // Chama a função do gerenciador diretamente
        gerenciador(); // Agora chamamos a função gerenciador diretamente
        exit(EXIT_FAILURE);
    } else {
        // Processo controle
        close(pipe_fd[0]); // fecha leitura

        int escolha;
        usleep(10000);
        printf("\nEscolha modo de entrada:\n1 - Interativo\n2 - Arquivo\n> ");
        scanf("%d", &escolha);
        getchar(); // consome \n

        if (escolha == 1) {
            modo_interativo(pipe_fd[1]);
        } else if (escolha == 2) {
            char nome_arquivo[100];
            printf("Digite o nome do arquivo de entrada: ");
            fgets(nome_arquivo, sizeof(nome_arquivo), stdin);
            nome_arquivo[strcspn(nome_arquivo, "\n")] = 0; // remove \n
            modo_arquivo(pipe_fd[1], nome_arquivo);
        } else {
            printf("Opção inválida.\n");
        }

        close(pipe_fd[1]);
        wait(NULL); // espera o gerenciador terminar
    }

    return 0;
}

void modo_interativo(int write_fd) {
    char buffer[TAM_BUFFER];

    while (1) {
        // Espera um pouco para garantir que as mensagens do gerenciador foram exibidas
        usleep(10000); // 10ms de delay
        
        printf("\nDigite um comando (U, I, M): ");
        fflush(stdout); // Garante que o prompt é exibido
        
        fgets(buffer, TAM_BUFFER, stdin);

        write(write_fd, buffer, strlen(buffer));

        if (buffer[0] == 'M') break;
    }
}

void modo_arquivo(int write_fd, const char *arquivo) {
    FILE *fp = fopen(arquivo, "r");
    if (!fp) {
        perror("Erro ao abrir arquivo");
        return;
    }

    char linha[TAM_BUFFER];
    while (fgets(linha, TAM_BUFFER, fp)) {
        write(write_fd, linha, strlen(linha));
        if (linha[0] == 'M') break;
    }

    fclose(fp);
}
