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

    printf("chdir %s\n", pathname);

    if (strlen(pathname) > 0)
        ino = getino(pathname);
    else
        ino = root->ino;

    mip = iget(dev, ino);

    if ((mip->inode.i_mode & 0xF000) == 0x4000){ // if is a dir
        iput(running->cwd);
        running->cwd = mip;
    }

    iput(mip);

    printf("\n cd to : ");
    pwd(running->cwd);

    return 0;
}

int ls_file(MINODE *mip, char *name)
{
    int i;
    time_t time;
    char temp[64];

    INODE *ip = &mip->inode;
    putchar('\n');
    
    if ((ip->i_mode & 0xF000) == 0x8000) // is_reg
        printf(" -");
    else if ((ip->i_mode & 0xF000) == 0x4000) // is_dir
        printf(" d");
    else if ((ip->i_mode & 0xF000) == 0xA000) // is_link
        printf(" l");

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
  
    while (cp < buf + BLKSIZE){
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;
	
        //printf("[%d %s]  ", dp->inode, temp); // print [inode# name]
        MINODE *fmip = iget(dev, dp->inode);
        ls_file(fmip, temp);
        iput(fmip);

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
    printf("ls %s\n", pathname);
    //printf("ls CWD only! YOU do it for ANY pathname\n");
    u32 *ino = malloc(32);
    findino(running->cwd, ino);

    printf("strlen(pathname)=%d\n", (int)strlen(pathname));

    if (strlen(pathname) > 0)
        *ino = getino(pathname);

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

    printf(" mkdir path=%s\n", path);
    printf(" mkdir name=%s\n", name);

    pino = getino(path);
    pmip = iget(dev, pino);

    if ((pmip->inode.i_mode & 0xF000) == 0x4000){ // is_dir
        if (search(pmip, name)==0){ // if can't find child name in start MINODE
            r = mymkdir(pmip, name);
            pmip->inode.i_links_count++; // increment link count
            pmip->inode.i_atime = time(0L); // touch atime
            pmip->dirty = 1; // make dirty
            iput(pmip); // write to disk

            printf("\n Successfully made new directory = %s\n\n", pathname);
            return r;
        } else {
            printf("\n That directory name=%s already exists.\n\n", name);
        }
    }

    return 0;
}

int mymkdir(MINODE *pip, char *name){
    char buf[BLKSIZE], *cp;
    DIR *dp;
    MINODE *mip;
    INODE *ip;
    int ino = ialloc(dev), bno = balloc(dev), i;
    printf("ino=%d\t bno=%d\n", ino, bno);

    mip = iget(dev, ino);
    ip = &mip->inode;

    char temp[256];
    findmyname(pip, pip->ino, temp);
    printf(" mycreat : ino=%d name=%s\n", pip->ino, temp);

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
    printf("write INODE to disk\n");

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
    printf("write data block %d to disk\n", bno);
    enter_name(pip, ino, name);

    iput(pip);

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

    printf(" creat path=%s\n", path);
    printf(" creat file=%s\n", name);

    pino = getino(path);
    pmip = iget(dev, pino);

    if ((pmip->inode.i_mode & 0xF000) == 0x4000){ // is_dir
        if (search(pmip, name)==0){ // if can't find child name in start MINODE
            r = mycreat(pmip, name);
            pmip->inode.i_atime = time(0L); // touch atime
            pmip->dirty = 1; // make dirty
            iput(pmip); // write to disk

            printf("\n Successfully created new file = %s\n\n", pathname);
            return r;
        } else {
            printf("\n That file name=%s already exists.\n\n", name);
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

    printf(" mycreat : search(pip)2\n");
    search(pip, "b");

    ip->i_mode = 0x81A4; // set to file type and set perms
    ip->i_uid = running->uid; // set owner uid
    ip->i_gid = running->gid; // set group id
    ip->i_size = 0; // no data blocks
    ip->i_links_count = 1; // . and ..
    //ip->i_blocks = 2; // each block = 512 bytes
    //ip->i_block[0] = bno; // new dir has only one data block
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); // set to current time

    mip->dirty = 1;
    iput(mip);
    printf("write INODE to disk\n");

    enter_name(pip, ino, name);

    return 0;
}
