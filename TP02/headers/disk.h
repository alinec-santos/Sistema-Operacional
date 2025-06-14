#ifndef DISK_H
#define DISK_H

#include <stdint.h>

#define DISK_SIZE_MIN (1 * 1024 * 1024)   // 1MB mínimo
#define DISK_SIZE_MAX (100 * 1024 * 1024) // 100MB máximo
#define BLOCK_SIZE_DEFAULT 4096           // 4KB por bloco

typedef struct {
    char *filename;      // Nome do arquivo que simula o disco
    int fd;             // Descritor do arquivo (Unix)
    uint32_t size;      // Tamanho total do disco (bytes)
    uint32_t block_size;// Tamanho do bloco (bytes)
} Disk;

// Cria/abre um disco virtual
Disk *disk_create(const char *filename, uint32_t size, uint32_t block_size);
// Libera o disco da memória
void disk_free(Disk *disk);

#endif