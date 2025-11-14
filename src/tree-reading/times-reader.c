#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "tree-reading/tree_reader.h"

int timing_reader(const char* path, int (*interpreter)(char*, ssize_t)){

    char* buffer = NULL;
    int result = 0;
    if(buffer_init(&buffer) == -1){
        return -1;
    }
    //Opening the file
    int fd = open(path, O_RDONLY);
    if(fd < 0){
        perror("open the timing/exitcodes file");
        result = -1;
        goto error;
    }
    //Reading the file
    unsigned int multiplicator = 1;
    ssize_t buffer_size = BUFFER_SIZE;
    unsigned int buf_ptr = 0; //Pointer to the current position in the buffer

    while(1){
        //Reallocation of the buffer if needed
        if(multiplicator * BUFFER_SIZE > buffer_size){
            size_t new_size = (BUFFER_SIZE * multiplicator)*2;
            char* new_buffer = realloc(buffer, new_size);

            if(new_buffer == NULL){
                perror("realloc");
                result = -1;
                goto error;
            }
            buffer = new_buffer;
            buffer_size = new_size;
        }

        //Reading from the file
        ssize_t nread = read(fd, buffer + buf_ptr, buffer_size - buf_ptr);

        if(nread < 0){
            perror("nread");
            result = -1;
            goto error;
        }else if(nread == 0){
            break;
        }else{
            buf_ptr += nread;
            multiplicator++;
        }
    }
    //Interpreting the read data
    result = interpreter(buffer, buf_ptr);

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
        dprintf(STDOUT_FILENO, "times-exitcodes of the given task : %s \n", data); // (provisional msg)
        //TODO : interpret the times-exitcodes information
    }else{
        dprintf(STDOUT_FILENO, "times-exitcodes file is empty at path %s\n", path);
    }
    return 0;
}
int timing_interpreter(char* data, const char* path, ssize_t size){
    if(size > 0){
        data[size] = '\0';
        dprintf(STDOUT_FILENO, "timing of the given task : %s \n", data); // (provisional msg)
        //TODO : interpret the timing information
    }else{
        dprintf(STDOUT_FILENO, "timing file is empty at path %s\n", path);
    }
    return 0;
}