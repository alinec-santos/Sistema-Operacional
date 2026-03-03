#ifndef SUPERBLOCK_H
#define SUPERBLOCK_H

#include "disk.h"

typedef struct {
    uint32_t magic;         // Número mágico (identificação)
    uint32_t disk_size;     // Tamanho total
    uint32_t block_size;    // Tamanho do bloco
    uint32_t inode_count;   // Número total de inodes
    uint32_t free_blocks;   // Blocos livres
    uint32_t free_inodes;       // I-nodes livres
    uint32_t inode_start; 
    uint32_t bitmap_start_block;
    uint32_t free_blocks_bitmap_start; // Bloco onde inicia o bitmap
    uint32_t free_blocks_count;       // Blocos livres totais
} Superblock;

// Escreve o superbloco no disco
void superblock_init(Disk *disk, Superblock *sb);
// Lê o superbloco do disco
Superblock *superblock_load(Disk *disk);

#endif