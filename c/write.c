#include "commands.h"

int write_file()
{
    char string[BLKSIZE];
    int fd;
    pfd();
    printf("fd [string]: ");
    scanf("%d %s", &fd, string);
    if(fd < 0 || fd >=NFD)
        return -1;
    if(running->fd[fd] == NULL)
        return -1;
    if(running->fd[fd]->mode == 1 || running->fd[fd]->mode == 2)
    {
        getchar();
        return(mywrite(fd, string, strlen(string)));
    }
    printf("Can write to a file designated for read\n");
    return -1;
}

int mywrite(int fd, char *buf, int nbytes)
{
    printf("In mywrite: %s %d\n", buf, nbytes);
    int count = nbytes, blk, dblk, *u;
    int ibuf[256], dbuf[256];
    char wbuf[BLKSIZE];
    OFT *oftp = running->fd[fd];
    MINODE *mip = running->fd[fd]->mptr;
    while(nbytes)
    {
        int lbk = oftp->offset / BLKSIZE;
        int startByte = oftp->offset % BLKSIZE;
        // Direct Block
        if(lbk <12)
        {
            printf("Direct Block\n");
            if(mip->inode.i_block[lbk] == 0)
                mip->inode.i_block[lbk] = balloc(mip->dev);
            blk  = mip->inode.i_block[lbk];
        }
        // Indirect Block
        else if(lbk >= 12 && lbk < 256 + 12)
        {
            printf("Indirect Block\n");
            if(mip->inode.i_block[12] == 0)
            {
                mip->inode.i_block[12] = balloc(mip->dev);
                for(int i = 0; i < 256; i++)
                    ibuf[i] = 0;
            }
            get_block(mip->dev, mip->inode.i_block[12], (char *)ibuf);
            int blk = ibuf[lbk -12];
            if(blk ==0)
            {
               mip->inode.i_block[lbk] = balloc(mip->dev);
               ibuf[lbk -12] = mip->inode.i_block[lbk];
            }
        }
        // Double indirect blocks
        else
        {
            printf("Double Indirect Block\n");
            get_block(mip->dev,mip->inode.i_block[13], (char *)dbuf);
            lbk -= (12+256);
            dblk = dbuf[lbk/256];
            get_block(mip->dev, mip->inode.i_block[13], (char *)dbuf);
            blk = dbuf[lbk % 256];
        }
        get_block(mip->dev, blk,wbuf);
        char *cp;
        cp = wbuf + startByte;
        int remain = BLKSIZE -startByte;
        char *cq;
        cq = (char *)buf;
        while(remain > 0)
        {
            *cp++ = *cq--;
            nbytes--; remain--;
            oftp->offset++;
            if(oftp->offset > mip->inode.i_size)
                mip->inode.i_size++;
            if(nbytes <= 0)
                break;
        }
        put_block(mip->dev,blk,wbuf);
    }
    mip->dirty =1;
    printf("Wrote %d char into file descriptor fd=%d\n", count,fd);
    return nbytes;
}