#ifndef __360PROJ_COMMANDS__
#define __360PROJ_COMMANDS__

#include <libgen.h>
#include <time.h>

#include <sys/stat.h>

#include "util.h"

// global vars from main
extern MINODE minode[NMINODE], *root;
extern PROC proc[NPROC], *running;
extern MNTENTRY mtable[NMNT];

extern int dev, imap, bmap, ninodes, nblocks;

// mountroot.c
int init();
int mount_root();
int quit();

// cd_ls_pwd.c
int cd(char *pathname);
int ls_file(MINODE *mip, char *name);
int ls_dir(MINODE *mip);
int ls(char *pathname);
char *rpwd(MINODE *wd);
char *pwd(MINODE *wd);

//mkdir_creat.c
int make_dir(char *pathname);
int mymkdir(MINODE *pip, char *name);
int enter_name(MINODE *pip, int myino, char *myname);
int creat_file(char *pathname);
int mycreat(MINODE *pip, char *name);

// rmdir.c
int rm_dir(char *pathname);
int rm_name(MINODE *pmip, char *name);

// symlink.c
int sym_link(char *src, char *dest);

// link.c
int link_file(char *pathname, char *linkname);

// unlink.c
int unlink_file(char *filename);

// open_close_lseek.c
int open_file(char *pathname, char *mode);
int truncate_file(MINODE *mip);
int close_file(int fd);

// read_cat.c
int read_file(char *fd, char *bytes);
int myread(int fd, char *buf, int bytes, int supress_msg);
int cat_file(char *pathname);

//write.c
int write_file();
int mywrite(int fd, char *buf, int nbytes);

// mount_umount.c
int mount(char *filesys, char *mount_point);
int umount(char *filesys);

#endif
