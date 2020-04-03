/*********** util.c file ****************/
#include "util.h"

extern PROC *running;
extern MINODE *root, minode[NMINODE];
extern char gpath[128];
extern char *name[32];
extern int n, inode_start, dev, imap, bmap, ninodes, nblocks;
int r;

int get_block(int dev, int blk, char *buf){
    lseek(dev, (long)blk*BLKSIZE, 0);
    r = read(dev, buf, BLKSIZE);

    return r;
}   

int put_block(int dev, int blk, char *buf){
    lseek(dev, (long)blk*BLKSIZE, 0);
    r = write(dev, buf, BLKSIZE);

    return r;
}   

int tokenize(char *pathname){
    int i;
    char *s;
    printf("tokenize %s\n", pathname);

    strcpy(gpath, pathname);   // tokens are in global gpath[ ]
    n = 0;

    s = strtok(gpath, "/");
    while(s){
        name[n] = s;
        n++;
        s = strtok(0, "/");
    }

    for (i= 0; i<n; i++)
        printf("%s  ", name[i]);
    printf("\n");

    return 0;
}
    
// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino){
    int i;
    MINODE *mip;
    char buf[BLKSIZE];
    int blk, offset;
    INODE *ip;

    for (i=0; i<NMINODE; i++){
        mip = &minode[i];
        if (mip->dev == dev && mip->ino == ino){
            mip->refCount++;
            //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
            return mip;
        }
    }
    
    for (i=0; i<NMINODE; i++){
        mip = &minode[i];
        if (mip->refCount == 0){
            //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
            mip->refCount = 1;
            mip->dev = dev;
            mip->ino = ino;

            // get INODE of ino into buf[ ]    
            blk    = (ino-1)/8 + inode_start;
            offset = (ino-1) % 8;

            //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

            get_block(dev, blk, buf);
            ip = (INODE *)buf + offset;
            // copy INODE to mp->INODE
            mip->inode = *ip;
            return mip;
        }
    }   
    printf("PANIC: no more free minodes\n");
    return 0;
}
    
void iput(MINODE *mip){
    int block, offset;
    char buf[BLKSIZE];
    INODE *ip;

    if (mip==0) return;

    mip->refCount--;
    if (mip->refCount > 0)  // minode is still in use
        return;
    if (!mip->dirty)        // INODE has not changed; no need to write back
        return;

    /* write INODE back to disk */
    /***** NOTE *******************************************
        For mountroot, we never MODIFY any loaded INODE
        so no need to write it back
        FOR LATER WROK: MUST write INODE back to disk if refCount==0 && DIRTY
        Write YOUR code here to write INODE back to disk
    ********************************************************/

    printf(" iput : inode_start=%d\n", inode_start);

    block = (mip->ino - 1) / 8 + inode_start;
    offset = (mip->ino -1) % 8;

    printf(" iput : block=%d, offset=%d\n", block, offset);

    get_block(mip->dev, block, buf);
    ip = (INODE *)buf + offset;
    *ip = mip->inode;
    put_block(mip->dev, block, buf);
    mip->refCount = 0;
} 

int search(MINODE *mip, char *name){
    char *cp, sbuf[BLKSIZE], temp[256];
    DIR *dp;
    INODE *ip;

    printf("search for %s in MINODE = [%d, %d]\n", name, mip->dev, mip->ino);
    ip = &(mip->inode);

    /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

    get_block(dev, ip->i_block[0], sbuf);
    dp = (DIR *)sbuf;
    cp = sbuf;
    printf("  ino   rlen  nlen  name\n");

    while (cp < sbuf + BLKSIZE){
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;
        printf("%4d  %4d  %4d    %s\n", 
            dp->inode, dp->rec_len, dp->name_len, temp);
        if (strcmp(temp, name)==0){
            printf("found %s : ino = %d\n", temp, dp->inode);
            return dp->inode;
        }
        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
    return 0;
}
    
int getino(char *pathname){
    int i, ino;// blk, disp;
    //char buf[BLKSIZE];
    //INODE *ip;
    MINODE *mip;

    printf("getino: pathname=%s\n", pathname);
    if (strcmp(pathname, "/")==0)
        return 2;

    // starting mip = root OR CWD
    if (pathname[0]=='/')
        mip = root;
    else
        mip = running->cwd;

    mip->refCount++;         // because we iput(mip) later

    tokenize(pathname);
    for (i=0; i<n; i++){
        printf("===========================================\n");
        printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
 
        ino = search(mip, name[i]);

        if (ino==0){
            iput(mip);
            printf("name %s does not exist\n", name[i]);
            return 0;
        }
        iput(mip);                // release current mip
        mip = iget(dev, ino);     // get next mip
    }

    iput(mip);                   // release mip  
    return ino;
}
    
int findmyname(MINODE *parent, u32 myino, char *myname){
    DIR *dp;
    char buf[BLKSIZE], temp[256], *cp;

    get_block(dev, parent->inode.i_block[0], buf);
    dp = (DIR *)buf;
    cp = buf;

    while (cp < buf + BLKSIZE){
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;

        if (dp->inode == myino)
            strcpy(myname, temp);

        cp += dp->rec_len;
        dp = (DIR *)cp;
    }

    return 0;
}

int show(MINODE *mip){
    DIR *dp;
    char buf[BLKSIZE], name[256], *cp;
    INODE *ip = &(mip->inode);

    get_block(mip->dev, ip->i_block[0], buf);
    cp = buf;
    dp = (DIR*)cp;

    printf("\ninode\trec_len\tname_len\tname\n========================================\n");
    while (cp < buf + BLKSIZE){
        strncpy(name, dp->name, dp->name_len);
        name[dp->name_len]=0;

        printf("%1d\t%1d\t%1d\t%s\n", dp->inode, dp->rec_len, dp->name_len, name);

        cp += dp->rec_len;
        dp = (DIR*)cp;
    }
    putchar('\n');

    return 0;
}

int findino(MINODE *mip, u32 *myino){ // myino = ino of . return ino of ..
    char buf[BLKSIZE], *cp;   
    DIR *dp;

    get_block(mip->dev, mip->inode.i_block[0], buf);
    cp = buf; 
    dp = (DIR *)buf;
    *myino = dp->inode;
    cp += dp->rec_len;
    dp = (DIR *)cp;
    return dp->inode;
}

int enter_name(MINODE *pip, int myino, char *myname){
    char buf[BLKSIZE], *cp, temp[256];
    DIR *dp;
    int block_i, i, ideal_len, need_len, remain, blk;

    printf(" enter_name : search(pip)\n");
    search(pip, "b");

    printf(" enter_name : search(pip)2\n");
    search(pip, "b");

    need_len = 4 * ((8 + (strlen(myname)) + 3) / 4);
    printf("need len for %s is %d\n", myname, need_len);

    for (i=0; i<12; i++){ // find empty block
        if (pip->inode.i_block[i]==0) break;
        get_block(pip->dev, pip->inode.i_block[i], buf); // get that empty block
        printf("get_block : i=%d\n", i);
        block_i = i;
        dp = (DIR *)buf;
        cp = buf;

        blk = pip->inode.i_block[i];

        printf(" stepping through parent data block[i] = %d\n", blk);
        while (cp + dp->rec_len < buf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;

            printf("[%d %s] ", dp->rec_len, temp);
            
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        printf("[%d %s]\n", dp->rec_len, dp->name);

        ideal_len = 4 * ((8 + dp->name_len + 3) / 4);

        printf("ideal_len=%d\n", ideal_len);
        remain = dp->rec_len - ideal_len;
        printf("remain=%d\n", remain);

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

    printf("put_block : i=%d\n", block_i);
    put_block(pip->dev, pip->inode.i_block[block_i], buf);
    printf("write parent data block=%d to disk\n", blk);

    return 0;
}

int rm_name(MINODE *pmip, char *name){
    char buf[BLKSIZE], *cp, *rm_cp, temp[256];
    DIR *dp;
    int block_i, i, j, size, last_len, rm_len;

    for (i=0; i<pmip->inode.i_blocks; i++){
        if (pmip->inode.i_block[i]==0) break;
        get_block(pmip->dev, pmip->inode.i_block[i], buf);
        printf("rm_name : get_block i=%d\n", i);
        printf("NAME=%s\n", name);
        dp = (DIR*)buf;
        cp = buf;

        block_i = i;

        i=0;
        j=0;
        while (cp + dp->rec_len < buf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;

            if (!strcmp(name, temp)){
                i=j;
                rm_cp = cp;
                rm_len = dp->rec_len;
                printf("rm=[%d %s] ", dp->rec_len, temp);
            } else
                printf("[%d %s] ", dp->rec_len, temp);


            last_len = dp->rec_len;
            cp += dp->rec_len;
            dp = (DIR*)cp;
            j++; // get count of entries into j 
        }
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;

        printf("[%d %s]\n", dp->rec_len, temp);
        printf("block_i=%d\n", block_i);

        if (j==0){ // first entry
            printf("First entry!\n");

            printf("deallocating data block=%d\n", block_i);
            bdalloc(pmip->dev, pmip->inode.i_block[block_i]); // dealloc this block

            for (i=block_i; i<pmip->inode.i_blocks; i++){ // move other blocks up
            }
        } else if (i==0) { // last entry
            cp -= last_len;
            printf("at end : temp=%s last_len=%d\n", temp, last_len);
            last_len = dp->rec_len;
            printf("at end : temp=%s last_len=%d\n", temp, last_len);
            dp = (DIR*)cp;
            dp->rec_len += last_len;
            printf("dp->rec_len=%d\n", dp->rec_len);

            //last_len = dp->rec_len; // length of last rec_len
            //printf("last r_len=%d taking rec_len of %s=%d ", last_r_len, temp, last_len);
            //cp -= last_r_len;
            //dp = (DIR*)cp;

            //strncpy(temp, dp->name, dp->name_len);
            //temp[dp->name_len] = 0;

            //printf("and putting it to %s who's prev rec len was %d", temp, dp->rec_len);
            //dp->rec_len += last_len;
            //printf(", but now it's rec_len=%d\n", dp->rec_len);
        } else { // middle entry
            size = buf+BLKSIZE - (rm_cp+rm_len);
            printf("in middle : copying n=%d bytes\n", size);
            memmove(rm_cp, rm_cp + rm_len, size);
            cp -= rm_len; 
            dp = (DIR*)cp;

            dp->rec_len += rm_len;

            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            printf("at the last entry=%s with rec_len=%d\n", temp, dp->rec_len);
        }

        put_block(pmip->dev, pmip->inode.i_block[block_i], buf);
    }


    return 0;
}

int abs_path(char *path){
    if (path[0] == '/')
        return 0;
    else
        return -1;
}

int tst_bit(char *buf, int bit){
    int i = bit / 8, j = bit % 8;

    if (buf[i] & (1 << j))
        return 1;

    return 0;
}

int set_bit(char *buf, int bit){
    int i = bit / 8, j = bit % 8;

    buf[i] |= (1 << j);

    return 0;
}

int clr_bit(char *buf, int bit){
    int i = bit / 8, j = bit % 8;

    buf[i] &= ~(1 << j);

    return 0;
}

int dec_free_inodes(int dev){
    char buf[BLKSIZE];

    get_block(dev, 1, buf); // dec the super table
    sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf; // dec the GD table
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);

    return 0;
}

int dec_free_blocks(int dev){
    char buf[BLKSIZE];

    get_block(dev, 1, buf); // dec the super table
    sp = (SUPER *)buf;
    sp->s_free_blocks_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf; // dec the GD table
    gp->bg_free_blocks_count--;
    put_block(dev, 2, buf);

    return 0;
}

int inc_free_inodes(int dev){
    char buf[BLKSIZE];

    get_block(dev, 1, buf); // get the super table
    sp = (SUPER*)buf;
    sp->s_free_inodes_count++; // inc free inodes
    put_block(dev, 1, buf); // put it back

    get_block(dev, 2, buf); // get the gd table
    gp = (GD*)buf;
    gp->bg_free_inodes_count++; // inc free inodes
    put_block(dev, 2, buf); // put it back

    return 0;
}

int inc_free_blocks(int dev){
    char buf[BLKSIZE];

    get_block(dev, 1, buf); 
    sp = (SUPER *)buf;
    sp->s_free_blocks_count++; // dec the super table
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf; // dec the GD table
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);

    return 0;
}

int ialloc(int dev){
    int i;
    char buf[BLKSIZE];

    get_block(dev, imap, buf);

    for (i=0; i < ninodes; i++){
        if (tst_bit(buf, i) == 0){
            set_bit(buf, i);
            put_block(dev, imap, buf);
            dec_free_inodes(dev);
            printf("allocated ino = %d\n", i+1); // bits 0..n, ino 1..n+1
            return i+1;
        }
    }
    return 0;
}

int idalloc(int dev, int ino){
    char buf[BLKSIZE];

    if (ino > ninodes){
        printf("inumber=%d out of range.\n", ino);
        return 0;
    }

    get_block(dev, imap, buf);
    clr_bit(buf, ino-1);
    put_block(dev, imap, buf);

    inc_free_inodes(dev);
    printf("deallocated ino=%d\n", ino);

    return 0;
}

int balloc(int dev){
    int i;
    char buf[BLKSIZE];

    get_block(dev, bmap, buf);

    for (i=0; i < nblocks; i++){
        if (tst_bit(buf, i) == 0){
            set_bit(buf, i);
            put_block(dev, bmap, buf);
            printf("allocated block = %d\n", i+1); // bits 0..n, ino 1..n+1
            dec_free_blocks(dev);
            return i+1;
        }
    }
    return 0;
}

// potential issue here, deallocating to wrong blk?
int bdalloc(int dev, int blk){
    char buf[BLKSIZE];

    if (blk > nblocks){
        printf("block=%d out of range.\n", blk);
        return 0;
    }

    get_block(dev, bmap, buf);
    clr_bit(buf, blk-1);
    put_block(dev, bmap, buf);

    inc_free_blocks(dev);
    printf("deallocated block=%d\n", blk-1);

    return 0;
}
