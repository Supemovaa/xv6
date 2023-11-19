#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define NULL 0

void sieve(int *p){
    close(p[1]);
    int recv, prime;
    if(read(p[0], &prime, sizeof(int)) < 0){
        exit(1);
    }
    printf("prime %d\n", prime);
    int newp[2];
    pipe(newp);
    if(read(p[0], &recv, sizeof(int)) == 4){
        if (fork() == 0){
            sieve(newp);
            exit(0);
        }
        else{
            close(newp[0]);
            if(recv % prime != 0)
                write(newp[1], &recv, sizeof(int));
            while (read(p[0], &recv, sizeof(int)) == 4){
                if(recv % prime == 0)
                    continue;
                write(newp[1], &recv, sizeof(int));
            }
            close(p[0]);
            close(newp[1]);
            wait(NULL);
            exit(0);
        }
    }
    return;
}

int main(int argc, char *argv[]){
    int p[2];
    pipe(p);
    if (fork() == 0){
        sieve(p);
        exit(0);
    }
    else{
        close(p[0]);
        int i = 2;
        write(p[1], &i, sizeof(int));
        for(i = 3; i <= 35; i += 2){
            write(p[1], &i, sizeof(int));
        }
        close(p[1]);
        wait(NULL);
        exit(0);
    }
    return 0;
}