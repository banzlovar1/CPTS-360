#include "../level1/commands.h"

int mount(char *filesys, char *mount_point){
    int i, fd, ino, mdev;
    MINODE *mip = running->cwd;
    MNTENTRY *mntptr;
    //char temp[64];

    // print mount table if one of inputs is null
    if (strlen(filesys)==0 || strlen(mount_point)==0){
        printf("[mount]: dev    name\n");
        printf("[mount]:-------------\n");
        for (i=0; i<NMNT; i++){
            mntptr = &mtable[i];
            
            if (mntptr->dev==0) break;

            printf("[mount]:  %d  %s\n", mntptr->dev, mntptr->name);
        }
        return 0;
    }

    if(running->uid != 0)
    {
        printf("[mount]: Permission denied, must be super user\n");
        return -1;
    }
    // do not want to mount an existing mounted system, also gets next open mnt entry
    for (i=0; i<NMNT; i++){
        mntptr = &mtable[i];
        if (mntptr->dev==0){
            printf("[mount]: found open mnt entry @ mtable[%d]\n", i);
            break;
        }
        if (strcmp(mntptr->name, filesys)==0) return -1;
    }

    printf("[mount]: Opening filesys=%s and checking if EXT2\n", filesys);

    if ((fd = open(filesys, O_RDWR))<0){
        printf("[mount]: could not open %s\n", filesys);
        return -1;
    }
    mdev=fd;
    printf("[mount]: open successful, dev=fd=%d\n", mdev);

    // get mount_point minode into memory
    ino = getino(mount_point);
    mip = iget(dev, ino);

    printf("[mount]: ino=%d mip=%p ip=%p\n", ino, mip, ip);

    // is a dir
    if ((mip->inode.i_mode & 0xF000) != 0x4000){
        printf("[mount]: mount_point=%s is not a dir\n", mount_point);
        return -1;
    }
    printf("[mount]: mount_point=%s is a dir\n", mount_point);
    // not busy
    if (running->cwd == mip){ // or if other process cwd
        printf("[mount]: mount_point=%s is busy\n", mount_point);
        return -1;
    }
    printf("[mount]: mount_point=%s is not busy\n", mount_point);

    // fill in mntentry attributes
    mntptr->mptr = mip;
    mntptr->dev = mdev;
    strncpy(mntptr->name, filesys, 64);

    // update mount_point mip
    mip->mounted=1;
    mip->mntptr = mntptr;

    printf("[mount]: Successfully mounted %s to %s!\n", filesys, mount_point);

    mip->mounted++;
    return 0;
}

int umount(char *filesys){
    int i, fd, ino, mdev;
    MINODE *mip = running->cwd;
    MNTENTRY *mntptr;

    if(running->uid != 0)
    {
        printf("[umount]: Permission denied, must be super user\n");
        return -1;
    }

    printf("[umount]: filesys=%s\n", filesys);
    // Check mount table to see if it has is mounted
    for (i=0; i<NMNT; i++){
        mntptr = &mtable[i];
        if (!strcmp(mntptr->name, filesys)){
            printf("[umount]: Found Mounted Disk @ mntable[%d]\n", i);
            break;
        }
        if (mntptr->dev==0){
            printf("[umount]: Could not find disk\n");
             return -1;
        }
    }
    // Check all dev in mounted system
    for(int j = 0; j < NMINODE; j++)
    {
        if(minode[j].dev == mtable[i].dev && minode[i].ino != 2)
        {
            printf("[umount]: Files in use, could not umount\n");
            return -1;
        }

    }

    printf("[umount]: File is not busy\n");

    // get mount_point minode into memory
    mntptr->mptr->mounted = 0;
    mntptr->dev = 0;
    iput(mntptr->mptr);

    return 0;
}
