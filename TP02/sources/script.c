#include "script.h"
#include "disk.h"
#include "superblock.h"
#include "inode.h"
#include "bitmap.h"
#include "dir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_LINE_LENGTH 256
#define MAX_ARGS 4

int dir_create_root(Disk *disk) {
    uint32_t inode_num = inode_alloc();
    if (inode_num != 0) {
        printf("[ERRO] Root precisa ser o inode 0!\n");
        return -1;
    }

    Inode *root_inode = inode_create(040755);  // Diretório padrão
    if (!root_inode) return -1;

    uint32_t block_num = bitmap_find_free_block(disk);
    if (block_num == (uint32_t)-1) {
        free(root_inode);
        return -1;
    }

    bitmap_set(disk, block_num, 1);
    root_inode->blocks[0] = block_num;
    root_inode->size = 2 * DIR_ENTRY_SIZE;

    DirEntry entries[2] = {
        {0, "."},
        {0, ".."}
    };

    lseek(disk->fd, block_num * disk->block_size, SEEK_SET);
    if (write(disk->fd, entries, sizeof(entries)) != sizeof(entries)) {
        bitmap_set(disk, block_num, 0);
        free(root_inode);
        return -1;
    }

    inode_save(disk, inode_num, root_inode);
    free(root_inode);

    return 0;
}

void modo_script(const char *filename) {
    FILE *script = fopen(filename, "r");
    if (!script) {
        printf("[ERRO] Não foi possível abrir o arquivo de script: %s\n", filename);
        return;
    }

    char line[MAX_LINE_LENGTH];
    Disk *disk = NULL;
    uint32_t current_dir_inode = 0; // Começa no root

    // Lê a primeira linha que contém o tamanho do bloco
    if (!fgets(line, sizeof(line), script)) {
        printf("[ERRO] Arquivo de script vazio\n");
        fclose(script);
        return;
    }

    size_t block_size;
    if (sscanf(line, "%zu", &block_size) != 1) {
        printf("[ERRO] Tamanho de bloco inválido na primeira linha\n");
        fclose(script);
        return;
    }

    // Configuração inicial do disco (igual ao modo interativo)
    size_t disk_size = 10 * 1024 * 1024; // 10 MB
    disk = disk_create("fs_script.bin", disk_size, block_size);
    if (!disk) {
        printf("[ERRO] Erro ao criar disco!\n");
        fclose(script);
        return;
    }

    Superblock sb;
    superblock_init(disk, &sb);
    inode_reset_counter();

    // Reserva blocos do superbloco e bitmap
    bitmap_set(disk, 0, 1);
    bitmap_set(disk, 1, 1);

    // Cria o diretório root
    if (dir_create_root(disk) != 0) {
        printf("[ERRO] Falha ao criar diretório root\n");
        disk_free(disk);
        fclose(script);
        return;
    }

    printf("[INFO] Sistema de arquivos inicializado com bloco de %zu bytes\n", block_size);

    // Processa cada comando do arquivo
    while (fgets(line, sizeof(line), script)) {
        // Remove newline e espaços extras
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0 || line[0] == '#') {
            continue; // Ignora linhas vazias e comentários
        }

        printf("\n[EXECUTANDO] %s\n", line);

        // Divide a linha em comando e argumentos
        char *args[MAX_ARGS] = {NULL};
        char *token = strtok(line, " ");
        int arg_count = 0;

        while (token != NULL && arg_count < MAX_ARGS) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }

        if (arg_count == 0) continue;

        // Processa o comando
        if (strcmp(args[0], "info") == 0) {
            // info [inode]
            uint32_t inode_num = (arg_count > 1) ? (uint32_t)atoi(args[1]) : current_dir_inode;
            Inode *inode = inode_load(disk, inode_num);
            if (!inode) {
                printf("[ERRO] Inode %u não encontrado.\n", inode_num);
                continue;
            }

            if ((inode->mode & 040000) == 040000) {
                // Diretório
                dir_list_detailed(disk, inode_num);
            } else {
                // Arquivo
                printf("Inode: %u\n", inode_num);
                printf("Tamanho: %u bytes\n", inode->size);
                printf("Criado em: %s", ctime(&inode->created_at));
                printf("Modificado em: %s", ctime(&inode->modified_at));
            }
            free(inode);
        }
        else if (strcmp(args[0], "create_file") == 0) {
            // create_file [diretorio] [arquivo_host] [nome_fs]
            if (arg_count < 4) {
                printf("[ERRO] Sintaxe: create_file [diretorio] [arquivo_host] [nome_fs]\n");
                continue;
            }
            uint32_t dir_inode = atoi(args[1]);
            if (file_create(disk, dir_inode, args[2], args[3]) == 0) {
                printf("Arquivo '%s' criado com sucesso no diretório %u.\n", args[3], dir_inode);
            } else {
                printf("[ERRO] Falha ao criar arquivo '%s'\n", args[3]);
            }
        }
        else if (strcmp(args[0], "list_dir") == 0) {
            // list_dir [inode]
            uint32_t dir_inode = (arg_count > 1) ? (uint32_t)atoi(args[1]) : current_dir_inode;
            if (dir_list(disk, dir_inode) != 0) {
                printf("[ERRO] Falha ao listar conteúdo do diretório %u\n", dir_inode);
            }
        }
        else if (strcmp(args[0], "create_dir") == 0) {
            // create_dir [diretorio_pai] [nome]
            if (arg_count < 3) {
                printf("[ERRO] Sintaxe: create_dir [diretorio_pai] [nome]\n");
                continue;
            }
            uint32_t parent_inode = atoi(args[1]);
            if (dir_create(disk, parent_inode, args[2]) == 0) {
                printf("Diretório '%s' criado com sucesso no diretório %u!\n", args[2], parent_inode);
            } else {
                printf("[ERRO] Falha ao criar diretório '%s'\n", args[2]);
            }
        }
        else if (strcmp(args[0], "rename_dir") == 0) {
            // rename_dir [diretorio_pai] [inode_dir] [novo_nome]
            if (arg_count < 4) {
                printf("[ERRO] Sintaxe: rename_dir [diretorio_pai] [inode_dir] [novo_nome]\n");
                continue;
            }
            uint32_t parent_inode = atoi(args[1]);
            uint32_t dir_inode = atoi(args[2]);
            if (dir_rename_entry(disk, parent_inode, dir_inode, args[3]) == 0) {
                printf("Diretório renomeado com sucesso!\n");
            } else {
                printf("[ERRO] Falha ao renomear diretório.\n");
            }
        }
        else if (strcmp(args[0], "delete_dir") == 0) {
            // delete_dir [diretorio_pai] [inode_dir]
            if (arg_count < 3) {
                printf("[ERRO] Sintaxe: delete_dir [diretorio_pai] [inode_dir]\n");
                continue;
            }
            uint32_t parent_inode = atoi(args[1]);
            uint32_t dir_inode = atoi(args[2]);
            
            // Remove entrada no diretório pai
            if (dir_remove_entry(disk, parent_inode, dir_inode) != 0) {
                printf("[ERRO] Falha ao remover entrada no diretório pai.\n");
                continue;
            }

            // Libera os blocos usados pelo diretório
            Inode *inode_apagar = inode_load(disk, dir_inode);
            if (!inode_apagar) {
                printf("[ERRO] Falha ao carregar inode do diretório.\n");
                continue;
            }
            for (int i = 0; i < MAX_BLOCKS_PER_INODE; i++) {
                if (inode_apagar->blocks[i] != 0 && inode_apagar->blocks[i] != (uint32_t)-1) {
                    bitmap_set(disk, inode_apagar->blocks[i], 0);
                }
            }
            free(inode_apagar);

            // Libera o inode
            inode_free(disk, dir_inode);
            printf("Diretório apagado com sucesso.\n");
        }
        else if (strcmp(args[0], "rename_file") == 0) {
            // rename_file [diretorio] [inode_file] [novo_nome]
            if (arg_count < 4) {
                printf("[ERRO] Sintaxe: rename_file [diretorio] [inode_file] [novo_nome]\n");
                continue;
            }
            uint32_t dir_inode = atoi(args[1]);
            uint32_t file_inode = atoi(args[2]);
            if (dir_rename_entry(disk, dir_inode, file_inode, args[3]) == 0) {
                printf("Arquivo renomeado com sucesso!\n");
            } else {
                printf("[ERRO] Falha ao renomear arquivo.\n");
            }
        }
        else if (strcmp(args[0], "move_file") == 0) {
            // move_file [diretorio_origem] [inode_file] [diretorio_destino]
            if (arg_count < 4) {
                printf("[ERRO] Sintaxe: move_file [diretorio_origem] [inode_file] [diretorio_destino]\n");
                continue;
            }
            uint32_t origem_inode = atoi(args[1]);
            uint32_t file_inode = atoi(args[2]);
            uint32_t destino_inode = atoi(args[3]);

            // Obter o nome do arquivo
            char nome_arquivo[MAX_NAME_LEN] = {0};
            Inode *origem_dir = inode_load(disk, origem_inode);
            if (!origem_dir) {
                printf("[ERRO] Diretório de origem inválido\n");
                continue;
            }

            uint32_t num_entries = origem_dir->size / DIR_ENTRY_SIZE;
            int encontrado = 0;
            
            for (uint32_t i = 0; i < num_entries; i++) {
                uint32_t block_idx = (i * DIR_ENTRY_SIZE) / disk->block_size;
                uint32_t offset = (i * DIR_ENTRY_SIZE) % disk->block_size;

                if (block_idx >= 10 || origem_dir->blocks[block_idx] == 0)
                    continue;

                DirEntry entry;
                lseek(disk->fd, origem_dir->blocks[block_idx] * disk->block_size + offset, SEEK_SET);
                read(disk->fd, &entry, sizeof(DirEntry));

                if (entry.inode_num == file_inode) {
                    strncpy(nome_arquivo, entry.name, MAX_NAME_LEN);
                    encontrado = 1;
                    break;
                }
            }
            free(origem_dir);

            if (!encontrado) {
                printf("[ERRO] Arquivo não encontrado no diretório de origem\n");
                continue;
            }

            // Remover do diretório de origem
            if (dir_remove_entry(disk, origem_inode, file_inode) != 0) {
                printf("[ERRO] Falha ao remover do diretório de origem.\n");
                continue;
            }

            // Adicionar no diretório de destino
            if (dir_add_entry(disk, destino_inode, file_inode, nome_arquivo) != 0) {
                printf("[ERRO] Falha ao adicionar no diretório destino.\n");
                continue;
            }

            printf("Arquivo movido com sucesso!\n");
        }
        else if (strcmp(args[0], "delete_file") == 0) {
            // delete_file [diretorio_pai] [inode_file]
            if (arg_count < 3) {
                printf("[ERRO] Sintaxe: delete_file [diretorio_pai] [inode_file]\n");
                continue;
            }
            uint32_t parent_inode = atoi(args[1]);
            uint32_t file_inode = atoi(args[2]);
            if (file_delete(disk, parent_inode, file_inode) == 0) {
                printf("Arquivo apagado com sucesso.\n");
            } else {
                printf("[ERRO] Falha ao apagar arquivo.\n");
            }
        }
        else if (strcmp(args[0], "read_file") == 0) {
            // read_file [inode_file]
            if (arg_count < 2) {
                printf("[ERRO] Sintaxe: read_file [inode_file]\n");
                continue;
            }
            uint32_t file_inode = atoi(args[1]);
            if (file_read(disk, file_inode) != 0) {
                printf("[ERRO] Falha ao ler o arquivo de inode %u.\n", file_inode);
            }
        }
        else if (strcmp(args[0], "cd") == 0) {
            // cd [inode_dir]
            if (arg_count < 2) {
                printf("[ERRO] Sintaxe: cd [inode_dir]\n");
                continue;
            }
            uint32_t new_dir = atoi(args[1]);
            Inode *inode = inode_load(disk, new_dir);
            if (!inode || (inode->mode & 040000) != 040000) {
                printf("[ERRO] O inode %u não é um diretório válido.\n", new_dir);
                if (inode) free(inode);
                continue;
            }
            free(inode);
            current_dir_inode = new_dir;
            printf("Diretório atual alterado para %u\n", current_dir_inode);
        }
        else {
            printf("[ERRO] Comando desconhecido: %s\n", args[0]);
        }
    }

    fclose(script);
    if (disk) disk_free(disk);
}