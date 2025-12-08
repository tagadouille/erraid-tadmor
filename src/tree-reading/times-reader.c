#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "tree-reading/tree_reader.h"
#include "types/timing.h"
#include "types/time_exitcode.h"
#include "erraid.h"

int timing_reader(const char* path, int (*interpreter)(char* data, const char* path, ssize_t size)){
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
    ssize_t buffer_size = BUFFER_SIZE;
    unsigned int buf_ptr = 0; //Pointer to the current position in the buffer
    unsigned int filesize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET); //Resetting the file descriptor to the beginning of the file

    while(buf_ptr < filesize){
        //Reallocation of the buffer if needed
        if (buf_ptr == buffer_size){
            buffer_size *= 2;
            char* new_buffer = realloc(buffer, buffer_size);

            if(new_buffer == NULL){
                perror("realloc");
                result = -1;
                goto error;
            }
            buffer = new_buffer;
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
        }
    }
    //Interpreting the read data
    result = interpreter(buffer, path, buf_ptr);

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
        time_array_t* arr = time_exitcode_parse(data, size);

        if(arr == NULL){
            dprintf(STDERR_FILENO, "Error while showing the time-exitcodes file");
            return -1;
        }
        
    }else{
        dprintf(STDERR_FILENO, "times-exitcodes file is empty at path %s\n", path);
        return -1;
    }
    return 0;
}

int timing_interpreter(char* data, const char* path, ssize_t size){
    if(size > 0){
        timing_t* timing = timing_create(data, size);

        if(timing == NULL){
            dprintf(STDERR_FILENO, "Error while creating the timing structure");
            free(timing);
            timing = NULL;
            return -1;
        }
        curr_task -> timing -> minutes = timing -> minutes;
        curr_task -> timing -> hours = timing -> hours;
        curr_task -> timing -> daysofweek = timing -> daysofweek;

    }else{
        dprintf(STDERR_FILENO, "timing file is empty at path %s\n", path);
        return -1;
    }
    return 0;
}