#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "tree-reading/tree_reader.h"
#include "tree-reading/cmd_reader.h"

int cmd_reader(const char* path){
    printf("Reading cmd folder at path %s\n", path);

    DIR* dirp = opendir(path);
    int result = 0;

    if(dirp == NULL){
        perror("opendir");
        result = -1;
        goto ret;
    }
    struct dirent* entry;

    //Finding all the file and extracting some informations
    while ((entry=readdir(dirp))){
        if(entry -> d_name[0] == '.') continue;

        if (strcmp(entry -> d_name, "type") == 0){
        
            if(type_reader(path) == 1){
                dprintf(STDERR_FILENO, "Error while reading type file of task at path %s\n", path);
                result = -1;
                goto error;
            }
        }
        else if(strcmp(entry -> d_name, "argv") == 0){
            
            if(argv_reader(path) == 1){
                dprintf(STDERR_FILENO, "Error while reading argv file of task at path %s\n", path);
                result = -1;
                goto error;
            }
        }
    }
    error:
    if(closedir(dirp) != 0){
        perror("closedir");
        return -1;
    }
    ret:
    if(result == -1){
        dprintf(STDERR_FILENO, "Error while reading cmd folder of task at path %s\n", path);
    }
    return result;
}

int argv_reader(const char* path){
    char* buffer = NULL;
    int result = 0;
    int fd = -1;

    if(buffer_init(&buffer) == 1){
        return -1;
    }
    unsigned int buffer_size = BUFFER_SIZE;

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
    unsigned int multiplicator = 0;
    unsigned int buf_ptr = 0; //Pointer to the current position in the buffer

    while(1){
        multiplicator++;
        //Reallocation of the buffer if needed
        if(buf_ptr + BUFFER_SIZE > buffer_size){
            char* new_buffer = realloc(buffer, (BUFFER_SIZE * multiplicator)*2);

            if(new_buffer == NULL){
                perror("realloc");
                result = -1;
                goto error;
            }
            buffer = new_buffer;
            buffer_size = (BUFFER_SIZE * multiplicator)*2;
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
    //TODO parse the file and display the content
    buffer[buf_ptr] = '\0';
    dprintf(STDOUT_FILENO, "argv content : %s \n", buffer); // (provisional msg)

    error:
    free(buffer);
    buffer = NULL;

    if(fd != -1 && close(fd) != 0){
        perror("close");
        result = -1;
    }
    return result;
}

int type_reader(const char* path){
    char* buffer = NULL;
    int result = 0;
    int fd = 0;

    if(buffer_init(&buffer) == 1){
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
            dprintf(STDOUT_FILENO, "type of the given task : %s \n", buffer); // (provisional msg)
            int type = type_interpreter(buffer);

            if(type == -1){
                dprintf(STDERR_FILENO, "Error : unknown type found %s\n", buffer);
                result = -1;
                goto error;
            }
            if(type == SI){
                dprintf(STDOUT_FILENO, "The task is an individual task\n");
            }
            //For the complex tasks
            else{
                if(all_tasks_reader(path) == -1){
                    dprintf(STDERR_FILENO, "Error while reading all the sub-tasks\n");
                    result = -1;
                    goto error;
                }
            }
        }else{
            result = -1;
            dprintf(STDERR_FILENO, "Error : type file has an invalid size\n");
            goto error;
        }
    }  
    error:
    free(buffer);
    buffer = NULL;
    if(fd != -1 && close(fd) != 0){
        perror("close");
        result = -1;
    }
    return result;
}

//TODO adapt the function according to the different types
int type_interpreter(char* buffer){
    if(strcmp(buffer, "SI") == 0){
        return SI;
    }else if(strcmp(buffer, "SQ") == 0){
        return SQ;
    }else{
        return -1;
    }
}