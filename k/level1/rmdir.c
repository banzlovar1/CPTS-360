#include "commands.h"

// Does not handle rmdir of . or .. or /
int rm_dir(char *pathname){
    DIR *dp;
    char buf[BLKSIZE], name[256], temp[256], *cp;
    MINODE *mip, *pmip;
    int i, ino, pino;

    strcpy(temp, pathname);

    ino = getino(pathname);
    if(ino == -1)
    {
        printf("[rmdir]: Access Denied\n");
        return 0;
    }
    mip = iget(dev, ino);

    findmyname(mip, ino, name);
    printf("[rm_dir] : path=%s pino=%d parent name=%s\n", pathname, mip->ino, name);

    //show(mip); not working for some reason?

    printf("[rm_dir]: running->uid=%d ", running->uid);
    printf("[rm_dir]: ino->i_uid=%d\n", mip->inode.i_uid);

    if (running->uid != mip->inode.i_uid || running->uid != 0) return 0; // How to check if running PROC is User
    printf("[rm_dir]: running->uid == ino->i_uid\n");

    if ((mip->inode.i_mode & 0xF000) == 0x4000 && mip->refCount <= 1){ // if is a dir and not being used
        printf("[rm_dir]: mip is a dir\n");
        printf("[rm_dir]: mip link_count=%d\n", mip->inode.i_links_count);
        
        if (mip->inode.i_links_count <= 2){ // could still have reg files
            int actual_links=0;
    
            get_block(dev, mip->inode.i_block[0], buf);
            dp = (DIR*)buf;
            cp = buf;

            while (cp < buf + BLKSIZE){
                actual_links++;

                cp += dp->rec_len;
                dp = (DIR*)cp;
            }

            if (actual_links <= 2){ // good to go
                for (i=0; i<12; i++){
                    if (mip->inode.i_block[i]==0)
                        continue;
                    else
                        bdalloc(mip->dev, mip->inode.i_block[i]); // dealloc mip blocks
                }
                idalloc(mip->dev, mip->ino); // dealloc mip inode
                iput(mip); // put it

                u32 *inum = malloc(8);
                pino = findino(mip, inum);
                pmip = iget(mip->dev, pino);
                findmyname(pmip, ino, name); // find the name of the dir to be deleted
                printf("[rm_dir]: pino=%d ino=%d name=%s\n", pino, ino, name);
                
                if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0 && strcmp(name, "/") != 0){
                    rm_name(pmip, name); // remove name from parent's dir 
                    pmip->inode.i_links_count--; // dec link count
                    pmip->inode.i_atime = pmip->inode.i_mtime = time(0L); // touch a/mtime
                    pmip->dirty = 1; // mark dirty
                    bdalloc(pmip->dev, mip->inode.i_block[0]);
                    idalloc(pmip->dev, mip->ino);
                    iput(pmip);
                }
            } else
                printf("[rm_dir]: mip has children! link_count=%d\n", actual_links);
        } else
            printf("[rm_dir]: mip has children! link_count=%d\n", mip->inode.i_links_count);
    } else if (mip->refCount > 1){
        printf("[rm_dir]: mip=%d is in use, refCount=%d!\n", mip->ino, mip->refCount);
        iput(mip);
    }
    else{
        printf("[rm_dir]: mip is not a dir!\n");
        iput(mip);
    }

    return 0;
}

int rm_name(MINODE *pmip, char *name){
    char buf[BLKSIZE], *cp, *rm_cp, temp[256];
    DIR *dp;
    int block_i, i, j, size, last_len, rm_len;

    for (i=0; i<pmip->inode.i_blocks; i++){
        if (pmip->inode.i_block[i]==0) break;
        get_block(pmip->dev, pmip->inode.i_block[i], buf);
        printf("[rm_name]: get_block i=%d\n", i);
        printf("[rm_name]: NAME=%s\n", name);
        dp = (DIR*)buf;
        cp = buf;

        block_i = i;

        i=0;
        j=0;
        printf("[rm_name]: ");
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
        printf("[rm_name]: block_i=%d\n", block_i);

        if (j==0){ // first entry
            printf("First entry!\n");

            printf("deallocating data block=%d\n", block_i);
            bdalloc(pmip->dev, pmip->inode.i_block[block_i]); // dealloc this block

            for (i=block_i; i<pmip->inode.i_blocks; i++){ // move other blocks up
            }
        } else if (i==0) { // last entry
            cp -= last_len;
            printf("[rm_name]: at end, temp=%s last_len=%d\n", temp, last_len);
            last_len = dp->rec_len;
            printf("[rm_name]: at end, temp=%s last_len=%d\n", temp, last_len);
            dp = (DIR*)cp;
            dp->rec_len += last_len;
            printf("[rm_name]: dp->rec_len=%d\n", dp->rec_len);
        } else { // middle entry
            size = buf+BLKSIZE - (rm_cp+rm_len);
            printf("[rm_name]: in middle, copying n=%d bytes\n", size);
            memmove(rm_cp, rm_cp + rm_len, size);
            cp -= rm_len; 
            dp = (DIR*)cp;

            dp->rec_len += rm_len;

            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            printf("[rm_name]: at the last entry=%s with rec_len=%d\n", temp, dp->rec_len);
        }

        put_block(pmip->dev, pmip->inode.i_block[block_i], buf);
    }


    return 0;
}
