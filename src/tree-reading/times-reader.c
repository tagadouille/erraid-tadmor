#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "tree-reading/tree_reader.h"

int timing_reader(char* path){

    char* buffer = NULL;
    int result = 0;
    if(buffer_init(&buffer) == -1){
        return -1;
    }
    //Reading the timing file
    int fd = open(path, O_RDONLY);
    if(fd < 0){
        perror("open timing file");
        result = -1;
        goto error;
    }
    ssize_t nread = read(fd, buffer, BUFFER_SIZE);
    if(nread < 0){
        perror("nread");
        result = -1;
        goto error;
    }else{
        //Dectection of an anomaly
        if(nread > 0){
            buffer[nread] = '\0';
            dprintf(STDOUT_FILENO, "timing of the given task : %s \n", buffer); // (provisional msg)
            //TODO : interpret the timing information
        }else{
            dprintf(STDOUT_FILENO, "timing file is empty at path %s\n", path);
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
int times_exitcodes_reader(char* path){

    char* buffer = NULL;
    int result = 0;
    if(buffer_init(&buffer) == -1){
        return -1;
    }
    //Reading the times-exitcodes file
    int fd = open(path, O_RDONLY);
    if(fd < 0){
        perror("open times-exitcodes");
        result = -1;
        goto error;
    }
    ssize_t nread = read(fd, buffer, BUFFER_SIZE);
    if(nread < 0){
        perror("nread");
        result = -1;
        goto error;
    }else{
        //Dectection of an anomaly
        if(nread > 0){
            buffer[nread] = '\0';
            dprintf(STDOUT_FILENO, "times-exitcodes of the given task : %s \n", buffer); // (provisional msg)
            //TODO : interpret the times-exitcodes information
        }else{
            dprintf(STDOUT_FILENO, "times-exitcodes file is empty at path %s\n", path);
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