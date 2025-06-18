#include "dir.h"
#include "inode.h"
#include "bitmap.h"
#include <unistd.h>  // Para lseek, read, write
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int dir_create(Disk *disk, uint32_t parent_inode_num, const char *name) {
    // 1. Aloca um novo i-node para o diretório
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
    uint32_t new_inode_num = inode_alloc();

    // 4. Cria as entradas padrão
    DirEntry entries[2] = {
        {new_inode_num, "."},      // Entrada para o próprio diretório
        {parent_inode_num, ".."}   // Entrada para o diretório pai
    };

    // 5. Escreve as entradas no bloco alocado
    lseek(disk->fd, block_num * disk->block_size, SEEK_SET);
    if (write(disk->fd, entries, sizeof(entries)) != sizeof(entries)) {
        bitmap_set(disk, block_num, 0); // Libera o bloco em caso de erro
        free(new_dir);
        return -1;
    }

    // 6. Salva o i-node do novo diretório
    inode_save(disk, new_inode_num, new_dir);

    // Adiciona o novo diretório ao pai
    dir_add_entry(disk, parent_inode_num, new_inode_num, name);


    free(new_dir);
    return 0;
    (void)name;
}
int dir_add_entry(Disk *disk, uint32_t dir_inode_num, uint32_t child_inode_num, const char *name) {
    Inode *dir_inode = inode_load(disk, dir_inode_num);
    if (!dir_inode) return -1;

    uint32_t block_num = dir_inode->blocks[0];
    uint32_t num_entries = dir_inode->size / DIR_ENTRY_SIZE;

    DirEntry new_entry;
    new_entry.inode_num = child_inode_num;
    strncpy(new_entry.name, name, MAX_NAME_LEN - 1);
    new_entry.name[MAX_NAME_LEN - 1] = '\0';

    lseek(disk->fd, block_num * disk->block_size + num_entries * DIR_ENTRY_SIZE, SEEK_SET);
    if (write(disk->fd, &new_entry, sizeof(DirEntry)) != sizeof(DirEntry)) {
        free(dir_inode);
        return -1;
    }

    // Atualiza tamanho do diretório
    dir_inode->size += DIR_ENTRY_SIZE;
    inode_save(disk, dir_inode_num, dir_inode);

    free(dir_inode);
    return 0;
}

int dir_list(Disk *disk, uint32_t inode_num) {
    // 1. Carrega o i-node do diretório
    Inode *dir = inode_load(disk, inode_num);
    if (!dir || !(dir->mode & 040000)) {
        if (dir) free(dir);
        return -1;
    }

    // 2. Lê o bloco do diretório
    DirEntry entries[dir->size / DIR_ENTRY_SIZE];
    lseek(disk->fd, dir->blocks[0] * disk->block_size, SEEK_SET);
    if (read(disk->fd, entries, dir->size) != dir->size) {
        free(dir);
        return -1;
    }

    // 3. Imprime as entradas
    printf("Conteúdo do diretório (inode %u):\n", inode_num);
    for (uint32_t  i = 0; i < dir->size / DIR_ENTRY_SIZE; i++) {
        printf("%8u %s\n", entries[i].inode_num, entries[i].name);
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
        write(disk->fd, buffer, bytes_read);
    }

    fclose(src);

    // 3. Salva o inode
    inode_save(disk, new_inode_num, inode);

    // 4. Adiciona a entrada no diretório pai
    dir_add_entry(disk, parent_inode_num, new_inode_num, fs_filename);

    free(inode);
    printf("[INFO] Arquivo '%s' criado com sucesso no FS!\n", fs_filename);
    return 0;
}

int file_read(Disk *disk, uint32_t inode_num) {
    Inode *inode = inode_load(disk, inode_num);
    if (!inode) {
        printf("[ERRO] Não foi possível carregar o inode %u\n", inode_num);
        return -1;
    }

    if ((inode->mode & 0100000) == 0) {
        printf("[ERRO] Inode %u não é um arquivo regular.\n", inode_num);
        free(inode);
        return -1;
    }

    printf("[INFO] Conteúdo do arquivo (inode %u):\n", inode_num);

    uint8_t buffer[disk->block_size];
    uint32_t remaining = inode->size;
    uint32_t block_index = 0;

    while (remaining > 0 && block_index < 10) {  // 10 blocos é o máximo que suportamos no inode
        uint32_t block_num = inode->blocks[block_index];
        uint32_t to_read = (remaining > disk->block_size) ? disk->block_size : remaining;

        lseek(disk->fd, block_num * disk->block_size, SEEK_SET);
        read(disk->fd, buffer, to_read);

        fwrite(buffer, 1, to_read, stdout);  // Imprime o conteúdo
        remaining -= to_read;
        block_index++;
    }

    printf("\n");

    free(inode);
    return 0;
}

