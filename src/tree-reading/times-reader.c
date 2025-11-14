#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "tree-reading/tree_reader.h"
#include "types/timing.h"
#include "types/time_exitcode.h"

int timing_reader(const char* path, int (*interpreter)(char*, ssize_t)){
    printf("Reading timing/exitcodes file at path %s\n", path);
    char* buffer = NULL;
    int result = 0;

    if(buffer_init(&buffer) == -1){
        return -1;
    }
    //Reading the file
    int fd = open(path, O_RDONLY);
    if(fd < 0){
        perror("open the timing/exitcodes file");
        result = -1;
        goto error;
    }
    ssize_t nread = read(fd, buffer, BUFFER_SIZE);
    if(nread < 0){
        perror("nread");
        result = -1;
        goto error;
    }else{
        if(interpreter(buffer, nread) < 0){
            //TODO display an error message
            result = -1;
            goto error;
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
int times_exitcodes_interpreter(char* data, const char* path, ssize_t size){
    if(size > 0){
        data[size] = '\0';
        char* output =time_exitcode_show(data, size);
        dprintf(STDOUT_FILENO, "times-exitcodes of the given task : %s \n", output); // (provisional msg)
        free(output);
        output = NULL;
    }else{
        dprintf(STDOUT_FILENO, "times-exitcodes file is empty at path %s\n", path);
    }
    return 0;
}
int timing_interpreter(char* data, const char* path, ssize_t size){
    if(size > 0){
        data[size] = '\0';
        char* output = timing_show(data, size);
        dprintf(STDOUT_FILENO, "timing of the given task : %s \n", output); // (provisional msg)
        free(output);
        output = NULL;
    }else{
        dprintf(STDOUT_FILENO, "timing file is empty at path %s\n", path);
    }
    return 0;
}