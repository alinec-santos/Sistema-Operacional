#include "dir.h"
#include "inode.h"
#include "bitmap.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    printf("Conteúdo do diretório (inode %u):\n", inode_num);
    
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
            printf("  [ERRO] Entrada %u: nome vazio\n", i);
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

int dir_create_root(Disk *disk) {
    // Aloca inode 0
    uint32_t inode_num = inode_alloc();
    if (inode_num != 0) {
        printf("[ERRO] Root precisa ser o inode 0!\n");
        return -1;
    }

    Inode *root_inode = inode_create(040755);  // Modo diretório
    uint32_t block_num = bitmap_find_free_block(disk);
    if (block_num == (uint32_t)-1) {
        printf("[ERRO] Sem blocos livres para root!\n");
        free(root_inode);
        return -1;
    }

    bitmap_set(disk, block_num, 1);
    root_inode->blocks[0] = block_num;
    root_inode->size = 2 * DIR_ENTRY_SIZE;

    // Cria "." e ".." com inicialização adequada
    DirEntry entries[2];
    memset(entries, 0, sizeof(entries));
    
    entries[0].inode_num = 0;
    strncpy(entries[0].name, ".", MAX_NAME_LEN - 1);
    entries[0].name[MAX_NAME_LEN - 1] = '\0';
    
    entries[1].inode_num = 0;
    strncpy(entries[1].name, "..", MAX_NAME_LEN - 1);
    entries[1].name[MAX_NAME_LEN - 1] = '\0';

    lseek(disk->fd, block_num * disk->block_size, SEEK_SET);
    if (write(disk->fd, entries, sizeof(entries)) != sizeof(entries)) {
        printf("[ERRO] Falha ao escrever entradas do diretório root\n");
        bitmap_set(disk, block_num, 0);
        free(root_inode);
        return -1;
    }

    inode_save(disk, inode_num, root_inode);
    free(root_inode);

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
