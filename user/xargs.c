#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char **argv){
    char *params[MAXARG];
    char buf[MAXARG], line[128];
    int m = 0, bytes, paraIdx = 0;
    for (int i = 1; i < argc; i++)
        params[paraIdx++] = argv[i];

    while((bytes = read(0, line, sizeof(line))) > 0){
        for (int i = 0; i < bytes; i++){
            if(line[i] == '\n'){
                buf[m++] = 0;
                params[paraIdx++] = buf;
                params[paraIdx] = 0;
                m = 0;
                paraIdx = argc - 1;
                if(fork() == 0)
                    exec(argv[1], params);
                else
                    wait(0);
            }
            else if(line[i] == ' '){
                buf[m++] = 0;
                params[paraIdx++] = buf;
                m = 0;
            }
            else{
                buf[m++] = line[i];
            }
        }
    }
    exit(0);
}