#include "commands.h"

int mv_file(char *src, char *dest)
{
    int sfd = open_file(src, "0");
    MINODE *mip = running->fd[sfd]->mptr;
    // Same Dev link then remove src
    if(mip->dev == fd)
    {
        printf("Same Dev\n");
        link_file(src, dest);
        unlink_file(src);
    }
    // Different dev use cp
    else
    {
        printf("Different Dev\n");
        //close_file(fd);
    }
    

}
