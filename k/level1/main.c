// Lucas Mason, Brett Anzlovar cs360 project
#include "level1/util.c"
#include "level1/cd_ls_pwd.c"
#include "level1/mkdir_creat.c"
#include "level1/rmdir.c"
#include "level1/symlink.c"
#include "level1/link.c"
#include "level1/unlink.c"
#include "level2/open_close_lseek.c"
#include "level2/read_cat.c"
#include "level2/write.c"
#include "level2/cp.c"
#include "level2/mv.c"
#include "level3/mount_umount.c"

// global variables
MINODE minode[NMINODE];
MINODE *root;

PROC proc[NPROC], *running;
OFT oft[NOFT];
MNTENTRY mtable[NMNT];

char default_disk[6]="mydisk";
char gpath[128]; // global for tokenized components
char *name[32];  // assume at most 32 components in pathname
int   n;         // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start; // disk parameters

int quit();

int init()
{
    int i, j;
    MINODE *mip;
    PROC   *p;
    MNTENTRY *mntptr; 

    printf("init()\n");

    for (i=0; i<NMINODE; i++){
        mip = &minode[i];
        mip->dev = mip->ino = 0;
        mip->refCount = 0;
        mip->mounted = 0;
        mip->mntptr = 0;
    }
    for (i=0; i<NPROC; i++){
        p = &proc[i];
        p->pid = i;
        p->uid = p->gid = 0;
        p->cwd = 0;
        p->status = FREE;
        for (j=0; j<NFD; j++)
            p->fd[j] = 0;
    }
    for (i=0; i<NMNT; i++){
        mntptr = &mtable[i];
        mntptr->dev=0;
        mntptr->mptr=0;
    }

    return 0;
}

// load root INODE and set root pointer to it
int mount_root()
{  
    printf("mount_root()\n");
    root = iget(dev, 2);

    return 0;
}

char *disk;
int main(int argc, char *argv[ ])
{
    //int ino;
    char buf[BLKSIZE];
    char line[128], cmd[32], src[128], dest[128];

    if (argc>1)
        disk=argv[1];
    else{
        printf("[main]: no disk specified, using default=%s\n", default_disk);
        disk=default_disk;
    }
    
 
    printf("checking EXT2 FS ....");
    if ((fd = open(disk, O_RDWR)) < 0){
        printf("open %s failed\n", disk);
        exit(1);
    }
    dev = fd;    // fd is the global dev 

    /********** read super block  ****************/
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;

    /* verify it's an ext2 file system ***********/
    if (sp->s_magic != 0xEF53){
        printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
        exit(1);
    }     
    printf("EXT2 FS OK\n");
    ninodes = sp->s_inodes_count;
    nblocks = sp->s_blocks_count;

    get_block(dev, 2, buf); 
    gp = (GD *)buf;

    bmap = gp->bg_block_bitmap;
    imap = gp->bg_inode_bitmap;
    inode_start = gp->bg_inode_table;
    printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);

    init();  
    mount_root();
    printf("root refCount = %d\n", root->refCount);

    printf("creating P0 as running process\n");
    running = &proc[0];
    proc[0].uid = 0;
    running->status = READY;
    running->cwd = iget(dev, 2);
    printf("root refCount = %d\n", root->refCount);

    // WRTIE code here to create P1 as a USER process
    proc[1].uid = 1;
    proc[1].status = FREE;
    while(1){
        printf("running p%d: [ls|cd|pwd|mkdir|rmdir|touch|symlink|link|unlink|open|read|lseek|pfd|write|close|cat|cp|mv|mount|umount|sw|quit] ", running->uid);
        fgets(line, 128, stdin);
        line[strlen(line)-1] = 0;

        *src=*dest=0; // reset src and dest strings

        if (line[0]==0)
        continue;

        sscanf(line, "%s %s %s", cmd, src, dest);
        printf("[main]: cmd=%s src=%s dest=%s\n", cmd, src, dest);

        src[strlen(src)] = 0;
        dest[strlen(dest)] = 0;

        if (strcmp(cmd, "ls")==0)
            ls(src);
        else if (strcmp(cmd, "cd")==0)
            cd(src);
        else if (strcmp(cmd, "pwd")==0)
            pwd(running->cwd);
        else if (strcmp(cmd, "mkdir")==0)
            make_dir(src);
        else if (strcmp(cmd, "rmdir")==0)
            rm_dir(src);
        else if (strcmp(cmd, "touch")==0)
            creat_file(src);
        else if (strcmp(cmd, "symlink")==0)
            sym_link(src, dest);
        else if (strcmp(cmd, "link")==0)
            link_file(src, dest);
        else if (strcmp(cmd, "unlink")==0)
            unlink_file(src);
        else if (strcmp(cmd, "open")==0)
            open_file(src, dest);
        else if (strcmp(cmd, "read")==0)
            read_file(src, dest);
        else if (strcmp(cmd, "cat")==0)
            cat_file(src);
        else if (strcmp(cmd, "close")==0)
            close_file(atoi(src));
        else if (strcmp(cmd, "lseek")==0)
            lseek_file(atoi(src), atoi(dest));
        else if (strcmp(cmd, "pfd")==0)
            pfd();
        else if (strcmp(cmd, "cp")==0)
            cp_file(src, dest);
         else if (strcmp(cmd, "mv")==0)
            mv_file(src, dest);
        else if (strcmp(cmd, "write")==0)
            write_file();
        else if (strcmp(cmd, "mount")==0)
            mount(src, dest);
        else if (strcmp(cmd, "umount")==0)
            umount(src);
         else if (strcmp(cmd, "sw")==0)
         {
             if(running->uid == 0)
             {
                printf("[sw]: switch from p1 to p0\n");
                running = &proc[1];
                running->status = READY;
                running->cwd = iget(dev, 2);
                proc[0].status = FREE;
             }
             else
             {
                printf("[sw]: switch from p0 to p1\n");
                running = &proc[0];
                running->status = READY;
                running->cwd = iget(dev, 2);
                proc[1].status = FREE;
             }
             

         }
        else if (strcmp(cmd, "quit")==0 || strcmp(cmd, "q")==0)
            quit();
    }
}

int quit()
{
    int i;
    MINODE *mip;
    for (i=0; i<NMINODE; i++){
        mip = &minode[i];
        if (mip->refCount > 0)
        iput(mip);
    }
    exit(0);
}
