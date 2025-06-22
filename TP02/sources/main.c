#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "disk.h"
#include "superblock.h"
#include "inode.h"
#include "bitmap.h"
#include "dir.h"
//falta o modo script e modo verboso
void print_header(const char *title) {
    printf("\n====================================\n");
    printf("  %s\n", title);
    printf("====================================\n");
}

void print_menu() {
    print_header("SISTEMA DE ARQUIVOS SIMULADO");

    printf("1 - Criar diretório '/home'\n"); //ok
    printf("2 - Criar novo arquivo \n"); //ok
    printf("3 - Listar conteúdo de um diretório\n"); //ok
    printf("4 - Criar novo diretório(IMPLEMENTAR) \n");
    printf("5 - Renomear diretório (A IMPLEMENTAR)\n");
    printf("6 - Apagar diretório (A IMPLEMENTAR)\n");
    printf("7 - Renomear arquivo (A IMPLEMENTAR)\n");
    printf("8 - Mover arquivo (A IMPLEMENTAR)\n");
    printf("9 - Apagar arquivo (A IMPLEMENTAR)\n");
    printf("10 - Listar conteúdo de um arquivo\n"); //ok
    printf("11 - Verificar informações do root\n"); //ok
    printf("0 - Sair\n");
    printf("------------------------------------\n");
    printf("Escolha a opção: ");
}

int main() {
    size_t disk_size = 10 * 1024 * 1024; // 10 MB fixo por enquanto
    size_t block_size;

    printf("Digite o tamanho do bloco em bytes (ex: 512, 1024, 2048, 4096): ");
    if (scanf("%zu", &block_size) != 1) {
        fprintf(stderr, "Entrada inválida.\n");
        return 1;
    }

    // Validar tamanho do bloco
    if (block_size < 512 || block_size > 8192 || (block_size & (block_size - 1)) != 0) {
        fprintf(stderr, "Tamanho de bloco inválido. Deve ser potência de 2 entre 512 e 8192.\n");
        return 1;
    }

    Disk *disk = disk_create("fs.bin", disk_size, block_size);
    if (!disk) {
        fprintf(stderr, "Erro ao criar disco!\n");
        return 1;
    }
    
    Superblock sb;
    superblock_init(disk, &sb);
    inode_reset_counter();

    // Reserva blocos do superbloco e bitmap
    bitmap_set(disk, 0, 1);
    bitmap_set(disk, 1, 1);

    /* ====================== */
    /* 2. CRIAÇÃO DO ROOT */
    /* ====================== */
    uint32_t root_inode_num = inode_alloc();
    if (root_inode_num != 0) {
        printf("[ERRO] Root precisa ser o inode 0!\n");
        return 1;
    }

    Inode *root_inode = inode_create(040755);  // Modo diretório
    if (!root_inode) {
        printf("[ERRO] Falha ao criar inode do root!\n");
        return 1;
    }

    // Aloca um bloco para o root
    uint32_t root_block = bitmap_find_free_block(disk);
    if (root_block == (uint32_t)-1) {
        printf("[ERRO] Sem blocos livres para o root!\n");
        free(root_inode);
        return 1;
    }
    bitmap_set(disk, root_block, 1);
    root_inode->blocks[0] = root_block;
    root_inode->size = 2 * DIR_ENTRY_SIZE;

    // Cria entradas "." e ".."
    DirEntry root_entries[2] = {
        {0, "."},
        {0, ".."}
    };

    // Escreve as entradas no bloco do root
    lseek(disk->fd, root_block * disk->block_size, SEEK_SET);
    write(disk->fd, root_entries, sizeof(root_entries));

    // Salva o inode root
    inode_save(disk, root_inode_num, root_inode);
    print_header("DIRETÓRIO ROOT CRIADO COM SUCESSO");
    printf("• Tamanho: %u bytes\n", root_inode->size);
    printf("• Criado em: %s", ctime(&root_inode->created_at));

    // DEBUG: Verificar root imediatamente após criação
    print_header("CONTEÚDO DO DIRETÓRIO ROOT (inode 0)");
    dir_list(disk, 0);

    /* ====================== */
    /* 3. VERIFICAÇÃO DO BITMAP */
    /* ====================== */
    print_header("VERIFICAÇÃO DO BITMAP");
    printf("Bloco 0 (superbloco) está %s\n", bitmap_get(disk, 0) ? "alocado" : "livre");
    printf("Bloco 1 (bitmap) está %s\n", bitmap_get(disk, 1) ? "alocado" : "livre");

    uint32_t free_block = bitmap_find_free_block(disk);
    printf("Primeiro bloco livre: %u\n", free_block);



    int opcao;
    while (1) {
        print_menu();

        if (scanf("%d", &opcao) != 1) {
            printf("\n[ERRO] Entrada inválida!\n");
            while(getchar() != '\n');  // limpar buffer
            continue;
        }
        getchar(); // consumir '\n' residual

        switch (opcao) {
            case 1:
                print_header("CRIAR DIRETÓRIO '/home'");
                if (dir_create(disk, 0, "home") == 0) {
                    printf("• Diretório '/home' criado com sucesso!\n");
                    print_header("CONTEÚDO DO DIRETÓRIO RAIZ APÓS CRIAÇÃO");
                    dir_list(disk, 0);
                } else {
                    printf("• Falha ao criar '/home'\n");
                }
                break;

            case 2: {
                print_header("CRIAR NOVO ARQUIVO");
                printf("[INFO] Diretórios disponíveis no root:\n");
                dir_list(disk, 0);

                printf("Digite o inode do diretório onde deseja criar o arquivo: ");
                uint32_t dir_inode;
                if(scanf("%u", &dir_inode) != 1){
                    printf("[ERRO] Entrada inválida para inode!\n");
                    while(getchar() != '\n');
                    break;
                }
                getchar();

                char nome_arquivo[256];
                printf("Digite o nome do arquivo a ser criado: ");
                fgets(nome_arquivo, sizeof(nome_arquivo), stdin);
                nome_arquivo[strcspn(nome_arquivo, "\n")] = 0; // remove '\n'

                char caminho_arquivo_real[512];
                printf("Digite o caminho do arquivo real para importar conteúdo: ");
                fgets(caminho_arquivo_real, sizeof(caminho_arquivo_real), stdin);
                caminho_arquivo_real[strcspn(caminho_arquivo_real, "\n")] = 0;

                if (file_create(disk, dir_inode, caminho_arquivo_real, nome_arquivo) == 0) {
                    printf("Arquivo '%s' criado com sucesso no diretório de inode %u!\n", nome_arquivo, dir_inode);
                } else {
                    printf("Falha ao criar arquivo '%s' no diretório de inode %u!\n", nome_arquivo, dir_inode);
                }
                break;
            }

            case 3: {
                print_header("DIRETÓRIOS DISPONÍVEIS NO ROOT");
                if (dir_list_dirs(disk, 0) != 0) {
                    printf("[ERRO] Falha ao listar diretórios no root.\n");
                    break;
                }

                printf("Digite o inode do diretório que deseja listar: ");
                uint32_t chosen_inode;
                if (scanf("%u", &chosen_inode) != 1) {
                    printf("[ERRO] Entrada inválida!\n");
                    while(getchar() != '\n');
                    break;
                }
                getchar();

                print_header("CONTEÚDO DO DIRETÓRIO ESCOLHIDO");
                if (dir_list(disk, chosen_inode) != 0) {
                    printf("[ERRO] Falha ao listar conteúdo do diretório %u\n", chosen_inode);
                }
                break;
            }
            case 4:
                print_header("CRIAR NOVO DIRETÓRIO (CUSTOMIZADO)");
                //  aqui vai ter que ser exibido os diretorios e o usuario vai escolher se ele quer criar um diretorio dentro de outro em um i-node separado dentro do root. 
                printf("[INFO] Opção ainda não implementada.\n");
                break;

            case 10: {
                print_header("LISTAR CONTEÚDO DE UM ARQUIVO");
                printf("[INFO] Diretórios disponíveis no root:\n");
                dir_list(disk, 0);

                printf("Digite o inode do arquivo que deseja ler: ");
                uint32_t file_inode;
                if(scanf("%u", &file_inode) != 1){
                    printf("[ERRO] Entrada inválida para inode!\n");
                    while(getchar() != '\n');
                    break;
                }
                getchar();

                printf("\n[EXECUTANDO] Lendo conteúdo do arquivo (inode %u):\n", file_inode);
                if (file_read(disk, file_inode) != 0) {
                    printf("[ERRO] Falha ao ler o arquivo de inode %u.\n", file_inode);
                }
                break;
            }

            case 11:
                print_header("INFORMAÇÕES DETALHADAS DO DIRETÓRIO ROOT");
                if (dir_list_detailed(disk, 0) != 0) {
                    printf("[ERRO] Falha ao listar informações do root.\n");
                }
                break;

            case 0:
                printf("\n[INFO] Sistema finalizado com sucesso.\n");
                free(root_inode);
                disk_free(disk);
                return 0;

            default:
                printf("\n[ERRO] Opção inválida!\n");
        }
        printf("\n");  // Espaço extra para facilitar leitura
    }

    return 0;
}
