#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

//from ls.c
char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void find(char* dir, char* filename){
    char buf[512], *p_buf;  //buf is used to save "dir/dir/.../filename", and the p_buf pointer is used to modify the content of buf
    int fd;
    struct dirent de;
    struct stat st;
    
    //fprintf(1, "find(): dir[] = %s, filename[] = %s\n", dir, filename);

    if((fd = open(dir, 0)) < 0){   //open(dir, 0): 0 <- O_RDONLY
        fprintf(2, "find: cannot open %s\n", dir);
        return;
    }

    //fprintf(1, "find(): fd = %d\n", fd);

    //dir msg -> &st
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", dir);
        close(fd);
        return;
    }

    //st.type == dir?
    if(st.type != T_DIR){
        fprintf(2, "usage: find <directory> <filename>\n");
        exit(1);
    }

    if(strlen(dir) + 1 + DIRSIZ + 1 > sizeof buf){
        fprintf(2, "find: directory too long\n");
        exit(1);
    }

    strcpy(buf, dir);
    //fprintf(1, "buf = %s, dir = %s\n", buf, dir);
    p_buf = buf + strlen(buf);  //if dir = /etc/abc, *(p_buf - 1) = c
    *p_buf++ = '/'; //*p_buf = '/'

    //fprintf(1, "[%c] [%c]\n", *p_buf, *(p_buf - 1));
    /*
    switch(st.type){
    case T_FILE:
        printf("%s %d %d %l\n", fmtname(dir), st.type, st.ino, st.size);
        break;
    }
    */

    //dir is a dicectory, and buf.length is long enough to append filename 
    //fd = open(dic, O_RDONLY)
    //buf[] = /etc/abc/, *p_buf = '/' 
    while(read(fd, &de, sizeof de) == sizeof de){
        if(de.inum == 0){
            continue;
        }
        //fprintf(1, "find() loop: fd = %d\n", fd);
        //fprintf(1, "find() loop: de.name = %s\n", de.name);
        
        memmove(p_buf, de.name, DIRSIZ); //dir + de.name -> a new dir or file
        p_buf[DIRSIZ] = '\0';
        //fprintf(1, "find() loop: p_buf[] = %s, buf[] = %s, dir[] = %s\n", 
        //          p_buf, buf, dir);
        //p_buf -= DIRSIZ + 
        //fprintf(1, "find() loop: new p_buf[] = %s, dir[] = %s\n", p_buf, dir);
        
        //stat(): fd = open(dir[], O_RDONLY) then r = fstat(fd, st)
        //    then close(fd) and return r 
        if(stat(buf , &st) < 0){
            fprintf(2, "find() loop: cannot stat %s\n", p_buf);
            continue;
        }
        //fprintf(1, "find() loop: %s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
        
        switch(st.type){
        case T_FILE:
            //fprintf(1, "find() loop: st.type = T_FILE, de.name[] = %s, filename[] = %s.\n", de.name, filename);
            if(strcmp(de.name, filename) == 0) {
                //printf("%s %d %d %l\n", fmtname(p_buf), st.type, st.ino, st.size);
                fprintf(1, "%s\n", buf);
            }
            break;
        case T_DIR:
            //fprintf(1, "find() loop: st.type = T_DIR, de.name[] = %s\n", de.name);
            if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0){
                break;
            }
            find(buf, filename);
            break;
        }
    }
    close(fd);
}

int main(int argc, char** argv){
    if(argc != 3){
        fprintf(2, "usage: find <directory> <filename>\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}