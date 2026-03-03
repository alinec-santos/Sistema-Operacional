#ifndef INODE_H
#define INODE_H

#include <time.h>
#include "disk.h"
#include "bitmap.h"
#define INODE_SIZE 128          // Tamanho fixo de cada i-node
#define DIRECT_BLOCKS 12        // Ponteiros diretos
#define INDIRECT_BLOCKS 1       // Ponteiro indireto simples

typedef struct {
    uint32_t mode;              // Tipo (arquivo/diretório) e permissões
    uint32_t uid;               // Dono
    uint32_t size;              // Tamanho em bytes
    time_t created_at;          // Timestamp de criação
    time_t modified_at;         // Timestamp de modificação
    uint32_t blocks[DIRECT_BLOCKS]; // Blocos diretos
    uint32_t indirect_block;    // Bloco indireto
} Inode;

// Cria um novo i-node vazio
Inode *inode_create(uint32_t mode);
// Salva i-node no disco
void inode_save(Disk *disk, uint32_t inode_num, Inode *inode);
// Carrega i-node do disco
Inode *inode_load(Disk *disk, uint32_t inode_num);

uint32_t inode_alloc();

void inode_reset_counter();

void inode_free(Disk *disk, uint32_t inode_num);
#endif