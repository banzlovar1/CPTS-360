#include "commands.h"

int sym_link(char *src, char *dest){
    int sino, dino;
    char *name, temp[128], buf[BLKSIZE];
    MINODE *mip; //*pmip;

    strcpy(temp, src);
    //path = dirname(temp);
    name = basename(src);
    name[strlen(name)] = 0;

    sino = getino(src);
    dino = getino(dest);
    if (sino > 0 && dino == 0){
        printf("[sym_link]: src exists and dest does not!\n");
        printf("[sym_link]: creating new file=%s\n", dest);
        creat_file(dest);
        dino = getino(dest);
        printf("[sym_link]: dest_ino=%d\n", dino);

        mip = iget(dev, dino);
        mip->inode.i_mode = 0127777; // set new (link) file type to link
        get_block(mip->dev, mip->inode.i_block[0], buf); // put name in link file block area
        strcpy(buf, name);
        put_block(mip->dev, mip->inode.i_block[0], buf);
        mip->inode.i_size = strlen(name); // modify size to = old file name
        mip->dirty = 1;
        iput(mip);
    } else if (sino <= 0)
        printf("[sym_link]: src already exists!");
    else if (dino != 0)
        printf("[sym_link]: dest already exists!");

    return 0;
}
