#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void main_processer(int* p){
    close(0); close(1);
    close(p[0]);
    for(int i = 2; i <= 35; i ++){
        if(write(p[1], &i, sizeof(int)) != sizeof(int)){
            fprintf(2, "ERROR: from main_processer write().\n");
            exit(1);
        }
        //fprintf(1, "main_processer loop %d\n", i);
    }
    close(p[1]);
    close(2);
    wait(0);
    exit(0);
}

void child_processer(int* p){
    close(p[1]);
    //close(0); //cause read false
    int pre_num = -1, num = -1;
    
    int _ret = read(p[0], &pre_num, sizeof(int));
    if(_ret == 0){
        exit(0);
    }
    if(!_ret){
        fprintf(2, "ERROR: from child_processer fork().\n");
        exit(1);
    }
    fprintf(1, "prime %d\n", pre_num);
    //close(1);

    int p_2[2];
    pipe(p_2);
    int _PID = fork();
    if(_PID > 0){
        while(read(p[0], &num, sizeof(int)) == sizeof(int)){    //TODO: read returns zero when the write-side of a pipe is closed.
            //fprintf(1, "PID[%d]: %d \% %d -> %d\n", getpid(), num, pre_num, num % pre_num);
            if(num % pre_num != 0){
                if(write(p_2[1], &num, sizeof(int)) != sizeof(int)){
                    fprintf(2, "ERROR: from child_processer write(p_2[1], ...).\n");
                    exit(1);
                }
            }
        }
        close(0);close(1);close(2);
        close(p[0]);
        close(p_2[0]);
        close(p_2[1]);
        wait(0);
    }
    else if(_PID == 0){
        child_processer(p_2);
    }
    else{
        fprintf(2, "ERROR: from child_processer fork().\n");
        exit(1);
    }
    //close(p_2[0]);
    //close(p_2[1]);
    //close(2);

    /*
    //should fork() before while loop
    while(read(p[0], &num, sizeof(int))){   //read pipe[0], until pipe is empty
        //fprintf(1, "child_processer while loop...\n");
        int p_2[2];
        pipe(p_2);
        fprintf(1, "num = %d, pre_num = %d, \% -> %d\n", num, pre_num, num % pre_num);
        if(num % pre_num != 0){
            int _PID = fork();
            if(_PID > 0){
                write(p_2[1], &num, sizeof(int));
            }
            else if(_PID == 0){
                child_processer(p_2);
            }
            else {
                fprintf(2, "error\n");
                //TODO
            }
        }
        close(p_2[1]);
        close(p_2[0]);
    }
    close(p[1]);    //need optimize
    close(p[0]);*/
    /*
    close(0);close(1);close(2);
    close(p[0]);
    close(p_2[0]);
    close(p_2[1]);*/
    exit(0);
}

int main(){
    int p[2];   //p[0] read, p[1] write
    pipe(p);

    int _PID = fork();
    if(_PID == 0){
        child_processer(p);
    }
    else if(_PID > 0){
        main_processer(p);
    }
    else{
        fprintf(2, "ERROR: from main().\n");
    }

    exit(0);
}