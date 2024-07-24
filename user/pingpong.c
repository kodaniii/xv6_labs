#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char** argv){
    //char* str = malloc(strlen(argv[1]) * 1);
    //fprintf(1, "strlen(argv[1]) = %d\n", strlen(argv[1]));
    //strcpy(str, argv[1]);
    //fprintf(1, "str = %s\n", str);
    //fprintf(1, "argv[1][1] = %c\n", argv[1][0]);
    
    char _ch = argc != 1 ? argv[1][0]: '\0';

    int p_a[2]; //p_a pipe from main process to child process, [0] read, [1] write
    int p_b[2]; //p_b pipe from child process to main process, [0] read, [1] write   
    pipe(p_a);
    pipe(p_b);

    int PID = fork();
    int exit_status = 0;
    if(PID > 0){
        //fprintf(1, "Main process PID = %d is running..,\n", getpid());
        close(p_a[0]);
        close(p_b[1]);
        close(0);
        //fprintf(1, "send: %c\n", _ch);
        if(write(p_a[1], &_ch, sizeof(char)) != sizeof(char)){ //TODO ERROR handle
            fprintf(2, "ERROR: from main process write().\n");
            exit_status = 1;
        }
        close(p_a[1]);
        if(read(p_b[0], &_ch, sizeof(char)) != sizeof(char)){
            fprintf(2, "ERROR: from main process read().\n");
            exit_status = 1;
        }
        else{
            fprintf(1, "%d: received pong\n", getpid());
        }
        close(p_b[0]);
        close(1);
        //fprintf(1, "get: %c\n", _ch);
    }
    else if(PID == 0){
        close(p_a[1]);
        close(p_b[0]);
        close(0);
        if(read(p_a[0], &_ch, sizeof(char)) != sizeof(char)){ //TODO ERROR handle
            fprintf(2, "ERROR: from child process read().\n");
            exit_status = 1;
        }
        else{
            fprintf(1, "%d: received ping\n", getpid());
        }
        close(p_a[0]);
        close(1);
        //fprintf(1, "get: %c\n", _ch);
        if(write(p_b[1], &_ch, sizeof(char)) != sizeof(char)){
            fprintf(2, "ERROR: from child process write().\n");
            exit_status = 1;
        }
        close(p_b[1]);
    }
    else{
        close(0);close(1);
        fprintf(2, "ERROR: from fork().\n");
        close(p_a[0]);
        close(p_a[1]);
        close(p_b[0]);
        close(p_b[1]);
        exit_status = 1;
    }

    close(2);
    exit(exit_status);
}