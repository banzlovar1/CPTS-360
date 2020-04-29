#include "commands.h"

extern int dev, imap, bmap, ninodes, nblocks;

int unlink_file(char *filename)
{
    char cpy[128];
    strcpy(cpy, filename);
    int ino = getino(filename);
    if(ino <= 0)
    {
        printf("[unlink]: File does not exist\n");
        return -1;
    }
    MINODE *mip = iget(dev,ino);
    // Check to see if ownership stands or is super user
    if(running->uid != mip->inode.i_uid || running->uid != 0)
    {
        printf("[unlink]: Permission Denied\n");
        return -1;
    }
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
