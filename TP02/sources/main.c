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

int main() {
    /* ====================== */
    /* 1. INICIALIZAÇÃO DO SISTEMA */
    /* ====================== */
    Disk *disk = disk_create("fs.bin", 10 * 1024 * 1024, 4096);
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
    printf("\n[INFO] Diretório root criado com sucesso!\n");
    printf("• Tamanho: %u bytes\n", root_inode->size);
    printf("• Criado em: %s", ctime(&root_inode->created_at));
    // DEBUG: Verificar root imediatamente após criação
    printf("\n[DEBUG] Root logo após criação:\n");
    dir_list(disk, 0);

    /* ====================== */
    /* 3. VERIFICAÇÃO DO BITMAP */
    /* ====================== */
    printf("\n[DEBUG] Verificação do bitmap:\n");
    printf("Bloco 0 (superbloco) está %s\n", bitmap_get(disk, 0) ? "alocado" : "livre");
    printf("Bloco 1 (bitmap) está %s\n", bitmap_get(disk, 1) ? "alocado" : "livre");

    uint32_t free_block = bitmap_find_free_block(disk);
    printf("Primeiro bloco livre: %u\n", free_block);

    /* ====================== */
    /* 4. CRIANDO DIRETÓRIO '/home' */
    /* ====================== */
    printf("\n[TESTE] Criando diretório '/home':\n");
    if (dir_create(disk, 0, "home") == 0) {
        printf("• Diretório '/home' criado com sucesso!\n");

        printf("\n[TESTE] Conteúdo do diretório raiz após criar '/home':\n");
        dir_list(disk, 0);
    } else {
        printf("• Falha ao criar '/home'\n");
    }

    /* ====================== */
    /* 5. CRIANDO ARQUIVO TESTE */
    /* ====================== */
    printf("\n[TESTE] Criando arquivo 'teste.txt' dentro do root:\n");
    file_create(disk, 0, "testes/teste1.txt", "teste.txt");

    printf("\n[TESTE] Listagem final do root após criar o arquivo:\n");
    dir_list(disk, 0);
    

    /* ====================== */
    /* 6. LENDO O ARQUIVO */
    /* ====================== */
    printf("\n[TESTE] Lendo conteúdo do arquivo (inode 2):\n");
    file_read(disk, 2);

    /* ====================== */
    /* 7. FINALIZAÇÃO */
    /* ====================== */
    free(root_inode);
    disk_free(disk);

    printf("\n[INFO] Sistema finalizado com sucesso.\n");
    return 0;
}