#include <stdio.h>
#include <stdlib.h>
#include "disk.h"
#include "superblock.h"
#include "inode.h"
#include "bitmap.c"

int main() {
    // Inicialização do disco e superbloco
    Disk *disk = disk_create("fs.bin", 10 * 1024 * 1024, 4096);
    Superblock sb;
    superblock_init(disk, &sb);
    // Após inicializar o disco:
    printf("Bloco 0 está %s\n", bitmap_get(disk, 0) ? "alocado" : "livre");
    uint32_t free_block = bitmap_find_free_block(disk);
    printf("Primeiro bloco livre: %u\n", free_block);
    // Teste de i-node
    Inode *root_inode = inode_create(disk, 040755); // Modo diretório
    inode_save(disk, 0, root_inode); // Salva i-node 0 (root)
    
    printf("I-node root criado!\n");
    printf("Tamanho: %u bytes\n", root_inode->size);
    printf("Criado em: %s", ctime(&root_inode->created_at));

    free(root_inode);
    disk_free(disk);
    return 0;
}