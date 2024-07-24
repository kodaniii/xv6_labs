#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

int gets_buf(char* buf, int max){
    gets(buf, max);
    if(buf[0] == '\0' || buf[0] == '\n') return 0;
    if(buf[strlen(buf) - 1] == '\n'){
            buf[strlen(buf) - 1] = '\0';
        }
    return 1;
}

//echo 123 | xargs echo 455 | xargs echo 566
int main(int argc, char** argv){
    int argv_idx = 0;
    char* exec_argv[MAXARG];
    for(argv_idx = 0; argv_idx < argc - 1; argv_idx ++){
        exec_argv[argv_idx] = (char*)malloc(strlen(argv[argv_idx + 1]) + 1);
        strcpy(exec_argv[argv_idx], argv[argv_idx + 1]);
    }
    exec_argv[++ argv_idx] = 0;

    //int p[2];
    //pipe(p);

    char buf[512];
    //gets(char* buf, int max) read from STDIN, break when buf[last] == '\n' or '\r'
    //get a line from STDIN
    //echo 123 | xargs echo 456, then buf[] = 123

    while(gets_buf(buf, 512)){
        //fprintf(2, "gets buf = %s, strlen(buf) = %d.\n", buf, strlen(buf));
        
        //fprintf(2, "buf[0] = %c, strlen(buf) = %d.\n", buf[0], strlen(buf));
        //fprintf(2, "buf = %s.\n", buf);
        
        //for(int i = 0; i < argc; i++){
        //    fprintf(2, "argv[%d] = %s\n", i, argv[i]);
        //}



        if(fork() == 0){
            //fprintf(2, "<child> gets buf = %s, strlen(buf) = %d.\n", buf, strlen(buf));
            
            //xargs优化
            //echo "1\n2" | xargs echo line
            //echo line 1 2
            int i = 0;
            while(buf[i] != 0){
                if(buf[i] == '\\' && buf[i + 1] == 'n'){
                    buf[i] = ' ';

                    memmove(buf + i + 1, buf + i + 2, strlen(buf) + 1 - 1 - i);
                }

                i++;
            }

            //fprintf(2, "<child> optimize_buf = %s, strlen(buf) = %d.\n", buf, strlen(buf));
            
            i = -1;
            while(exec_argv[++ i] != 0);
            
            exec_argv[i] = (char*)malloc(strlen(buf) + 1);
            strcpy(exec_argv[i], buf);
            exec_argv[++i] = 0;
            
            //exec_argv[argv_idx - 1] = (char*)malloc(strlen(buf) + 1);
            //strcpy(exec_argv[argv_idx - 1], buf);
            //exec_argv[++ argv_idx] = 0;

            //for(int i = 0; exec_argv[i] != 0; i ++){
            //    fprintf(2, "<child> exec_argv[%d] = %s\n", i, exec_argv[i]);
            //}

            exec(exec_argv[0], exec_argv);
        }
    }
    
    exit(0);
}


// //echo 123 | xargs echo 455 | xargs echo 566
// int main(int argc, char** argv){
//     //char* _argv[MAXARG];
//     //for(int i = 0; i < argc; i ++){
//         //_argv[i] = argv[i];
//         //fprintf(1, "get _argv[%d] = %s\n", i, _argv[i]);
//         //fprintf(2, "<%d>: get _argv[%d] = %s\n", getpid(), i, _argv[i]);
//         */
//         $ echo 123 | xargs echo 455 | xargs echo 566
//         <52>: get _argv[0] = xargs
//         <52>: get _argv[1] = echo
//         <52>: get _argv[2] = 566
//         <51>: get _argv[0] = xargs
//         <51>: get _argv[1] = echo
//         <51>: get _argv[2] = 455
//         //get buf[] = 123
//         */
//     //}

//     int argv_idx = 0;
//     char* exec_argv[argc];
//     for(argv_idx = 0; argv_idx < argc - 1; argv_idx ++){
//         exec_argv[argv_idx] = (char*)malloc(strlen(argv[argv_idx + 1]) + 1);
//         strcpy(exec_argv[argv_idx], argv[argv_idx + 1]);
//         //fprintf(2, "find_argv[%d] = %s\n", argv_idx, exec_argv[argv_idx]);
//     }
//     exec_argv[++ argv_idx] = 0;

//     int p[2];
//     pipe(p);

//     char buf[512];
//     //gets(char* buf, int max) read from STDIN, break when buf[last] == '\n' or '\r'
//     //get a line from STDIN
//     //echo 123 | xargs echo 456, then buf[] = 123
//     while(gets(buf, 512) && buf[0] != '\0'){
//         //fprintf(1, "get buf[] = %s\n", buf);
//         //fprintf(2, "get buf[] = %s\n", buf);

//         int pid = fork();
//         if(pid > 0){
//             write(p[1], buf, strlen(buf));
//             wait(0);
//             /*
//             $ echo 123 | xargs echo 455 | xargs echo 566
//             <52>: get _argv[0] = xargs
//             <52>: get _argv[1] = echo
//             <52>: get _argv[2] = 566
//             <51>: get _argv[0] = xargs
//             <51>: get _argv[1] = echo
//             <51>: get _argv[2] = 455
//             get buf[] = 123
//             */
//         }

//         else if(pid == 0){
//         /*
//         if (strcmp(_argv[0] == "xargs") == 0){
//             //find file that filename == argv[0]
//             char* find_argv[4];
//             find_argv[0] = "find";
//             find_argv[1] = ".";
//             //find_argv[2] = _argv[1];
//             find_argv[2] = (char*)malloc(strlen(_argv[1]) + 1);
//             strcpy(find_argv[2], _argv[1]);
//             find_argv[3] = 0;

//             for(int i = 0; i < 4; i++){
//                 fprintf(2, "find_argv[%d] = %s\n", i, find_argv[i]);
//             }
//             exec("./find", find_argv);
//         }
//         */
//             while(read(p[0], buf, strlen(buf)) == strlen(buf)){
//                 exec_argv[argv_idx - 1] = (char*)malloc(strlen(buf) + 1);
//                 strcpy(exec_argv[argv_idx - 1], buf);
//                 exec_argv[++ argv_idx] = 0;

//                 //for(int i = 0; exec_argv[i] != 0; i ++){
//                 //    fprintf(2, "exec_argv[%d] = %s\n", i, exec_argv[i]);
//                 //}

//                 exec(exec_argv[0], exec_argv);
//             }
//         }
//     }

//     exit(0);
// }