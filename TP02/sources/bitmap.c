#include "bitmap.h"
#include "superblock.h"
#include <unistd.h>
#include <stdlib.h>

#define BITMAP_START_BLOCK 1

void bitmap_init(Disk *disk, Superblock *sb) {
    // Calcula o tamanho necessário para o bitmap
    uint32_t bitmap_size = disk->size / disk->block_size / 8;
    uint8_t *bitmap = calloc(bitmap_size, 1);
    
    // Marca blocos obrigatórios como USADOS (1):
    bitmap_set(disk, 0, 1);  // Superbloco
    bitmap_set(disk, 1, 1);  // Bitmap itself
    
    // Escreve o bitmap no disco
    lseek(disk->fd, sb->bitmap_start_block * disk->block_size, SEEK_SET);
    write(disk->fd, bitmap, bitmap_size);
    free(bitmap);
}

void bitmap_set(Disk *disk, uint32_t block_num, int used) {
    uint32_t byte_pos = block_num / 8;
    uint8_t bit_mask = 1 << (block_num % 8);
    uint8_t byte;

    // Lê o byte atual do bitmap no disco
    lseek(disk->fd, disk->block_size + byte_pos, SEEK_SET); // Corrigido: Começa no bloco 1 (bitmap)
    read(disk->fd, &byte, 1);

    // Modifica o bit correspondente
    if (used) {
        byte |= bit_mask;
    } else {
        byte &= ~bit_mask;
    }

    // Escreve o byte de volta no disco
    lseek(disk->fd, disk->block_size + byte_pos, SEEK_SET);
    write(disk->fd, &byte, 1);
}

int bitmap_get(Disk *disk, uint32_t block_num) {
    uint32_t byte_pos = block_num / 8;
    uint8_t bit_mask = 1 << (block_num % 8);
    uint8_t byte;

    // CORREÇÃO: Precisa ler do bloco 1 (bitmap), não da posição 0
    lseek(disk->fd, disk->block_size + byte_pos, SEEK_SET);
    read(disk->fd, &byte, 1);
    return (byte & bit_mask) ? 1 : 0;
}

uint32_t bitmap_find_free_block(Disk *disk) {
    uint32_t total_blocks = disk->size / disk->block_size;
    for (uint32_t i = 0; i < total_blocks; i++) {
        if (!bitmap_get(disk, i)) return i;
    }
    return (uint32_t)-1; // Retorna valor inválido se nenhum bloco livre for encontrado
}