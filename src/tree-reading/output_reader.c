#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "tree-reading/tree_reader.h"

//TODO : maybe change the STDOUT_FILE by smth else

int output_reader(const char* path, bool is_stderr){

    char* buffer = NULL;
    int result = 0;
    if(buffer_init(&buffer) == -1){
        return -1;
    }
    //Reading the standard output file
    int fd = open(path, O_RDONLY);
    if(fd < 0){
        if(is_stderr)
            perror("open standard error file");
        else{
            perror("open standard output file");
        }
        result = -1;
        goto error;
    }
    while(1){
        ssize_t nread = read(fd, buffer, BUFFER_SIZE);

        if(nread < 0){
            perror("nread");
            result = -1;
            goto error;
        }else{
            //Detection of an anomaly
            if(nread != 0){
                if(write(STDOUT_FILENO, buffer, nread) != nread){
                    if(is_stderr)
                        perror("write standard error to STDOUT");
                    else{
                        perror("write standard output to STDOUT");
                    }
                    result = -1;
                    goto error;
                }
            }else{
                break;
            }
        }
    }
    error:
    free(buffer);
    buffer = NULL;
    if(close(fd) != 0){
        result = -1;
        perror("close");
    }
    return result;
}