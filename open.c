#include "commands.h"

int open_file(char *pathname, char *mode){
    MINODE *mip;
    int ino, i_mode = atoi(mode);

    printf("[open_file]: i_mode=%d\n", i_mode);

    if (pathname[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;

    ino = getino(pathname);
    mip = iget(dev, ino); // get the ino and mip of pathname

    if ((mip->inode.i_mode & 0xF000) == 0x8000){
    }

    return 0;
}

int truncate_file(MINODE *mip){
    return 0;
}
