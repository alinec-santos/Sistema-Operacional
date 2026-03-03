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
void print_header(const char *title) {
    printf("\n====================================\n");
    printf("  %s\n", title);
    printf("====================================\n");
}

void print_menu() {
    print_header("SISTEMA DE ARQUIVOS SIMULADO");

    printf("1 - Verificar informações do root\n"); //ok
    printf("2 - Criar novo arquivo \n"); //ok
    printf("3 - Listar conteúdo de um diretório\n"); //ok
    printf("4 - Criar novo diretório \n");
    printf("5 - Renomear diretório\n");
    printf("6 - Apagar diretório \n");
    printf("7 - Renomear arquivo\n");
    printf("8 - Mover arquivo \n");
    printf("9 - Apagar arquivo\n");
    printf("10 - Listar conteúdo de um arquivo\n"); //ok
    printf("0 - Sair\n");
    printf("------------------------------------\n");
    printf("Escolha a opção: ");
}

int modointerativo() {
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

            case 1: {
                print_header("INFORMAÇÕES DETALHADAS");

                printf("Navegue até o diretório ou arquivo que deseja consultar:\n");
                uint32_t chosen_inode = navegar_diretorios(disk, 0);

                Inode *inode = inode_load(disk, chosen_inode);
                if (!inode) {
                    printf("[ERRO] Inode %u não encontrado.\n", chosen_inode);
                    break;
                }

                if ((inode->mode & 040000) == 040000) {
                    // Diretório
                    dir_list_detailed(disk, chosen_inode);
                } else {
                    // Arquivo
                    print_header("INFORMAÇÕES DO ARQUIVO");
                    printf("Inode: %u\n", chosen_inode);
                    printf("Tamanho: %u bytes\n", inode->size);
                    printf("Criado em: %s", ctime(&inode->created_at));
                    printf("Modificado em: %s", ctime(&inode->modified_at));
                    // Pode adicionar outras infos relevantes
                }
                free(inode);
                break;
            }

            case 2: {
                print_header("CRIAR NOVO ARQUIVO");
                printf("[INFO] Navegue até o diretório onde deseja criar o arquivo:\n");
                uint32_t dir_inode = navegar_diretorios(disk, 0);  // navegação interativa

               // while (getchar() != '\n');  // limpa buffer residual

                char nome_arquivo[256];
                printf("Digite o nome do arquivo a ser criado: ");
                fgets(nome_arquivo, sizeof(nome_arquivo), stdin);
                nome_arquivo[strcspn(nome_arquivo, "\n")] = 0; // remove '\n'

                char caminho_arquivo_real[512];
                printf("Digite o caminho do arquivo real para importar conteúdo: ");
                fgets(caminho_arquivo_real, sizeof(caminho_arquivo_real), stdin);
                caminho_arquivo_real[strcspn(caminho_arquivo_real, "\n")] = 0;

                if (file_create(disk, dir_inode, caminho_arquivo_real, nome_arquivo) == 0) {
                    printf("Arquivo '%s' criado com sucesso no inode %u.\n", nome_arquivo,dir_inode);
                } else {
                    printf("Falha ao criar arquivo '%s' \n", nome_arquivo);
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

            case 4: {
                print_header("CRIAR NOVO DIRETÓRIO");
                uint32_t destino = navegar_diretorios(disk, 0);
                char nome[256];
                printf("Digite o nome do novo diretório: ");
                fgets(nome, sizeof(nome), stdin);
                nome[strcspn(nome, "\n")] = 0;
                if (dir_create(disk, destino, nome) == 0)
                    printf("Diretório '%s' criado com sucesso no inode %u!\n", nome, destino);
                else
                    printf("[ERRO] Falha ao criar o diretório '%s'\n", nome);
                break;
            }

            case 5: {
                print_header("RENOMEAR DIRETÓRIO");

                // Passo 1: navegar até o diretório pai onde está o diretório a renomear
                uint32_t parent_inode = navegar_diretorios(disk, 0);
                
                // Listar diretórios dentro do escolhido
                printf("Diretórios dentro do inode %u:\n", parent_inode);
                if (dir_list_dirs(disk, parent_inode) != 0) {
                    printf("[ERRO] Falha ao listar diretórios.\n");
                    break;
                }

                // Escolher diretório a renomear (pelo inode)
                printf("Digite o inode do diretório que deseja renomear: ");
                uint32_t inode_dir;
                if (scanf("%u", &inode_dir) != 1) {
                    printf("[ERRO] Entrada inválida!\n");
                    while(getchar() != '\n');
                    break;
                }
                getchar();

                // Verificar se é diretório (para evitar renomear arquivo)
                Inode *check_inode = inode_load(disk, inode_dir);
                if (!check_inode) {
                    printf("[ERRO] Inode não existe.\n");
                    break;
                }
                if ((check_inode->mode & 040000) != 040000) {
                    printf("[ERRO] Esse inode não é um diretório e não pode ser renomeado.\n");
                    free(check_inode);
                    break;
                }
                free(check_inode);

                // Pedir novo nome
                char novo_nome[256];
                printf("Digite o novo nome para o diretório: ");
                fgets(novo_nome, sizeof(novo_nome), stdin);
                novo_nome[strcspn(novo_nome, "\n")] = 0;

                // Renomear
                if (dir_rename_entry(disk, parent_inode, inode_dir, novo_nome) == 0) {
                    printf("Diretório renomeado com sucesso!\n");
                } else {
                    printf("[ERRO] Falha ao renomear diretório.\n");
                }

                break;
            }

            case 6: {
                print_header("APAGAR DIRETÓRIO");
                uint32_t current_inode = 0; // começa pelo root
                uint32_t parent_inode = (uint32_t)-1;
                (void) parent_inode;
                char resposta[10];

                while (1) {
                    printf("\nDiretórios dentro do inode %u:\n", current_inode);
                    if (dir_list_dirs(disk, current_inode) != 0) {
                        printf("[ERRO] Falha ao listar diretórios.\n");
                        break;
                    }

                    printf("Deseja entrar em algum subdiretório? (s/n): ");
                    fgets(resposta, sizeof(resposta), stdin);
                    if (resposta[0] != 's' && resposta[0] != 'S') {
                        // Quer apagar o diretório atual?
                        if (current_inode == 0) {
                            printf("[AVISO] Não é permitido apagar o diretório root.\n");
                            break;
                        }

                        printf("Confirma apagar o diretório com inode %u? (s/n): ", current_inode);
                        fgets(resposta, sizeof(resposta), stdin);
                        if (resposta[0] == 's' || resposta[0] == 'S') {
                            // Encontra o pai
                            uint32_t pai = dir_find_parent(disk, current_inode);
                            if (pai == (uint32_t)-1) {
                                printf("[ERRO] Não foi possível encontrar o diretório pai.\n");
                                break;
                            }

                            // Remove entrada no diretório pai
                            if (dir_remove_entry(disk, pai, current_inode) != 0) {
                                printf("[ERRO] Falha ao remover entrada no diretório pai.\n");
                                break;
                            }

                            // Libera os blocos usados pelo diretório a apagar
                            Inode *inode_apagar = inode_load(disk, current_inode);
                            if (!inode_apagar) {
                                printf("[ERRO] Falha ao carregar inode do diretório.\n");
                                break;
                            }
                            for (int i = 0; i < MAX_BLOCKS_PER_INODE; i++) {
                                if (inode_apagar->blocks[i] != 0 && inode_apagar->blocks[i] != (uint32_t)-1) {
                                    bitmap_set(disk, inode_apagar->blocks[i], 0);
                                }
                            }
                            free(inode_apagar);

                            // Libera o inode
                            inode_free(disk, current_inode);


                            printf("Diretório apagado com sucesso.\n");
                        } else {
                            printf("Operação cancelada.\n");
                        }

                        break;
                }

                    // Entrar em subdiretório escolhido
                    printf("Digite o inode do subdiretório: ");
                    uint32_t proximo_inode;
                    if (scanf("%u", &proximo_inode) != 1) {
                        printf("[ERRO] Entrada inválida.\n");
                        while (getchar() != '\n'); // limpar buffer
                        continue;
                    }
                    getchar();

                    Inode *inode_verif = inode_load(disk, proximo_inode);
                    if (!inode_verif || (inode_verif->mode & 040000) != 040000) {
                        printf("[ERRO] Inode %u não é um diretório válido.\n", proximo_inode);
                        if (inode_verif) free(inode_verif);
                        continue;
                    }
                    free(inode_verif);

                    parent_inode = current_inode;
                    current_inode = proximo_inode;
                }
                break;
            }

            case 7: {
                uint32_t dir_inode = navegar_diretorios(disk, 0);  // Navega até o diretório desejado
                dir_list(disk, dir_inode);  // Lista arquivos e diretórios

                printf("\nDigite o inode do ARQUIVO que deseja renomear: ");
                uint32_t alvo_inode;
                if (scanf("%u", &alvo_inode) != 1) {
                    printf("[ERRO] Entrada inválida!\n");
                    while (getchar() != '\n');
                    break;
                }
                getchar(); // limpar \n

                // Verificar se o inode é um arquivo regular
                Inode *inode = inode_load(disk, alvo_inode);
                if (!inode) {
                    printf("[ERRO] Inode inválido!\n");
                    break;
                }

                if ((inode->mode & 0100000) != 0100000) {
                    printf("[ERRO] O inode %u não é um arquivo. Apenas arquivos podem ser renomeados aqui.\n", alvo_inode);
                    free(inode);
                    break;
                }
                free(inode);

                printf("Digite o novo nome: ");
                char novo_nome[MAX_NAME_LEN];
                fgets(novo_nome, sizeof(novo_nome), stdin);
                novo_nome[strcspn(novo_nome, "\n")] = '\0';  // remove newline

                if (dir_rename_entry(disk, dir_inode, alvo_inode, novo_nome) == 0) {
                    printf("[INFO] Arquivo renomeado com sucesso.\n");
                } else {
                    printf("[ERRO] Falha ao renomear o arquivo.\n");
                }

                break;
            }

            case 8: { // Mover arquivo
                printf("Selecione o diretório onde o arquivo está:\n");
                uint32_t origem_inode = navegar_diretorios(disk, 0);

                // Listar arquivos no diretório de origem
                Inode *origem_dir = inode_load(disk, origem_inode);
                if (!origem_dir) break;

                printf("\nArquivos disponíveis no diretório [%u]:\n", origem_inode);
                uint32_t num_entries = origem_dir->size / DIR_ENTRY_SIZE;

                for (uint32_t i = 0; i < num_entries; i++) {
                    uint32_t block_idx = (i * DIR_ENTRY_SIZE) / disk->block_size;
                    uint32_t offset = (i * DIR_ENTRY_SIZE) % disk->block_size;

                    if (block_idx >= 10 || origem_dir->blocks[block_idx] == 0)
                        continue;

                    DirEntry entry;
                    lseek(disk->fd, origem_dir->blocks[block_idx] * disk->block_size + offset, SEEK_SET);
                    if (read(disk->fd, &entry, sizeof(DirEntry)) != sizeof(DirEntry))
                        continue;

                    Inode *entry_inode = inode_load(disk, entry.inode_num);
                    if (!entry_inode) continue;

                    if ((entry_inode->mode & 0100000) == 0100000) {
                        printf("  [%u] %s\n", entry.inode_num, entry.name);
                    }

                    free(entry_inode);
                }

                free(origem_dir);

                printf("Digite o inode do arquivo a mover: ");
                uint32_t inode_arquivo;
                if (scanf("%u", &inode_arquivo) != 1) {
                    while (getchar() != '\n');
                    break;
                }
                getchar(); // limpar \n

                Inode *inode = inode_load(disk, inode_arquivo);
                if (!inode || (inode->mode & 0100000) != 0100000) {
                    printf("[ERRO] O inode %u não é um arquivo.\n", inode_arquivo);
                    if (inode) free(inode);
                    break;
                }
                free(inode);

                // Obter o nome do arquivo
                char nome_arquivo[MAX_NAME_LEN] = {0};
                origem_dir = inode_load(disk, origem_inode);
                for (uint32_t i = 0; i < num_entries; i++) {
                    uint32_t block_idx = (i * DIR_ENTRY_SIZE) / disk->block_size;
                    uint32_t offset = (i * DIR_ENTRY_SIZE) % disk->block_size;

                    if (block_idx >= 10 || origem_dir->blocks[block_idx] == 0)
                        continue;

                    DirEntry entry;
                    lseek(disk->fd, origem_dir->blocks[block_idx] * disk->block_size + offset, SEEK_SET);
                    read(disk->fd, &entry, sizeof(DirEntry));

                    if (entry.inode_num == inode_arquivo) {
                        strncpy(nome_arquivo, entry.name, MAX_NAME_LEN);
                        break;
                    }
                }
                free(origem_dir);

                // Navegar até diretório de destino
                printf("Agora selecione o diretório destino:\n");
                uint32_t destino_inode = navegar_diretorios(disk, 0);

                // Remover do diretório de origem
                if (dir_remove_entry(disk, origem_inode, inode_arquivo) != 0) {
                    printf("[ERRO] Falha ao remover do diretório de origem.\n");
                    break;
                }

                // Adicionar no diretório de destino
                if (dir_add_entry(disk, destino_inode, inode_arquivo, nome_arquivo) != 0) {
                    printf("[ERRO] Falha ao adicionar no diretório destino.\n");
                    break;
                }

                printf("[OK] Arquivo movido com sucesso!\n");
                break;
            }

            case 9: { // Apagar arquivo
                printf("Selecione o diretório onde está o arquivo a ser apagado:\n");
                uint32_t parent_inode = navegar_diretorios(disk, 0);

                dir_list(disk, parent_inode);
                printf("Digite o inode do arquivo a apagar: ");
                uint32_t file_inode;
                if (scanf("%u", &file_inode) != 1) {
                    while (getchar() != '\n');
                    break;
                }
                getchar(); // limpar \n

                Inode *inode = inode_load(disk, file_inode);
                if (!inode || (inode->mode & 0100000) != 0100000) {
                    printf("[ERRO] O inode %u não é um arquivo válido.\n", file_inode);
                    if (inode) free(inode);
                    break;
                }
                free(inode);

                if (file_delete(disk, parent_inode, file_inode) == 0) {
                    printf("[OK] Arquivo apagado com sucesso.\n");
                } else {
                    printf("[ERRO] Falha ao apagar arquivo.\n");
                }

                break;
            }

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

}
