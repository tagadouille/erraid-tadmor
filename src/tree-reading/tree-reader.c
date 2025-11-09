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

int task_reader(const char* path, uint16_t task_id){

    //construction of the path
    char* pathcpy = make_path(path, "tasks");
    if(pathcpy == NULL){
        return -1;
    }
    //Converting task_id to string :
    char id[sizeof(uint16_t) + 1];
    sprintf(id, "%i", task_id);

    size_t result = task_finder(pathcpy, id);
    free(pathcpy);
    pathcpy = NULL;
    return result;
}

int all_tasks_reader(const char* path){
    printf("Reading all the tasks in the tree at path %s\n", path);
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
        result = extract_task_information(newpath, true);
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
int task_finder(char* path, char* task_id){
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
            printf("Task with id %s found at path %s\n", task_id, newpath);
            result = extract_task_information(newpath, false);
            is_task_found = true;
            break;
        }
    }
    if(!is_task_found){
        dprintf(STDERR_FILENO, "The task with the id %d couldn't be find :/", task_id);
        return -1;
    }
    error:
    if(closedir(dirp) != 0){
        perror("closedir");
        result = -1;
    }
    return result;
}

int extract_task_information(const char* path, bool is_sequence){
    printf("Extracting task information at path %s\n", path);
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
            if(cmd_reader(path) == -1){
                dprintf(STDERR_FILENO, "Error while reading cmd folder of task at path %s\n", path);
                result = -1;
                goto error;
            }
        }else{
            if(strcmp(entry -> d_name, "cmd") == 0){
                //Construction of the path to the cmd folder
                char* cmd_path = make_path(path, "cmd");
                if(cmd_path == NULL){
                    result = -1;
                    goto error;
                }
                if(cmd_reader(cmd_path) == -1){
                    dprintf(STDERR_FILENO, "Error while reading cmd folder of task at path %s\n", path);
                    result = -1;
                    goto error;
                }
                continue;
            }
            else if(strcmp(entry -> d_name, "times_exitcodes") == 0){
                //Construction of the path to the times_exitcodes file
                char* times_exitcodes_path = make_path(path, "times_exitcodes");
                if(times_exitcodes_path == NULL){
                    result = -1;
                    goto error;
                }
                if(times_exitcodes_reader(times_exitcodes_path) == -1){
                    dprintf(STDERR_FILENO, "Error while reading times_exitcodes file of task at path %s\n", path);
                    result = -1;
                    goto error;
                }
            }
            else if(strcmp(entry -> d_name, "timing") == 0){
                //Construction of the path to the timing file
                char* timing_path = make_path(path, "timing");
                if(timing_path == NULL){
                    result = -1;
                    goto error;
                }
                if(timing_reader(timing_path) == -1){
                    dprintf(STDERR_FILENO, "Error while reading timing file of task at path %s\n", path);
                    result = -1;
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