#include "commands.h"

int mount(char *filesys, char *mount_point){
    MINODE *mip = running->cwd;

    // print mount table if one of inputs is null
    if (strlen(filesys)==0 || strlen(mount_point)==0){
        printf("[mount]: Displaying mounted filesystems:");

        return 0;
    }

    struct mntable *mntableptr = mip->mntptr;

    mip->mounted++;
    return 0;
}

int umount(char *filesys){
    printf("[umount]: filesys=%s\n", filesys);

    return 0;
}
