#define _GNU_SOURCE

#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "tree-reading/tree_reader.h"
#include "tree-reading/cmd_reader.h"
#include "types/argument.h"
#include "types/task.h"

int cmd_reader(const char* path, command_t* cmd){
    int result = 0;

    char* type_path = make_path(path, "type");
    char* argv_path = make_path(path, "argv");

    if(type_path == NULL || argv_path == NULL){
        result = -1;
        goto error;
    }

    if(access(type_path, F_OK) == 0){
        if(type_reader(path, cmd) == -1){
            dprintf(STDERR_FILENO, "Error while reading type file of task at path %s\n", path);
            result = -1;
            goto error;
        }
    }else{
        dprintf(STDERR_FILENO, "Type file doesn't exist at path %s\n", path);
    }
    error:
    if(result == -1){
        dprintf(STDERR_FILENO, "Error while reading cmd folder of task at path %s\n", path);
    }
    return result;
}

int argv_reader(const char* path, command_t* og_command, command_type_t type){
    char* buffer = NULL;
    int result = 0;
    int fd = -1;

    if(buffer_init(&buffer) < 0){
        return -1;
    }

    //Construction of the path to the argv file
    char* argv_path = make_path(path, "argv");
    if(argv_path == NULL){
        result = -1;
        goto error;
    }
    
    //Reading the type file
    fd = open(argv_path, O_RDONLY);

    if(fd < 0){
        perror("open type file");
        result = -1;
        goto error;
    }
    free(argv_path);
    argv_path = NULL;

    //Reading the argv file and extracting the information
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
    if(command_filler(buffer, buf_ptr, og_command, type) == -1){
        dprintf(STDERR_FILENO, "Error while filling the command structure from argv file at path %s\n", path);
        result = -1;
        goto error;
    }

    error:
    if (buffer != NULL){
        free(buffer);
        buffer = NULL;
    }
    if(fd != -1 && close(fd) != 0){
        perror("close");
        result = -1;
    }
    return result;
}

int type_reader(const char* path, command_t *cmd){
    char* buffer = NULL;
    int result = 0;
    int fd = -1;

    if(buffer_init(&buffer) < 0){
        return -1;
    }
    //Construction of the path to the type file
    char* type_path = make_path(path, "type");

    if(type_path == NULL){
        result = -1;
        goto error;
    }
    //Reading the type file
    fd = open(type_path, O_RDONLY);

    free(type_path);
    type_path = NULL;

    if(fd < 0){
        perror("open type file");
        result = -1;
        goto error;
    }
    ssize_t nread = read(fd, buffer, BUFFER_SIZE);

    if(nread < 0){
        perror("nread");
        result = -1;
        goto error;
    }else{
        //Detection of an anomaly
        if(nread == 2){
            buffer[nread] = '\0';

            if(type_interpreter(path, buffer, cmd) == -1){
                result = -1;
                goto error;
            }
        }else{
            result = -1;
            dprintf(STDERR_FILENO, "Error : type file has an invalid size\n");
            goto error;
        }
    }  
    error:
    if (buffer)
    {
        free(buffer);
    }
    buffer = NULL;
    if(fd != -1 && close(fd) != 0){
        perror("close");
        result = -1;
    }
    return result;
}

//TODO adapt the function according to the different types
int type_interpreter(const char* path, char* buffer, command_t* cmd){
    int result = 0;

    if(strcmp(buffer, "SI") == 0){
        printf("Reading a simple instruction at path %s\n", path);

        if(argv_reader(path, cmd, SI) == -1){
            dprintf(STDERR_FILENO, "Error while reading argv file of task at path %s\n", path);
            result = -1;
        }
    }else if(strcmp(buffer, "SQ") == 0){
        if(all_tasks_reader(path, cmd) == -1){
            dprintf(STDERR_FILENO, "Error while reading all the sub-tasks\n");
            result = -1;
        }
    }else{
        dprintf(STDERR_FILENO, "Unknown command type : %s\n", buffer);
        return -1;
    }
    return result;
}