#include "commands.h"

int mount(){
    printf("[mount]: stuff\n");



    return 0;
}

int umount(char *filesys){
    printf("[umount]: filesys=%s\n", filesys);

    return 0;
}
