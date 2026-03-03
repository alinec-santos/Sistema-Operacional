#include "superblock.h"
#include "bitmap.h"
#include <unistd.h>
#include <stdlib.h>  
#define FS_MAGIC 0x46535F53 // "FS_S"

void superblock_init(Disk *disk, Superblock *sb) {
    sb->magic = FS_MAGIC;
    sb->disk_size = disk->size;
    sb->block_size = disk->block_size;
    sb->bitmap_start_block = 1; 
    bitmap_init(disk,sb); 
    lseek(disk->fd, 0, SEEK_SET);
    write(disk->fd, sb, sizeof(Superblock)); // Escreve no início do disco
}

Superblock *superblock_load(Disk *disk) {
    Superblock *sb = malloc(sizeof(Superblock));
    lseek(disk->fd, 0, SEEK_SET);
    read(disk->fd, sb, sizeof(Superblock));
    return (sb->magic == FS_MAGIC) ? sb : NULL;
}