#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "disk.h"
#include "superblock.h"
#include "inode.h"
#include "bitmap.h"
#include "dir.h"
#include "interativo.h"
#include "script.h"

int main() {

    int opcao;
    printf("1- Modo Interativo  2- Modo Script\n");
    printf("Escolha uma opção: ");
    scanf("%i",&opcao);
    if(opcao == 1){
        modointerativo();
    }else if(opcao == 2){
        modo_script("testes/comandos.txt");

    } else {
        printf("opção inválida");
        return 0;
    }
    
    return 0;
}
