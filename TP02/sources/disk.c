#include "disk.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

Disk *disk_create(const char *filename, uint32_t size, uint32_t block_size) {
    if (size < DISK_SIZE_MIN || size > DISK_SIZE_MAX) return NULL;
    if (block_size % 512 != 0) return NULL; // Alinhado a setores de 512B

    Disk *disk = malloc(sizeof(Disk));
    disk->filename = strdup(filename);
    disk->size = size;
    disk->block_size = block_size;

    // Cria arquivo binário (O_RDWR | O_CREAT, 0644)
    disk->fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (disk->fd == -1) {
        free(disk->filename);
        free(disk);
        return NULL;
    }

    // Ajusta tamanho do arquivo (ftruncate)
    ftruncate(disk->fd, size);
    return disk;
}

void disk_free(Disk *disk) {
    close(disk->fd);
    free(disk->filename);
    free(disk);
}