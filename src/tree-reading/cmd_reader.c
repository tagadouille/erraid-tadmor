#define _GNU_SOURCE

#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "tree-reading/tree_reader.h"
#include "tree-reading/cmd_reader.h"
#include "types/argument.h"
#include "types/task.h"

int cmd_reader(const char* path){
    int result = 0;

    char* type_path = make_path(path, "type");

    if(type_path == NULL){
        result = -1;
        goto error;
    }

    // Trying to acces to the type file
    command_type_t type = type_reader(type_path);

    if(type == INVALID){
        dprintf(STDERR_FILENO, "Error while reading type file of task at path %s\n", path);
        result = -1;
        goto error;
    }
    // If the type is SI, construct the task by reading the argv file
    if(type == SI){
        curr_task -> cmd = type_processor(path, SI, curr_task -> cmd, NULL);
        
        if(curr_task -> cmd == NULL){
            result = -1;
            goto error;
        }
    }// Else command_parser is used
    else{
        command_t* cmd = create_command(curr_task -> cmd, type);
        cmd = command_parser(path, cmd);
        
        if(cmd == NULL){
            result = -1;
            goto error;
        }
        curr_task -> cmd = cmd;
    }
    error:
    if(result == -1){
        dprintf(STDERR_FILENO, "Error while reading cmd folder of task at path %s\n", path);
    }
    free(type_path);
    type_path = NULL;
    return result;
}

command_t* command_parser(const char* path, command_t* cmd){
    DIR* dirp = NULL;

    // Creation of the paths to the argv and type files
    char* argv_path = make_path_no_test(path, "argv");

    if(argv_path == NULL){
        return NULL;
    }
    char* curr_type_path = make_path(path, "type");

    if(curr_type_path == NULL){
        return NULL;
    }
    // We are in a leaf node, we read the argv file
    if(access(argv_path, F_OK) == 0){
        cmd = argv_reader(path, cmd);

        if(cmd == NULL){
            dprintf(STDERR_FILENO, "Error while reading argv file of task at path %s\n", path);
            free(argv_path);
            argv_path = NULL;
            goto cmd_clean;
        }
        free(argv_path);
        argv_path = NULL;
        return cmd;
    }

    free(argv_path);
    argv_path = NULL;

    dirp = opendir(path);

    if(dirp == NULL){
        perror("opendir for command tree parsing");
        goto cmd_clean;
    }

    //Reading the type file of the current folder
    command_type_t type = type_reader(curr_type_path);
    free(curr_type_path);
    curr_type_path = NULL;

    if(type == INVALID){
        dprintf(STDERR_FILENO, "Error while reading type file of task at path %s\n", path);
        goto cmd_clean;
    }
    struct dirent* entry;

    // Reading all the direct sons of the current node
    while((entry = readdir(dirp)) != NULL){
        //Skipping the . and .. entries
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }
        //We check if the entry is a directory
        if(entry->d_type == DT_DIR){
            // Test if the name is an int :
            char *end;
            strtol(entry->d_name, &end, 10);

            if (*end != '\0' || errno == ERANGE) {
                continue;
            }
            //Recursively calling command_parser on the subdirectory
            char* sub_path = make_path(path, entry->d_name);

            if(sub_path == NULL){
                goto cmd_clean;
            }
            //Construction of the path to the type file of the son command
            char* type_path = make_path(sub_path, "type");

            if(type_path == NULL){
                free(sub_path);
                sub_path = NULL;
                goto cmd_clean;
            }
            // Construction of the son command
            //Reading the type file of the son and filling the command structure accordingly
            command_t* son_cmd = NULL;
            command_type_t son_type = type_reader(type_path);

            if(son_type == INVALID){
                dprintf(STDERR_FILENO, "Error while reading type file of task at path %s\n", sub_path);
                cmd = NULL;
                goto clean;
            }

            son_cmd = create_command(son_cmd, son_type);

            if(son_cmd == NULL){
                goto clean;
            }
            son_cmd = command_parser(sub_path, son_cmd);

            if(son_cmd == NULL){
                goto clean;
            }
    
            cmd = type_processor(path, type, cmd, son_cmd);
            if(cmd == NULL){
                dprintf(STDERR_FILENO, "Error while reading type file of task at path %s\n", path);
                goto clean;
            }
            //freeing the allocated paths
            free(type_path);
            type_path = NULL;

            free(sub_path);
            sub_path = NULL;
            continue;

            clean:
                if (type_path) free(type_path);
                if (sub_path) free(sub_path);
                if (son_cmd)  command_free(son_cmd);
                goto cmd_clean;
        }
    }

    if(dirp != NULL && closedir(dirp) != 0){
        perror("closedir");
        goto clean;
    }
    return cmd;

    cmd_clean:
    dprintf(STDERR_FILENO, "Error while parsing command tree at path %s\n", path);
    command_free(cmd);

    if(dirp != NULL && closedir(dirp) != 0){
        perror("closedir");
    }
    return NULL;
}

command_t* argv_reader(const char* path, command_t* og_command){
    char* buffer = NULL;
    int fd = -1;

    if(buffer_init(&buffer) < 0){
        command_free(og_command);
        return NULL;
    }

    //Construction of the path to the argv file
    char* argv_path = make_path(path, "argv");
    if(argv_path == NULL){
        command_free(og_command);
        goto error;
    }
    
    //Reading the type file
    fd = open(argv_path, O_RDONLY);

    if(fd < 0){
        perror("open argv file");
        command_free(og_command);
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
                command_free(og_command);
                goto error;
            }
            buffer = new_buffer;
        }
        //Reading from the file
        ssize_t nread = read(fd, buffer + buf_ptr, buffer_size - buf_ptr);

        if(nread < 0){
            perror("nread");
            command_free(og_command);
            goto error;
        }else if(nread == 0){
            break;
        }else{
            buf_ptr += nread;
        }
    }
    og_command = command_filler(buffer, buf_ptr, og_command, SI);
    if(og_command == NULL){
        dprintf(STDERR_FILENO, "Error while filling the command structure from argv file at path %s\n", path);
        goto error;
    }

    error:
    if (buffer != NULL){
        free(buffer);
        buffer = NULL;
    }
    if(fd != -1 && close(fd) != 0){
        perror("close");
        command_free(og_command);
    }
    return og_command;
}

command_type_t type_reader(const char* path){
    char* buffer = NULL;
    int result = INVALID;
    int fd = -1;

    if(buffer_init(&buffer) < 0){
        return INVALID;
    }
    //Reading the type file
    fd = open(path, O_RDONLY);

    if(fd < 0){
        perror("open type file");
        goto error;
    }
    ssize_t nread = read(fd, buffer, BUFFER_SIZE);

    if(nread < 0){
        perror("nread");
        goto error;
    }else{
        //Detection of an anomaly
        if(nread == 2){
            buffer[nread] = '\0';

            command_type_t type = type_interpreter(buffer);

            if(type == INVALID){
                goto error;
            }
            result = type;
        }else{
            dprintf(STDERR_FILENO, "Error : type file has an invalid size\n");
            goto error;
        }
    }  
    error:
    if (buffer != NULL){
        free(buffer);
    }
    buffer = NULL;
    if(fd != -1 && close(fd) != 0){
        perror("close");
        result = INVALID;
    }
    return result;
}

//TODO adapt the functions according to the different types
command_type_t type_interpreter(char* buffer){

    if(strcmp(buffer, "SI") == 0){      
        return SI;
    }else if(strcmp(buffer, "SQ") == 0){
        return SQ;
    }else{
        dprintf(STDERR_FILENO, "Unknown command type : %s\n", buffer);
        return INVALID;
    }
}

command_t* type_processor(const char* path, command_type_t type, command_t* og_command, command_t* cmd){

    switch (type){
        case SI:
            og_command = argv_reader(path, og_command);
            return og_command;
        case SQ:
            og_command = add_complex_command(og_command, cmd);
            if(og_command == NULL){
                dprintf(STDERR_FILENO, "Error while adding command to composed command at path %s\n", path);
                return NULL;
            }
            return og_command;
        default:
            dprintf(STDERR_FILENO, "Error : invalid command type");
            return NULL;
    }
}