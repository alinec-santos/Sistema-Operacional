#include "inode.h"
#include "superblock.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Inode *inode_create(Disk *disk, uint32_t mode) {
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