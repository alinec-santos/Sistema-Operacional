#ifndef INTERATIVO_H
#define INTERATIVO_H

#include "disk.h" // Para o tipo Disk
#include "superblock.h"
#include "dir.h"
#include "inode.h"
#include "bitmap.h"

void print_header(const char *title);
void print_menu();
int modointerativo();
#endif