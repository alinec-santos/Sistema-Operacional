#ifndef BITMAP_H
#define BITMAP_H

#include "disk.h" // Para o tipo Disk
#include "superblock.h"
// Inicializa o bitmap no disco (marca blocos como livres/alocados)
void bitmap_init(Disk *disk, Superblock *sb);

// Marca um bloco como usado (1) ou livre (0)
void bitmap_set(Disk *disk, uint32_t block_num, int used);

// Verifica se um bloco está livre (0) ou usado (1)
int bitmap_get(Disk *disk, uint32_t block_num);

// Encontra e retorna o número do primeiro bloco livre
uint32_t bitmap_find_free_block(Disk *disk);

#endif