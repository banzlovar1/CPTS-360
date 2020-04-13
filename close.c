#include "commands.h"

int close_file(char g[50])
{
    int fd = atoi(g);
    // See if the file descriptor exists
    if(fd < 0 || fd >=NFD)
    {
        printf("File out of range\n");
        return -1;
    }
    if(running->fd[fd] == NULL)
    {
        printf("File does not exist\n");
        return -1;
    }

    OFT *oftp;
    oftp = running->fd[fd];
    running->fd[fd] = 0;
    oftp->refCount--;
    if(oftp->refCount > 0) 
        return 0;
    MINODE *mip = oftp->mptr;
    iput(mip);
    return 0;
}