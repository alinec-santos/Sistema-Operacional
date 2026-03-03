#ifndef DIR_H
#define DIR_H

#include <stdint.h>  // Para uint32_t
#include "disk.h"    // Para Disk

#define MAX_NAME_LEN 28   // 28 caracteres + terminador \0
#define DIR_ENTRY_SIZE 32 // Tamanho fixo (4 bytes inode + 28 bytes nome)
#define MAX_BLOCKS_PER_INODE 10

// Estrutura que representa uma entrada de diretório
typedef struct {
    uint32_t inode_num;   // Número do i-node associado
    char name[MAX_NAME_LEN]; // Nome do arquivo/diretório
} DirEntry;

// Cria um novo diretório
int dir_create(Disk *disk, uint32_t parent_inode_num, const char *name);

// Lista conteúdo de um diretório
int dir_list(Disk *disk, uint32_t inode_num);

int dir_add_entry(Disk *disk, uint32_t dir_inode_num, uint32_t child_inode_num, const char *name);

int file_create(Disk *disk, uint32_t parent_inode_num, const char *host_filename, const char *fs_filename);

int file_read(Disk *disk, uint32_t inode_num);

int dir_list_detailed(Disk *disk, uint32_t inode_num) ;

int dir_list_dirs(Disk *disk, uint32_t inode_num);

uint32_t navegar_diretorios(Disk *disk, uint32_t starting_inode);

int dir_rename_entry(Disk *disk, uint32_t parent_inode_num, uint32_t child_inode_num, const char *novo_nome);

int dir_delete(Disk *disk, uint32_t dir_inode_num);

uint32_t dir_find_parent(Disk *disk, uint32_t target_inode_num) ;

uint32_t dir_find_parent_recursive(Disk *disk, uint32_t current_inode_num, uint32_t target_inode_num);

int dir_remove_entry(Disk *disk, uint32_t dir_inode_num, uint32_t target_inode_num);

int file_delete(Disk *disk, uint32_t parent_inode_num, uint32_t file_inode_num) ;





#endif