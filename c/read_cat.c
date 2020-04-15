#include "commands.h"

int read_file(char *fd, char *bytes){
    int i_fd = atoi(fd), i_bytes = atoi(bytes);
    char buf[BLKSIZE];
    printf("[read_file]: Reading fd=%d\n", i_fd);

    return myread(i_fd, buf, i_bytes);
}

int myread(int fd, char *buf, int bytes){
    int offset = running->fd[fd]->offset;
    printf("[myread]: fd=%d offset=%d bytes=%d\n", fd, offset, bytes);

    return 0;
}

int cat_file(char *filename){

    return 0;
}
