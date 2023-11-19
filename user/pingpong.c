#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    int fd[2];
    pipe(fd);
    int pid = fork();
    if(pid == 0) {
        char ch;
        read(fd[0], &ch, sizeof(char));
        close(fd[0]);
        printf("%d: received ping\n", getpid());
        write(fd[1], &ch, 1);
        close(fd[1]);
    }
    else {
        char ch = 'a';
        write(fd[1], &ch, sizeof(char));
        close(fd[1]);
        read(fd[0], &ch, sizeof(char));
        printf("%d: received pong\n", getpid());
        close(fd[0]);
    }
    exit(0);
}