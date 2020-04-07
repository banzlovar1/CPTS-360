#include "commands.h"

int unlink_file(char *filename)
{
    char cpy[128];
    strcpy(cpy, filename);
    int ino = getino(filename);
    if(ino < 1)
    {
        printf("File does not exist\n");
        return 0;
    }
    MINODE *mip = iget(dev,ino);
    if(S_ISREG(mip->inode.i_mode) || S_ISLNK(mip->inode.i_mode))
    {
        char *parent = dirname(filename);
        char *child = basename(filename);
        int pino = getino(parent);
        MINODE *pmip = iget(dev, pino);
        rm_name(pmip, child);
        pmip->dirty = 1;
        iput(pmip);
        mip->inode.i_links_count--;
        if(mip->inode.i_links_count>0)
            mip->dirty=1;
        else{
            for(int i = 0; i < 12 && mip->inode.i_block[i] !=0; i++)
                bdalloc(dev, mip->inode.i_block[i]);
            idalloc(dev, ino);
        }
        iput(mip);
    }
    return 0;
}
