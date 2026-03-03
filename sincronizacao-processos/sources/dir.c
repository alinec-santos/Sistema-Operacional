#include "dir.h"
#include "inode.h"
#include "bitmap.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_BLOCKS_PER_INODE 10

int dir_create(Disk *disk, uint32_t parent_inode_num, const char *name) {
    // 1. Aloca um novo i-node para o diretório
    uint32_t new_inode_num = inode_alloc();
    Inode *new_dir = inode_create(040755); // 040755 = modo diretório
    if (!new_dir) return -1;

    // 2. Encontra um bloco livre e marca como usado
    uint32_t block_num = bitmap_find_free_block(disk);
    if (block_num == (uint32_t)-1) {
        free(new_dir);
        return -1;
    }
    bitmap_set(disk, block_num, 1);

    // 3. Configura o i-node do novo diretório
    new_dir->blocks[0] = block_num;
    new_dir->size = 2 * DIR_ENTRY_SIZE; // 2 entradas (. e ..)

    // 4. Cria as entradas padrão
    DirEntry entries[2];
    memset(entries, 0, sizeof(entries)); // Limpa toda a estrutura
    
    entries[0].inode_num = new_inode_num;      // Entrada para o próprio diretório
    strncpy(entries[0].name, ".", MAX_NAME_LEN - 1);
    entries[0].name[MAX_NAME_LEN - 1] = '\0';
    
    entries[1].inode_num = parent_inode_num;   // Entrada para o diretório pai
    strncpy(entries[1].name, "..", MAX_NAME_LEN - 1);
    entries[1].name[MAX_NAME_LEN - 1] = '\0';

    // 5. Escreve as entradas no bloco alocado
    lseek(disk->fd, block_num * disk->block_size, SEEK_SET);
    if (write(disk->fd, entries, sizeof(entries)) != sizeof(entries)) {
        bitmap_set(disk, block_num, 0); // Libera o bloco em caso de erro
        free(new_dir);
        return -1;
    }

    // 6. Salva o i-node do novo diretório
    inode_save(disk, new_inode_num, new_dir);

    // 7. Adiciona o novo diretório ao pai
    if (dir_add_entry(disk, parent_inode_num, new_inode_num, name) != 0) {
        printf("[ERRO] Falha ao adicionar entrada '%s' no diretório pai (inode %u)\n", name, parent_inode_num);
        bitmap_set(disk, block_num, 0); // Libera o bloco
        free(new_dir);
        return -1;
    }

    free(new_dir);
    return 0;
}

int dir_add_entry(Disk *disk, uint32_t dir_inode_num, uint32_t child_inode_num, const char *name) {
    Inode *dir_inode = inode_load(disk, dir_inode_num);
    if (!dir_inode) return -1;

    uint32_t num_entries = dir_inode->size / DIR_ENTRY_SIZE;
    uint32_t target_block_index = (num_entries * DIR_ENTRY_SIZE) / disk->block_size;
    uint32_t block_offset = (num_entries * DIR_ENTRY_SIZE) % disk->block_size;

    if (target_block_index >= 10) {
        printf("[ERRO] Diretório cheio! (Limite de 10 blocos por inode)\n");
        free(dir_inode);
        return -1;
    }

    // Se o bloco alvo ainda não foi alocado, aloque agora
    if (dir_inode->blocks[target_block_index] == 0) {
        uint32_t new_block = bitmap_find_free_block(disk);
        if (new_block == (uint32_t)-1) {
            printf("[ERRO] Sem blocos livres para expandir o diretório.\n");
            free(dir_inode);
            return -1;
        }
        bitmap_set(disk, new_block, 1);
        dir_inode->blocks[target_block_index] = new_block;
    }

    // Cria a nova entrada de diretório
    DirEntry new_entry;
    memset(&new_entry, 0, sizeof(DirEntry)); // Limpa a estrutura
    new_entry.inode_num = child_inode_num;
    strncpy(new_entry.name, name, MAX_NAME_LEN - 1);
    new_entry.name[MAX_NAME_LEN - 1] = '\0';

    // Grava a entrada no bloco certo, posição certa
    lseek(disk->fd, dir_inode->blocks[target_block_index] * disk->block_size + block_offset, SEEK_SET);
    if (write(disk->fd, &new_entry, sizeof(DirEntry)) != sizeof(DirEntry)) {
        printf("[ERRO] Falha ao escrever entrada de diretório.\n");
        free(dir_inode);
        return -1;
    }

    // Atualiza o tamanho do diretório
    dir_inode->size += DIR_ENTRY_SIZE;
    inode_save(disk, dir_inode_num, dir_inode);

    free(dir_inode);
    return 0;
}

int dir_list(Disk *disk, uint32_t inode_num) {
    Inode *dir = inode_load(disk, inode_num);
    if (!dir) {
        printf("[ERRO] Não foi possível carregar inode %u\n", inode_num);
        return -1;
    }

    // Garante que o inode seja de diretório
    if ((dir->mode & 040000) != 040000) {
        printf("[ERRO] Inode %u não é um diretório (mode: %o).\n", inode_num, dir->mode);
        free(dir);
        return -1;
    }

    uint32_t num_entries = dir->size / DIR_ENTRY_SIZE;
    //printf("Conteúdo do diretório (inode %u):\n", inode_num);
    
    if (num_entries == 0) {
        printf("  (vazio)\n");
        free(dir);
        return 0;
    }

    // Lê entrada por entrada para evitar problemas de buffer
    for (uint32_t i = 0; i < num_entries; i++) {
        uint32_t block_idx = (i * DIR_ENTRY_SIZE) / disk->block_size;
        uint32_t offset_in_block = (i * DIR_ENTRY_SIZE) % disk->block_size;
        
        if (block_idx >= 10 || dir->blocks[block_idx] == 0) {
            printf("  [ERRO] Entrada %u: bloco não alocado ou inválido\n", i);
            continue;
        }

        DirEntry entry;
        memset(&entry, 0, sizeof(DirEntry));
        
        // Lê a entrada específica
        lseek(disk->fd, dir->blocks[block_idx] * disk->block_size + offset_in_block, SEEK_SET);
        ssize_t bytes_read = read(disk->fd, &entry, sizeof(DirEntry));
        
        if (bytes_read != sizeof(DirEntry)) {
            printf("  [ERRO] Entrada %u: falha na leitura (%zd bytes lidos)\n", i, bytes_read);
            continue;
        }
        
        // Verifica se a entrada é válida
        if (entry.name[0] == '\0') {
            //printf("  [ERRO] Entrada %u: nome vazio\n", i);
        } else if (entry.name[0] < 32 || entry.name[0] > 126) {
            printf("  [ERRO] Entrada %u: nome corrompido (primeiro char: %d)\n", i, entry.name[0]);
        } else {
            // Garante que o nome seja terminado em null
            entry.name[MAX_NAME_LEN - 1] = '\0';
            printf("  [%u] %s\n", entry.inode_num, entry.name);
        }
    }

    free(dir);
    return 0;
}

int file_create(Disk *disk, uint32_t parent_inode_num, const char *host_filename, const char *fs_filename) {
    FILE *src = fopen(host_filename, "rb");
    if (!src) {
        printf("[ERRO] Não foi possível abrir o arquivo de origem: %s\n", host_filename);
        return -1;
    }

    // 1. Aloca um inode para o arquivo
    uint32_t new_inode_num = inode_alloc();
    Inode *inode = inode_create(0100644);  // Modo arquivo regular (rw-r--r--)

    // 2. Lê o conteúdo do arquivo real e escreve nos blocos do disco virtual
    uint8_t buffer[disk->block_size];
    size_t bytes_read;
    uint32_t block_index = 0;

    while ((bytes_read = fread(buffer, 1, disk->block_size, src)) > 0) {
        if (block_index >= 10) {
            printf("[ERRO] Arquivo muito grande! (Limite de 10 blocos)\n");
            free(inode);
            fclose(src);
            return -1;
        }

        uint32_t block_num = bitmap_find_free_block(disk);
        if (block_num == (uint32_t)-1) {
            printf("[ERRO] Sem blocos livres!\n");
            free(inode);
            fclose(src);
            return -1;
        }

        bitmap_set(disk, block_num, 1);
        inode->blocks[block_index++] = block_num;
        inode->size += bytes_read;

        lseek(disk->fd, block_num * disk->block_size, SEEK_SET);
        if (write(disk->fd, buffer, bytes_read) != (ssize_t)bytes_read) {
            printf("[ERRO] Falha ao escrever dados do arquivo\n");
            free(inode);
            fclose(src);
            return -1;
        }
    }

    fclose(src);

    // 3. Salva o inode
    inode_save(disk, new_inode_num, inode);

    // 4. Adiciona a entrada no diretório pai
    if (dir_add_entry(disk, parent_inode_num, new_inode_num, fs_filename) != 0) {
        printf("[ERRO] Falha ao adicionar arquivo '%s' no diretório\n", fs_filename);
        free(inode);
        return -1;
    }

    free(inode);
    return 0;
}

int file_read(Disk *disk, uint32_t inode_num) {
    Inode *inode = inode_load(disk, inode_num);
    if (!inode) {
        printf("[ERRO] Não foi possível carregar o inode %u\n", inode_num);
        return -1;
    }

    if ((inode->mode & 0100000) == 0) {
        printf("[ERRO] Inode %u não é um arquivo regular (mode: %o).\n", inode_num, inode->mode);
        free(inode);
        return -1;
    }

    printf("[INFO] Conteúdo do arquivo (inode %u):\n", inode_num);

    uint8_t buffer[disk->block_size];
    uint32_t remaining = inode->size;
    uint32_t block_index = 0;

    while (remaining > 0 && block_index < 10) {
        if (inode->blocks[block_index] == 0) {
            printf("[ERRO] Bloco %u não alocado para o arquivo\n", block_index);
            break;
        }

        uint32_t block_num = inode->blocks[block_index];
        uint32_t to_read = (remaining > disk->block_size) ? disk->block_size : remaining;

        lseek(disk->fd, block_num * disk->block_size, SEEK_SET);
        ssize_t bytes_read = read(disk->fd, buffer, to_read);
        
        if (bytes_read <= 0) {
            printf("[ERRO] Falha ao ler bloco %u do arquivo\n", block_index);
            break;
        }

        fwrite(buffer, 1, bytes_read, stdout);
        remaining -= bytes_read;
        block_index++;
    }

    printf("\n");
    free(inode);
    return 0;
}

int dir_list_detailed(Disk *disk, uint32_t inode_num) {
    Inode *dir = inode_load(disk, inode_num);
    if (!dir) {
        printf("[ERRO] Não foi possível carregar inode %u\n", inode_num);
        return -1;
    }

    if ((dir->mode & 040000) != 040000) {
        printf("[ERRO] Inode %u não é um diretório.\n", inode_num);
        free(dir);
        return -1;
    }

    uint32_t num_entries = dir->size / DIR_ENTRY_SIZE;
    printf("Conteúdo detalhado do diretório (inode %u):\n", inode_num);

    for (uint32_t i = 0; i < num_entries; i++) {
        uint32_t block_idx = (i * DIR_ENTRY_SIZE) / disk->block_size;
        uint32_t offset_in_block = (i * DIR_ENTRY_SIZE) % disk->block_size;

        if (block_idx >= 10 || dir->blocks[block_idx] == 0) {
            printf("  [ERRO] Entrada %u: bloco não alocado ou inválido\n", i);
            continue;
        }

        DirEntry entry;
        lseek(disk->fd, dir->blocks[block_idx] * disk->block_size + offset_in_block, SEEK_SET);
        ssize_t bytes_read = read(disk->fd, &entry, sizeof(DirEntry));

        if (bytes_read != sizeof(DirEntry) || entry.name[0] == '\0') {
            continue; // ignora entradas inválidas
        }

        Inode *entry_inode = inode_load(disk, entry.inode_num);
        if (!entry_inode) {
            printf("  [%u] %s - [ERRO ao carregar inode]\n", entry.inode_num, entry.name);
            continue;
        }

        // Exibe informações
        printf("  [%u] %-15s | Tamanho: %u bytes | Criado em: %s",
               entry.inode_num,
               entry.name,
               entry_inode->size,
               ctime(&entry_inode->created_at));

        free(entry_inode);
    }

    free(dir);
    return 0;
}

int dir_list_dirs(Disk *disk, uint32_t inode_num) {
    Inode *dir = inode_load(disk, inode_num);
    if (!dir) {
        printf("[ERRO] Não foi possível carregar inode %u\n", inode_num);
        return -1;
    }

    if ((dir->mode & 040000) != 040000) {
        printf("[ERRO] Inode %u não é um diretório.\n", inode_num);
        free(dir);
        return -1;
    }

    uint32_t num_entries = dir->size / DIR_ENTRY_SIZE;
    printf("Diretórios dentro do inode %u:\n", inode_num);

    for (uint32_t i = 0; i < num_entries; i++) {
        uint32_t block_idx = (i * DIR_ENTRY_SIZE) / disk->block_size;
        uint32_t offset_in_block = (i * DIR_ENTRY_SIZE) % disk->block_size;

        if (block_idx >= 10 || dir->blocks[block_idx] == 0)
            continue;

        DirEntry entry;
        lseek(disk->fd, dir->blocks[block_idx] * disk->block_size + offset_in_block, SEEK_SET);
        ssize_t bytes_read = read(disk->fd, &entry, sizeof(DirEntry));
        if (bytes_read != sizeof(DirEntry)) continue;
        if (entry.name[0] == '\0') continue;

        Inode *entry_inode = inode_load(disk, entry.inode_num);
        if (!entry_inode) continue;

        if ((entry_inode->mode & 040000) == 040000) {  // É diretório
            printf("  [%u] %s\n", entry.inode_num, entry.name);
        }

        free(entry_inode);
    }
    free(dir);
    return 0;
}

uint32_t navegar_diretorios(Disk *disk, uint32_t starting_inode) {
    uint32_t current_inode = starting_inode;
    char resposta[10];

    while (1) {
        printf("\n--- Conteúdo do diretório (inode %u) ---\n", current_inode);
        dir_list(disk, current_inode);

        printf("\nDeseja entrar em algum subdiretório? (s/n): ");
        fgets(resposta, sizeof(resposta), stdin);

        if (resposta[0] != 's' && resposta[0] != 'S') {
            return current_inode; // usuário quer ficar nesse diretório
        }

        printf("Digite o inode do subdiretório: ");
        uint32_t proximo_inode;
        if (scanf("%u", &proximo_inode) != 1) {
            printf("[ERRO] Entrada inválida. Tente novamente.\n");
            while (getchar() != '\n');
            continue;
        }
        getchar(); // limpar '\n'

        Inode *inode = inode_load(disk, proximo_inode);
        if (!inode || (inode->mode & 040000) != 040000) {
            printf("[ERRO] O inode %u não é um diretório válido.\n", proximo_inode);
            if (inode) free(inode);
            continue;
        }

        free(inode);
        current_inode = proximo_inode; // navega para o próximo diretório
    }
}

int dir_rename_entry(Disk *disk, uint32_t parent_inode_num, uint32_t child_inode_num, const char *novo_nome) {
    Inode *parent_inode = inode_load(disk, parent_inode_num);
    if (!parent_inode) return -1;

    if ((parent_inode->mode & 040000) != 040000) {
        printf("[ERRO] Inode %u não é um diretório.\n", parent_inode_num);
        free(parent_inode);
        return -1;
    }

    uint32_t num_entries = parent_inode->size / DIR_ENTRY_SIZE;

    for (uint32_t i = 0; i < num_entries; i++) {
        uint32_t block_idx = (i * DIR_ENTRY_SIZE) / disk->block_size;
        uint32_t offset_in_block = (i * DIR_ENTRY_SIZE) % disk->block_size;

        if (block_idx >= 10 || parent_inode->blocks[block_idx] == 0)
            continue;

        DirEntry entry;
        lseek(disk->fd, parent_inode->blocks[block_idx] * disk->block_size + offset_in_block, SEEK_SET);
        ssize_t bytes_read = read(disk->fd, &entry, sizeof(DirEntry));
        if (bytes_read != sizeof(DirEntry)) continue;

        if (entry.inode_num == child_inode_num) {
            // Renomear
            strncpy(entry.name, novo_nome, MAX_NAME_LEN - 1);
            entry.name[MAX_NAME_LEN - 1] = '\0';

            lseek(disk->fd, parent_inode->blocks[block_idx] * disk->block_size + offset_in_block, SEEK_SET);
            if (write(disk->fd, &entry, sizeof(DirEntry)) != sizeof(DirEntry)) {
                printf("[ERRO] Falha ao escrever a entrada renomeada.\n");
                free(parent_inode);
                return -1;
            }

            free(parent_inode);
            return 0;  // sucesso
        }
    }

    free(parent_inode);
    printf("[ERRO] Entrada com inode %u não encontrada no diretório %u.\n", child_inode_num, parent_inode_num);
    return -1;
}

int dir_remove_entry(Disk *disk, uint32_t dir_inode_num, uint32_t target_inode_num) {
    Inode *dir_inode = inode_load(disk, dir_inode_num);
    if (!dir_inode) return -1;

    if ((dir_inode->mode & 040000) != 040000) {
        free(dir_inode);
        return -1;
    }

    uint32_t num_entries = dir_inode->size / DIR_ENTRY_SIZE;
    int found = 0;

    for (uint32_t i = 0; i < num_entries; i++) {
        uint32_t block_idx = (i * DIR_ENTRY_SIZE) / disk->block_size;
        uint32_t offset_in_block = (i * DIR_ENTRY_SIZE) % disk->block_size;

        if (block_idx >= MAX_BLOCKS_PER_INODE || dir_inode->blocks[block_idx] == 0)
            continue;

        DirEntry entry;
        lseek(disk->fd, dir_inode->blocks[block_idx] * disk->block_size + offset_in_block, SEEK_SET);
        ssize_t bytes_read = read(disk->fd, &entry, sizeof(DirEntry));
        if (bytes_read != sizeof(DirEntry)) continue;

        if (entry.inode_num == target_inode_num) {
            // Para "remover", vamos zerar o nome para invalidar a entrada
            memset(&entry.name, 0, sizeof(entry.name));
            entry.inode_num = 0;

            lseek(disk->fd, dir_inode->blocks[block_idx] * disk->block_size + offset_in_block, SEEK_SET);
            if (write(disk->fd, &entry, sizeof(DirEntry)) != sizeof(DirEntry)) {
                free(dir_inode);
                return -1;
            }
            found = 1;
            break;
        }
    }

    free(dir_inode);

    if (!found) return -1;

    return 0;
}

uint32_t dir_find_parent_recursive(Disk *disk, uint32_t current_inode_num, uint32_t target_inode_num) {
    Inode *current_inode = inode_load(disk, current_inode_num);
    if (!current_inode) return (uint32_t)-1;

    if ((current_inode->mode & 040000) != 040000) { // Se não for diretório, libera e retorna
        free(current_inode);
        return (uint32_t)-1;
    }

    uint32_t num_entries = current_inode->size / DIR_ENTRY_SIZE;

    for (uint32_t i = 0; i < num_entries; i++) {
        uint32_t block_idx = (i * DIR_ENTRY_SIZE) / disk->block_size;
        uint32_t offset_in_block = (i * DIR_ENTRY_SIZE) % disk->block_size;

        if (block_idx >= MAX_BLOCKS_PER_INODE || current_inode->blocks[block_idx] == 0)
            continue;

        DirEntry entry;
        lseek(disk->fd, current_inode->blocks[block_idx] * disk->block_size + offset_in_block, SEEK_SET);
        ssize_t bytes_read = read(disk->fd, &entry, sizeof(DirEntry));
        if (bytes_read != sizeof(DirEntry)) continue;

        // Ignorar "." e ".."
        if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) continue;

        if (entry.inode_num == target_inode_num) {
            free(current_inode);
            return current_inode_num; // Achei o pai
        }

        // Se for diretório, busca recursivamente dentro dele
        Inode *entry_inode = inode_load(disk, entry.inode_num);
        if (entry_inode && (entry_inode->mode & 040000) == 040000) {
            uint32_t res = dir_find_parent_recursive(disk, entry.inode_num, target_inode_num);
            free(entry_inode);
            if (res != (uint32_t)-1) {
                free(current_inode);
                return res; // Pai encontrado recursivamente
            }
        } else {
            if(entry_inode) free(entry_inode);
        }
    }

    free(current_inode);
    return (uint32_t)-1; // Não achou
}

uint32_t dir_find_parent(Disk *disk, uint32_t target_inode_num) {
    // Começa pelo root (inode 0)
    if (target_inode_num == 0) return (uint32_t)-1; // root não tem pai
    return dir_find_parent_recursive(disk, 0, target_inode_num);
}

int file_delete(Disk *disk, uint32_t parent_inode_num, uint32_t file_inode_num) {
    Inode *file_inode = inode_load(disk, file_inode_num);
    if (!file_inode) return -1;

    if ((file_inode->mode & 0100000) != 0100000) {
        printf("[ERRO] Inode %u não é um arquivo regular.\n", file_inode_num);
        free(file_inode);
        return -1;
    }

    // Libera todos os blocos usados pelo arquivo
    for (int i = 0; i < MAX_BLOCKS_PER_INODE; i++) {
        if (file_inode->blocks[i] != 0 && file_inode->blocks[i] != (uint32_t)-1) {
            bitmap_set(disk, file_inode->blocks[i], 0);
        }
    }

    // Libera o inode
    inode_free(disk, file_inode_num);

    // Remove entrada do diretório pai
    if (dir_remove_entry(disk, parent_inode_num, file_inode_num) != 0) {
        printf("[ERRO] Falha ao remover a entrada do diretório pai.\n");
        free(file_inode);
        return -1;
    }

    free(file_inode);
    return 0;
}
