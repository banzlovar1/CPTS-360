/****************************************************************************
*                   KCW  Implement ext2 file system                         *
*****************************************************************************/
#include "util.c"
#include "mountroot.c"
#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "symlink.c"
#include "link.c"
#include "unlink.c"

// global variables
MINODE minode[NMINODE];
MINODE *root;

PROC proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[32];  // assume at most 32 components in pathname
int   n;         // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start; // disk parameters


char *disk = "mydisk";
int main(int argc, char *argv[ ])
{
    //int ino;
    char buf[BLKSIZE];
    char line[128], cmd[32], src[128], dest[128];
 
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
    running->status = READY;
    running->cwd = iget(dev, 2);
    printf("root refCount = %d\n", root->refCount);

    // WRTIE code here to create P1 as a USER process
  
    while(1){
        printf("input command : [ls|cd|pwd|mkdir|rmdir|touch|symlink|link|unlink|quit] ");
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
        else if (strcmp(cmd, "quit")==0 || strcmp(cmd, "q")==0)
            quit();
    }
}
