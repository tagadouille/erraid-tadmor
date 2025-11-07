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

int task_reader(const char* path, uint16_t task_id){
    //construction of the path
    char* pathcpy = malloc(pathconf("/", _PC_PATH_MAX)); // for getting maximum size of a linux path

    if(pathcpy == NULL){
        perror("malloc");
        return 1;
    }
    strcat(strcpy(pathcpy, path), "tasks");

    //Converting task_id to string :
    char id[UINT_MAX];
    sprintf(id, "%i", task_id);

    return task_finder(pathcpy, id);
}

int task_finder(char* path, char* task_id){
    DIR* dirp = opendir(path);

    if(dirp == NULL){
        perror("opendir for tasks tree reading");
        return 1;
    }

    size_t result = 0;
    struct dirent* entry;
    bool is_task_found = false;

    //Finding the task
    while ((entry=readdir(dirp))) {
        if(entry -> d_name[0] == '.') continue;

        if(strcmp(entry -> d_name, task_id) == 0){
            snprintf(path, pathconf("/", _PC_PATH_MAX), "%s/%s", task_id);
            result = extract_task_information(path);
            is_task_found = true;
            break;
        }
    }
    if(!is_task_found){
        dprintf(stderr, "The task with the id %d couldn't be find :/", task_id);
        return 1;
    }
    if(closedir(dirp) != 0){
        perror("closedir");
        result = 1;
    }
    return result;
}

int extract_task_information(const char* path){
    size_t result = 0;
    DIR* dirp = opendir(path);

    if(dirp == NULL){
        perror("opendir for tasks tree reading");
        return 1;
    }
    struct dirent* entry;

    //Finding all the file and extracting some informations
    while ((entry=readdir(dirp))){
        if(entry -> d_name[0] == '.') continue;

        if(strcmp(entry -> d_name, "cmd")){
            cmd_reader(/*TODO put the path*/);
            continue;
        }
        else if(strcmp(entry -> d_name, "times_exitcodes")){
            //TODO gather times exit information
        }
        else if(strcmp(entry -> d_name, "timing")){
            //TODO gather timing information
        }
    }
    if(closedir(dirp) != 0){
        perror("closedir");
        result = 1;
    }
    return result;
}