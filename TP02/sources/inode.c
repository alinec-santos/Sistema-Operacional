#include "inode.h"
#include "superblock.h"
#include "disk.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
static uint32_t next_inode_num = 1;


extern Disk *disk;

Inode *inode_create(uint32_t mode) {
    Inode *inode = malloc(sizeof(Inode));
    memset(inode, 0, sizeof(Inode));
    
    inode->mode = mode;
    inode->created_at = time(NULL);
    inode->modified_at = inode->created_at;
    return inode;
}

void inode_save(Disk *disk, uint32_t inode_num, Inode *inode) {
    uint32_t offset = sizeof(Superblock) + (inode_num * INODE_SIZE);
    lseek(disk->fd, offset, SEEK_SET);
    write(disk->fd, inode, sizeof(Inode));
}

Inode *inode_load(Disk *disk, uint32_t inode_num) {
    Inode *inode = malloc(sizeof(Inode));
    uint32_t offset = sizeof(Superblock) + (inode_num * INODE_SIZE);
    
    lseek(disk->fd, offset, SEEK_SET);
    read(disk->fd, inode, sizeof(Inode));
    return inode;
}

uint32_t inode_alloc() {
    return next_inode_num++;
}

void inode_reset_counter() {
    next_inode_num = 0;
}

void inode_free(Disk *disk, uint32_t inode_num) {
    bitmap_set(disk, inode_num, 0);
    printf("[INFO] inode %u liberado\n", inode_num);
}