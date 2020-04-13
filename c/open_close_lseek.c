#include "commands.h"

int open_file(char *pathname, char *mode){
    OFT *open;
    MINODE *mip;
    int i, ino, i_mode = atoi(mode);

    printf("[open_file]: i_mode=%d\n", i_mode);

    if (pathname[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;

    ino = getino(pathname);
    mip = iget(dev, ino); // get the ino and mip of pathname

    if ((mip->inode.i_mode & 0xF000) != 0x8000) // check if regular file
        return -1;

    // check if file is already open with incompatible mode
    
    open = (OFT*)malloc(sizeof(open)); // build the open fd
    open->mode = i_mode;
    open->refCount = 1;
    open->mptr = mip;

    switch (i_mode){ // offset
        case 0: // Read: offset = 0
            open->offset = 0; 
            break;
        case 1: // Write: truncate file to size=0
            truncate_file(mip);
            open->offset = 0;
            break;
        case 2: // Read/Write: Don't truncate, offset = 0
            open->offset = 0;
            break;
        case 4: // Append: offset to size of file
            open->offset = mip->inode.i_size;
            break;
        default:
            printf("[open_file]: Invalid mode!\n");
            return -1;
    }

    for (i=0; i<NFD; i++) // find empty fd in running PROC's fd array
        if (running->fd[i] == NULL){
            running->fd[i] = open;
            break;
        }
    
    mip->inode.i_atime = time(0L); // update inode access time

    if (i_mode > 0) // if not R then update modify time as well
        mip->inode.i_mtime = time(0L);

    mip->dirty = 1;

    return i; // return fd i
}

int truncate_file(MINODE *mip){
    INODE *ip = &mip->inode;      
    int i;

    for (i=0; i<ip->i_blocks; i++){ // iterate through blocks
        if (ip->i_block[i] != 0)
            bzero(ip->i_block, BLKSIZE); // zero it if it's not empty
        else
            break; // break once we reach empty block (we can assume the following are empty)
    }
    // have to do indirect and double-indirect blocks as well
    
    ip->i_mtime = time(0L); // update modified time
    ip->i_size = 0;
    mip->dirty = 1;

    return 0;
}
