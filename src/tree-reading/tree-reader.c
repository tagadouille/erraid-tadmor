#define _GNU_SOURCE

#include <limits.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "tree-reading/cmd_reader.h"
#include "tree-reading/times_reader.h"
#include "tree-reading/tree_reader.h"
#include "tree-reading/output_reader.h"
#include "erraid.h"

int task_reader(const char* path, uint64_t task_id, Action_type action){
    dprintf(STDERR_FILENO, "DEBUG: task_reader called with path='%s', task_id=%lu\n",
        path, task_id);

    curr_task = task_create(task_id);

    if(curr_task == NULL){
        dprintf(STDERR_FILENO, "Error while creating task with id %lu\n", task_id);
        return -1;
    }

    //Converting task_id to string :
    char id[6];
    snprintf(id, sizeof(id), "%lu", task_id);

    int result = task_finder(path, id, action);

    //If an error occured while finding the task, we free curr_task
    if(result == -1){
        task_destroy(curr_task);
    } 
    return result;
}

int task_finder(const char* path, char* task_id, Action_type action){
    DIR* dirp = opendir(path);

    if(dirp == NULL){
        perror("opendir for tasks tree reading");
        return -1;
    }

    int result = 0;
    struct dirent* entry;
    bool is_task_found = false;

    //Finding the task
    while ((entry=readdir(dirp))) {
        dprintf(STDERR_FILENO, "DEBUG: task_finder sees entry '%s'\n", entry->d_name);
        if(entry -> d_name[0] == '.') continue;

        if(strcmp(entry -> d_name, task_id) == 0){
            //Construction of the path to the task
            char* newpath = make_path(path, entry -> d_name);
            if(newpath == NULL){
                result = -1;
                goto error;
            }
            result = extract_task_information(newpath, action);
            
            free(newpath);
            newpath = NULL;
            is_task_found = true;
            break;
        }
    }
    if(!is_task_found){
        dprintf(STDERR_FILENO, "The task with the id %s couldn't be find :/", task_id);
        return -1;
    }
    error:
    if(closedir(dirp) != 0){
        perror("closedir");
        result = -1;
    }
    return result;
}

int extract_task_information(const char* path, Action_type action){
   
    int result = 0;
    //Calling the appropriate reader depending on the action
    switch (action){
        case LIST:
            if(aux_extract_cmd(path) != 0){
                result = -1;
            }
            if(aux_extract_time(path, "timing") < 0){
                dprintf(STDERR_FILENO, "Error while reading timing file of task at path %s\n", path);
                result = -1;
            }
            break;
        case TIME_EXIT:
            if(aux_extract_time(path, "times-exitcodes") < 0){
                dprintf(STDERR_FILENO, "Error while reading times_exitcodes file of task at path %s\n", path);
                result = -1;
            }break;
        case OUTPUT:
            if(aux_extract_output(path, "stdout", output_reader, false) < 0){
                dprintf(STDERR_FILENO, "Error while reading stdout file of task at path %s\n", path);
                result = -1;
            }break;
        case ERR:
            if(aux_extract_output(path, "stderr", output_reader, true) < 0){
                dprintf(STDERR_FILENO, "Error while reading stderr file of task at path %s\n", path);
                result = -1;
            }
        default:
            break;
    }
    return result;
}

int aux_extract_cmd(const char* path){
    int result = 0;
    //Construction of the path to the file
    char* new_path = make_path(path, "cmd");
    if(new_path == NULL){
        result = -1;
        goto error;
    }
    if(cmd_reader(new_path) == -1){
        result = -1;
        goto error;
    }
    error:
    free(new_path);
    new_path = NULL;
    return result;
}

int aux_extract_output(const char* path, char* folder_name, int (*func)(const char*, bool), bool is_stderr){
    int result = 0;
    //Construction of the path to the file
    char* new_path = make_path(path, folder_name);
    if(new_path == NULL){
        dprintf(STDERR_FILENO, "The task hasn't been executed yet\n");
        result = -1;
        goto error;
    }
    if(func(new_path, is_stderr) == -1){
        result = -1;
        goto error;
    }
    error:
    free(new_path);
    new_path = NULL;
    return result;
}

int aux_extract_time(const char* path, char* folder_name){
    int result = 0;
    //Construction of the path to the file
    char* new_path = make_path(path, folder_name);
    if(new_path == NULL){
        result = -1;
        goto error;
    }
    if(strcmp(folder_name, "timing") == 0){
        if(timing_reader(new_path, timing_interpreter) == -1){
            result = -1;
            goto error;
        }
    }else{
        if(timing_reader(new_path, times_exitcodes_interpreter) == -1){
            result = -1;
            goto error;
        }
    }
    error:
    free(new_path);
    new_path = NULL;
    return result;
}

int buffer_init(char** buffer) {
    *buffer = malloc(BUFFER_SIZE);
    if (*buffer == NULL) {
        perror("malloc");
        return -1;
    }
    return 0;
}

int folder_exist(const char* path){
    if(access(path, F_OK) != 0){
        dprintf(STDERR_FILENO, "the file doesn't exist at path : %s\n", path);
        return 1;
    }
    return 0;
}

char* make_path(const char* og_path, const char* folder_name){
    char* pathcpy = malloc(MAX_PATH);

    if(pathcpy == NULL){
        perror("malloc");
        return NULL;
    }
    snprintf(pathcpy, MAX_PATH, "%s/%s", og_path, folder_name); //Concatenation of og_path and folder_name
    if(folder_exist(pathcpy) == 1){
        free(pathcpy);
        return NULL;
    }
    return pathcpy;
}
char* make_path_no_test(const char* og_path, const char* folder_name){
    char* pathcpy = malloc(MAX_PATH);

    if(pathcpy == NULL){
        perror("malloc");
        return NULL;
    }
    snprintf(pathcpy, MAX_PATH, "%s/%s", og_path, folder_name); //Concatenation of og_path and folder_name

    return pathcpy;
}