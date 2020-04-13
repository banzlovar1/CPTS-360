#include "commands.h"

int make_dir(char *pathname){
    char *path, *name, cpy[128];
    int pino, r, dev;
    MINODE *pmip;

    strcpy(cpy, pathname); // dirname/basename destroy pathname, must make copy

    if (abs_path(pathname)==0){
        pmip = root;
        dev = root->dev;
    } else {
        pmip = running->cwd;
        dev = running->cwd->dev;
    }

    path = dirname(cpy);
    name = basename(pathname);

    pino = getino(path);
    pmip = iget(dev, pino);

    if ((pmip->inode.i_mode & 0xF000) == 0x4000){ // is_dir
        printf("[mkdir]: pmip is a dir\n");
        if (search(pmip, name)==0){ // if can't find child name in start MINODE
            r = mymkdir(pmip, name);
            pmip->inode.i_links_count++; // increment link count
            pmip->inode.i_atime = time(0L); // touch atime
            pmip->dirty = 1; // make dirty
            iput(pmip); // write to disk

            printf("\n[mkdir]: new directory = %s\n\n", pathname);
            return r;
        } else {
            printf("\n[mkdir]: Dir %s already exists.\n\n", name);
            iput(pmip);
        }
    }

    iput(pmip);

    return 0;
}

int mymkdir(MINODE *pip, char *name){
    char buf[BLKSIZE], *cp;
    DIR *dp;
    MINODE *mip;
    INODE *ip;
    int ino = ialloc(dev), bno = balloc(dev), i;
    printf("[mkdir]: ino=%d bno=%d\n", ino, bno);

    mip = iget(dev, ino);
    ip = &mip->inode;

    char temp[256];
    findmyname(pip, pip->ino, temp);
    printf("[mkdir]: ino=%d name=%s\n", pip->ino, temp);

    ip->i_mode = 0x41ED; // set to dir type and set perms
    ip->i_uid = running->uid; // set owner uid
    ip->i_gid = running->gid; // set group id
    ip->i_size = BLKSIZE; // set byte size
    ip->i_links_count = 2; // . and ..
    ip->i_blocks = 2; // each block = 512 bytes
    ip->i_block[0] = bno; // new dir has only one data block
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); // set to current time

    for (i=1; i <= 14; i++)
        ip->i_block[i] = 0;

    mip->dirty = 1; // make dirty
    iput(mip); // write to disk
    printf("[mymkdir]: wrote mip to disk\n");

    bzero(buf, BLKSIZE);
    cp = buf;

    dp = (DIR*)cp; // write . to buf
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';

    cp += dp->rec_len;
    dp = (DIR *)cp; // move pointer to end of last entry into buf

    dp->inode = pip->ino;
    dp->rec_len = BLKSIZE-12;
    dp->name_len = 2;
    dp->name[0] = dp->name[1] = '.';

    put_block(dev, bno, buf); // write buf to disk at bno
    printf("[mymkdir]: write data block %d to disk\n", bno);
    enter_name(pip, ino, name);

    return 0;
}

int enter_name(MINODE *pip, int myino, char *myname){
    char buf[BLKSIZE], *cp, temp[256];
    DIR *dp;
    int block_i, i, ideal_len, need_len, remain, blk;

    need_len = 4 * ((8 + (strlen(myname)) + 3) / 4);
    printf("[enter_name]: need len for %s is %d\n", myname, need_len);

    for (i=0; i<12; i++){ // find empty block
        if (pip->inode.i_block[i]==0) break;
        get_block(pip->dev, pip->inode.i_block[i], buf); // get that empty block
        printf("[enter_name]: get_block : i=%d\n", i);
        block_i = i;
        dp = (DIR *)buf;
        cp = buf;

        blk = pip->inode.i_block[i];

        printf("[enter_name]: stepping through parent data block[i] = %d\n", blk);
        printf("[enter_name]: ");
        while (cp + dp->rec_len < buf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;

            printf("[%d %s] ", dp->rec_len, temp);
            
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        printf("[%d %s]\n", dp->rec_len, dp->name);

        ideal_len = 4 * ((8 + dp->name_len + 3) / 4);

        printf("[enter_name]: ideal_len=%d\n", ideal_len);
        remain = dp->rec_len - ideal_len;
        printf("[enter_name]: remain=%d\n", remain);

        if (remain >= need_len){
            dp->rec_len = ideal_len; // trim last rec_len to ideal_len

            cp += dp->rec_len;
            dp = (DIR*)cp;
            dp->inode = myino;
            dp->rec_len = remain;
            dp->name_len = strlen(myname);
            strcpy(dp->name, myname);
        }
    }

    printf("[enter_name]: put_block : i=%d\n", block_i);
    put_block(pip->dev, pip->inode.i_block[block_i], buf);
    printf("[enter_name]: write parent data block=%d to disk\n", blk);

    return 0;
}

int creat_file(char *pathname){
    char *path, *name, cpy[128];
    int pino, r, dev;
    MINODE *pmip;

    strcpy(cpy, pathname); // dirname/basename destroy pathname, must make copy

    if (abs_path(pathname)==0){
        pmip = root;
        dev = root->dev;
    } else {
        pmip = running->cwd;
        dev = running->cwd->dev;
    }

    path = dirname(cpy);
    name = basename(pathname);

    printf("[touch]: path=%s\n", path);
    printf("[touch]: file=%s\n", name);

    pino = getino(path);
    pmip = iget(dev, pino);

    if ((pmip->inode.i_mode & 0xF000) == 0x4000){ // is_dir
        if (search(pmip, name)==0){ // if can't find child name in start MINODE
            r = mycreat(pmip, name);
            pmip->inode.i_atime = time(0L); // touch atime
            pmip->dirty = 1; // make dirty
            iput(pmip); // write to disk

            printf("\n[touch]: Successfully created new file = %s\n\n", pathname);
            return r;
        } else {
            printf("\n[touch]: That file name=%s already exists.\n\n", name);
            iput(pmip);
        }
    }

    return 0;
}

int mycreat(MINODE *pip, char *name){
    MINODE *mip;
    INODE *ip;
    int ino = ialloc(dev);

    mip = iget(dev, ino);
    ip = &mip->inode;

    ip->i_mode = 0x81A4; // set to file type and set perms
    ip->i_uid = running->uid; // set owner uid
    ip->i_gid = running->gid; // set group id
    ip->i_size = 0; // no data blocks
    ip->i_links_count = 1; // . and ..
    //ip->i_blocks = 2; // each block = 512 bytes
    //ip->i_block[0] = bno; // new dir has only one data block
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); // set to current time

    mip->dirty = 1;
    printf("[mycreat]: write INODE to disk\n");

    enter_name(pip, ino, name);

    iput(mip);

    return 0;
}
