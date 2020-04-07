<<<<<<< HEAD
/************* cd_ls_pwd.c file **************/
#include "commands.h"

extern PROC *running;
extern MINODE *root;
extern int dev, imap, bmap, ninodes, nblocks;
char t1[9] = "xwrxwrxwr", t2[9] = "---------";

/******************** Assignment 5 ********************/

int cd(char *pathname)   
{
    MINODE *mip;
    int ino;

    if (strlen(pathname) > 0)
        ino = getino(pathname);
    else
        ino = root->ino;

    mip = iget(dev, ino);

    if ((mip->inode.i_mode & 0xF000) == 0x4000){ // if is a dir
        iput(running->cwd);
        running->cwd = mip;
    }

    printf("\n[cd]: cwd = ");
    pwd(running->cwd);

    return 0;
}

int ls_file(MINODE *mip, char *name)
{
    int i;
    time_t time;
    char temp[64], l_name[128], buf[BLKSIZE];

    INODE *ip = &mip->inode;
    putchar('\n');
    
    if ((ip->i_mode & 0xF000) == 0x8000) // is_reg
        printf(" -");
    else if ((ip->i_mode & 0xF000) == 0x4000) // is_dir
        printf(" d");
    else if ((ip->i_mode & 0xF000) == 0xA000){ // is_link
        printf(" l");
        // get link file name
        get_block(mip->dev, mip->inode.i_block[0], buf);
        strcpy(l_name, buf);
        put_block(mip->dev, mip->inode.i_block[0], buf);
        l_name[strlen(l_name)] = 0;
    }

    for (i=8; i >= 0; i--) // permissions
        if (ip->i_mode & (1 << i))
            putchar(t1[i]);
        else
            putchar(t2[i]);
    putchar(' ');

    printf("%d ", ip->i_links_count); // link count
    printf("%d ", ip->i_uid); // owner
    printf("%d ", ip->i_gid); // group
    printf("%5d ", ip->i_size); // byte size
    time = (time_t)ip->i_ctime;
    strcpy(temp, ctime(&time));
    temp[strlen(temp)-1]=0;
    printf("%s ", temp); // time
    printf("%s", name); // name

    if ((ip->i_mode & 0xF000) == 0xA000)
        printf(" -> %s", l_name);

    iput(mip);
    
    return 0;
}

int ls_dir(MINODE *mip)
{
    char buf[BLKSIZE], temp[256], *cp;
    DIR *dp;
  
    // Assume DIR has only one data block i_block[0]
    get_block(dev, mip->inode.i_block[0], buf); 
    dp = (DIR *)buf;
    cp = buf;

    printf("[ls]: mip->ino=%d\n", mip->ino);
    if (mip->ino == 0)
        return 0;
  
    while (cp < buf + BLKSIZE){
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;
	
        //printf("[%d %s]  ", dp->inode, temp); // print [inode# name]
        MINODE *fmip = iget(dev, dp->inode);
        fmip->dirty=0;
        ls_file(fmip, temp);

        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
    putchar('\n');
    putchar('\n');

    iput(mip);

    return 0;
}

int ls(char *pathname)  
{
    u32 *ino = malloc(32);
    findino(running->cwd, ino);

    if (strlen(pathname) > 0){
        printf("[ls]: path=%s\n", pathname);
        *ino = getino(pathname);
    }

    printf("[ls]: no path\n");
    
    if (ino != 0)
        ls_dir(iget(dev, *ino));

    return 0;
}

char *rpwd(MINODE *wd){
    MINODE *pip;
    int p_ino=0;
    u32 *ino=malloc(8);
    char my_name[256];

    if (wd == root) return 0;

    p_ino = findino(wd, ino);
    pip = iget(dev, p_ino);

    findmyname(pip, *ino, my_name);

    rpwd(pip);
    printf("/%s", my_name);

    iput(pip);

    return 0;
}

char *pwd(MINODE *wd){
    if (wd == root){
        printf("/\n");
        return 0;
    }
    putchar('\n');
    putchar(' ');
    rpwd(wd);
    putchar('\n');
    putchar('\n');

    return 0;
}

/******************** Level 1 ********************/


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

// Does not handle rmdir of . or .. or /
int rm_dir(char *pathname){
    DIR *dp;
    char buf[BLKSIZE], name[256], temp[256], *cp;
    MINODE *mip, *pmip;
    int i, ino, pino;

    strcpy(temp, pathname);

    ino = getino(pathname);
    mip = iget(dev, ino);

    findmyname(mip, ino, name);
    printf("[rm_dir] : path=%s pino=%d parent name=%s\n", pathname, mip->ino, name);

    //show(mip); not working for some reason?

    printf("[rm_dir]: running->uid=%d ", running->uid);
    printf("[rm_dir]: ino->i_uid=%d\n", mip->inode.i_uid);

    if (running->uid != mip->inode.i_uid) return 0; // How to check if running PROC is superuser?
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
                printf("[rm_dir]: mip has children2! link_count=%d\n", actual_links);
        } else
            printf("[rm_dir]: mip has children1! link_count=%d\n", mip->inode.i_links_count);
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

int sym_link(char *src, char *dest){
    int sino, dino;
    char *name, temp[128], buf[BLKSIZE];
    MINODE *mip, *pmip;

    strcpy(temp, src);
    //path = dirname(temp);
    name = basename(src);
    name[strlen(name)] = 0;

    sino = getino(src);
    dino = getino(dest);
    if (sino > 0 && dino == 0){
        printf("[sym_link]: src exists and dest does not!\n");
        printf("[sym_link]: creating new file=%s\n", dest);
        creat_file(dest);
        dino = getino(dest);
        printf("[sym_link]: dest_ino=%d\n", dino);

        mip = iget(dev, dino);
        mip->inode.i_mode = 0127777; // set new (link) file type to link
        get_block(mip->dev, mip->inode.i_block[0], buf); // put name in link file block area
        strcpy(buf, name);
        put_block(mip->dev, mip->inode.i_block[0], buf);
        mip->inode.i_size = strlen(name); // modify size to = old file name
        mip->dirty = 1;
        iput(mip);
    }





    return 0;
}
=======
>>>>>>> 8bb95b2d787ecc98601cede73c9fe02f9ab13032
