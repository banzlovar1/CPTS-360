#ifndef __360PROJ_COMMANDS__
#define __360PROJ_COMMANDS__

#include <libgen.h>
#include <time.h>

#include <sys/stat.h>

#include "util.h"

int cd(char *pathname);
int ls_file(MINODE *mip, char *name);
int ls_dir(MINODE *mip);
int ls(char *pathname);
char *pwd(MINODE *wd);
int make_dir(char *pathname);
int mymkdir(MINODE *pip, char *name);
int creat_file(char *pathname);
int mycreat(MINODE *pip, char *name);

#endif
