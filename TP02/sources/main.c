#include <stdio.h>
#include <stdlib.h>
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

    /* ====================== */
    /* 2. VERIFICAÇÃO DO BITMAP (TESTE) */
    /* ====================== */
    printf("\n[DEBUG] Verificação do bitmap:\n");
    printf("Bloco 0 (superbloco) está %s\n", bitmap_get(disk, 0) ? "alocado" : "livre");
    printf("Bloco 1 (bitmap) está %s\n", bitmap_get(disk, 1) ? "alocado" : "livre");
    
    uint32_t free_block = bitmap_find_free_block(disk);
    printf("Primeiro bloco livre: %u\n", free_block);

    /* ====================== */
    /* 3. CRIAÇÃO DA ESTRUTURA BÁSICA */
    /* ====================== */
    // Cria i-node root (diretório raiz)
    uint32_t root_inode_num = inode_alloc();
    Inode *root_inode = inode_create(040755);
    inode_save(disk, root_inode_num, root_inode);

    
    printf("\n[INFO] I-node root criado:\n");
    printf("• Tamanho: %u bytes\n", root_inode->size);
    printf("• Criado em: %s", ctime(&root_inode->created_at));

    /* ====================== */
    /* 4. OPERAÇÕES DE DIRETÓRIO (NOVO!) */
    /* ====================== */
    printf("\n[TESTE] Criando estrutura de diretórios:\n");
    
    // Cria /home dentro do root
    if (dir_create(disk, 0, "home") == 0) {
        printf("• Diretório '/home' criado com sucesso!\n");
        
        // Lista conteúdo do root (deve mostrar '.' e 'home')
        printf("\nConteúdo do diretório raiz:\n");
        dir_list(disk, 0);
    } else {
        printf("• Falha ao criar '/home'\n");
    }
    printf("\n[TESTE] Criando arquivo de exemplo:\n");
    file_create(disk, 0, "testes/teste1.txt", "teste.txt");

    printf("\nConteúdo do diretório raiz após criar o arquivo:\n");
    dir_list(disk, 0);

    printf("\n[TESTE] Lendo conteúdo do arquivo:\n");
    file_read(disk, 2);
    /* ====================== */
    /* 5. FINALIZAÇÃO */
    /* ====================== */
    free(root_inode);
    disk_free(disk);
    
    printf("\nSistema finalizado com sucesso.\n");
    return 0;
}