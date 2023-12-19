#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

/**
 * @brief find file "name" recursively, beginning from directory "path"
 * 
 * @param path 
 * @param name 
 */
void find(char *path, char *name){
    char buf[512], *p;
    int fd;
    // directory entry 文件目录项
    struct dirent de;
    struct stat st;

    // 打开目录文件path
    if((fd = open(path, O_RDONLY)) < 0){
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // path是一个目录文件
    switch(st.type){
        case T_DEVICE:
        case T_FILE:
            fprintf(2, "Error: Parameter 2 is supposed to be a directory, file given\n");
            break;

        case T_DIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("ls: path too long\n");
                break;
            }
            // 记录名字
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            // 读取目录文件下所有的目录项
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0)
                    continue;
                if(strcmp(".", de.name) == 0 || strcmp("..", de.name) == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0){
                    printf("ls: cannot stat %s\n", buf);
                    continue;
                }
                if(st.type == T_DEVICE || st.type == T_FILE){
                    if(strcmp(name, de.name) == 0)
                        printf("%s\n", buf);
                }
                // 如果是path目录下的目录，递归地find
                if(st.type == T_DIR){
                    find(buf, name);
                }
            }
            break;
    }
    close(fd);
}

int main(int argc, char *argv[]){
    if(argc != 3) {
        fprintf(2, "Error: Wrong number of arguments\n");
        exit(0);
    }
    find(argv[1], argv[2]);
    exit(0);
}