#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include "tree-reading/tree_reader.h"

//TODO : unify the two functions into one with an argument specifying the output type (stdout or stderr)
//TODO : maybe change the STDOUT_FILE by smth else

int standard_output_reader(char* path){

    char* buffer = NULL;
    int result = 0;
    if(buffer_init(&buffer) == -1){
        return -1;
    }
    //Reading the standard output file
    int fd = open(path, O_RDONLY);
    if(fd < 0){
        perror("open standard output file");
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
            //Dectection of an anomaly
            if(nread != 0){
                if(write(STDOUT_FILENO, buffer, nread) != nread){
                    perror("write standard output to STDOUT");
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

int error_output_reader(char* path){

    char* buffer = NULL;
    int result = 0;
    if(buffer_init(&buffer) == -1){
        return -1;
    }
    //Reading the error output file
    int fd = open(path, O_RDONLY);
    if(fd < 0){
        perror("open error output file");
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
            //Dectection of an anomaly
            if(nread != 0){
                if(write(STDOUT_FILENO, buffer, nread) != nread){
                    perror("write standard error output to STDOUT");
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