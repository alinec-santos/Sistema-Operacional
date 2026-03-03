#ifndef SCRIPT_H
#define SCRIPT_H
#include "dir.h"
#include "disk.h"
void modo_script(const char *filename);
int dir_create_root(Disk *disk);


//extras 
void print_directory_tree(Disk *disk, uint32_t dir_inode, int depth);


#endif