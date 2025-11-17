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

int task_reader(const char* path, uint16_t task_id, Action_type action){

    //construction of the path
    char* pathcpy = make_path(path, "tasks");
    if(pathcpy == NULL){
        return -1;
    }
    curr_task = task_create(task_id);

    if(curr_task == NULL){
        dprintf(STDERR_FILENO, "Error while creating task with id %u\n", task_id);
        return -1;
    }

    //Converting task_id to string :
    char id[6];
    snprintf(id, sizeof(id), "%u", task_id);

    int result = task_finder(pathcpy, id, action);

    //If an error occured while finding the task, we free curr_task
    if(result == -1){
        task_destroy(curr_task);
        curr_task = NULL;
    }
    free(pathcpy);
    pathcpy = NULL;
    return result;
}

int all_tasks_reader(const char* path, command_t* cmd){
    int result = 0;
    DIR* dirp = opendir(path);

    if(dirp == NULL){
        perror("opendir for tasks tree reading");
        return -1;
    }
    struct dirent* entry;

    //Finding all the tasks
    while ((entry=readdir(dirp))) {
        if(entry -> d_name[0] == '.') continue;
        if(strcmp(entry -> d_name, "type") == 0 || strcmp(entry -> d_name, "argv") == 0) continue;

        //Construction of the path to the task
        char* newpath = make_path(path, entry -> d_name);
        if(newpath == NULL){
            result = -1;
            goto error;
        }
        result = extract_task_information(newpath, LIST, cmd, true);
        free(newpath);
        newpath = NULL;
    }
    error:
    if(closedir(dirp) != 0){
        perror("closedir");
        result = -1;
    }
    return result;
}
int task_finder(char* path, char* task_id, Action_type action){
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
        if(entry -> d_name[0] == '.') continue;

        if(strcmp(entry -> d_name, task_id) == 0){
            //Construction of the path to the task
            char* newpath = make_path(path, entry -> d_name);
            if(newpath == NULL){
                result = -1;
                goto error;
            }
            result = extract_task_information(newpath, action, curr_task -> cmd, false);
            
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

int extract_task_information(const char* path, Action_type action, command_t* cmd, bool is_sequence){
    int result = 0;
    DIR* dirp = opendir(path);

    if(dirp == NULL){
        perror("opendir for tasks tree reading");
        return -1;
    }
    struct dirent* entry;

    //Finding all the file and extracting some informations
    while ((entry=readdir(dirp))){
        if(entry -> d_name[0] == '.') continue;

        //If the task is a sequence, we only use cmd_reader
        if(is_sequence){
            if(cmd_reader(path, cmd) == -1){
                result = -1;
                goto error;
            }
        }else{
            //Calling the appropriate reader depending on the action and the file name
            if(action == LIST && strcmp(entry -> d_name, "cmd") == 0){
                if(aux_extract(path, "cmd", cmd) != 0){
                    goto error;
                }
            }else if(action == LIST && strcmp(entry -> d_name, "timing") == 0){
                if(aux_extract_time(path, entry -> d_name) < 0){
                    dprintf(STDERR_FILENO, "Error while reading timing file of task at path %s\n", path);
                    goto error;
                }
            }else if(action == TIME_EXIT && strcmp(entry -> d_name, "times-exitcodes") == 0){
                if(aux_extract_time(path, entry -> d_name) < 0){
                    dprintf(STDERR_FILENO, "Error while reading times_exitcodes file of task at path %s\n", path);
                    goto error;
                }
            }
            else if(action == OUTPUT && strcmp(entry -> d_name, "stdout") == 0){
                if(aux_extract_output(path, "stdout", output_reader, false) < 0){
                    dprintf(STDERR_FILENO, "Error while reading stdout file of task at path %s\n", path);
                    goto error;
                }
            }else if(action == ERR && strcmp(entry -> d_name, "stderr") == 0){
                if(aux_extract_output(path, "stderr", output_reader, true) < 0){
                    dprintf(STDERR_FILENO, "Error while reading stderr file of task at path %s\n", path);
                    goto error;
                }
            }
        }
    }
    error:
    if(closedir(dirp) != 0){
        perror("closedir");
        result = -1;
    }
    return result;
}

int aux_extract(const char* path, char* folder_name, command_t* cmd){
    //Construction of the path to the file
    char* new_path = make_path(path, folder_name);
    if(new_path == NULL){
        return -1;
    }
    if(cmd_reader(new_path, cmd) == -1){
        return -1;
    }
    free(new_path);
    new_path = NULL;
    return 0;
}

int aux_extract_output(const char* path, char* folder_name, int (*func)(const char*, bool), bool is_stderr){
    //Construction of the path to the file
    char* new_path = make_path(path, folder_name);
    if(new_path == NULL){
        return -1;
    }
    if(func(new_path, is_stderr) == -1){
        return -1;
    }
    free(new_path);
    new_path = NULL;
    return 0;
}

int aux_extract_time(const char* path, char* folder_name){
    //Construction of the path to the file
    char* new_path = make_path(path, folder_name);
    if(new_path == NULL){
        return -1;
    }
    if(strcmp(folder_name, "timing") == 0){
        if(timing_reader(new_path, timing_interpreter) == -1){
            return -1;
        }
    }else{
        if(timing_reader(new_path, times_exitcodes_interpreter) == -1){
            return -1;
        }
    }
    free(new_path);
    new_path = NULL;
    return 0;
}

int buffer_init(char** buffer) {
    *buffer = malloc(BUFFER_SIZE);
    if (*buffer == NULL) {
        perror("malloc");
        return -1;
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
    return pathcpy;
}