#include "commands.h"

char t1[9] = "xwrxwrxwr", t2[9] = "---------";

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
    int cur_dev = dev;
    rpwd(running->cwd);
    dev = cur_dev;
    putchar('\n');

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
    printf("%6d ", ip->i_size); // byte size
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
    } else {
        printf("[ls]: no path");
    }
    
    if (ino != 0)
        ls_dir(iget(dev, *ino));

    return 0;
}

char *rpwd(MINODE *wd){
    MINODE *pip, *newmip;
    int p_ino=0, i;
    u32 ino;
    char my_name[256];
    MNTENTRY *mntptr;

    // if at root of other dev, must hop to root dev mount point
    if (wd->dev != root->dev && wd->ino == 2){
        for (i=0; i<NMNT; i++)
            if ((mntptr=&mtable[i])->dev == wd->dev)
                break;

        newmip = mntptr->mptr;
        iput(wd);
        p_ino = findino(newmip, &ino);
        wd = iget(newmip->dev, ino);
        pip = iget(newmip->dev, p_ino);
        dev = newmip->dev;
    }
    if (wd == root) return 0;

    p_ino = findino(wd, &ino);
    pip = iget(dev, p_ino);

    findmyname(pip, ino, my_name);

    rpwd(pip);
    printf("/%s", my_name);

    iput(pip);

    return 0;
}

char *pwd(MINODE *wd){
    int cur_dev = dev;

    if (wd == root){
        printf("[pwd]: /\n");
        return 0;
    }
    printf("[pwd]: ");
    rpwd(wd);
    putchar('\n');

    // make sure dev is not changed
    dev = cur_dev;

    return 0;
}
