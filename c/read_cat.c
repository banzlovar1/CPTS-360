#include "commands.h"

int read_file(char *fd, char *bytes){
    int i_fd = atoi(fd), i_bytes = atoi(bytes);
    char buf[BLKSIZE];
    printf("[read_file]: Reading fd=%d\n", i_fd);

    return myread(i_fd, buf, i_bytes, 0);
}

int min3(int a, int b, int c){
    if (a <= b && a <= c)
        return a;
    else if (b <= c && b <= a)
        return b;
    else
        return c;
}

int myread(int fd, char *buf, int nbytes, int supress_msg){
    char readbuf[BLKSIZE];
    int count=0, blk, lblk, start, remain, avail;
    char *cq = buf;

    if (running->fd[fd] == NULL){ // make sure fd exists
        printf("[myread]: fd is NULL!");
        return -1;
    }
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->mptr;

    avail = mip->inode.i_size - oftp->offset;

    if (!supress_msg)
        printf("[myread]: fd=%d offset=%d bytes=%d\n", fd, oftp->offset, nbytes);

    while (nbytes && avail){ // read loop
        lblk = oftp->offset / BLKSIZE;
        start = oftp->offset % BLKSIZE;

        if (lblk < 12)
            blk = mip->inode.i_block[0];
        else if (lblk >= 12 && lblk < 256 + 12){}
            // indirect blocks
        else{}
            // double indirect blocks

        get_block(mip->dev, blk, readbuf);
        char *cp = readbuf + start; // takes care of offset
        remain = BLKSIZE - start;

        // read optimization
        int min = min3(nbytes, avail, remain);
        if (!supress_msg)
            printf("[myread]: min=%d\n", min);
        strncpy(buf, cp, min);

        oftp->offset += min;
        count += min;
        avail -= min;
        nbytes -= min;
        remain -= min;

        //while (remain > 0){
        //    *cq++ = *cp++; // cpy byte into buf
        //    oftp->offset++;
        //    count++; // inc offset and count
        //    avail--; nbytes--; remain--; // dec avail, nbytes and remain
        //    if (nbytes <= 0 || avail <= 0)
        //        break;
        //}
    }

    if (!supress_msg)
        printf("[myread]: nbytes=%d text=%s\n", count, buf);

    return count;
}

int cat_file(char *filename){
    char mybuf[BLKSIZE], temp=0;
    int n, i, fd = open_file(filename, "0");

    printf("[cat_file]:\n\n");
    while ((n = myread(fd, mybuf, BLKSIZE, 1))){
        mybuf[n]=0;

        for (i=0; i<n; i++){
            if (mybuf[i] == '\\'){
                i++;
                if (mybuf[i] == 'n'){
                    putchar('\n');
                    continue;
                }
            }

            putchar(mybuf[i]);
        }
    }
    printf("\n\n");

    close_file(fd);

    return 0;
}
