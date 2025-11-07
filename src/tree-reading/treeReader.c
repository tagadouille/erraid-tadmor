#include "limits.h"
#include "dirent.h"
#include "string.h"


int task_reader(const char* path, uint16 task_id){
    strcat(path, "tasks"); //construction of the path

    DIR* dirp = opendir(path);

    if(dirp == NULL){
        perror("opendir for tasks tree reading");
        return 1;
    }
    //Converting task_id to string :
    char id[UINT_MAX];
    sprintf(id, "%i", task_id);

    size_t result = 0;
    struct dirent* entry;

    //Finding the task
    while ((entry=readdir(dirp))) {
        if(entry -> d_name[0] == '.') continue;

        if(strcmp(entry -> d_name, id) == 0){
            result = extract_task_information(/*TODO put the path*/);
            break;
        }
    }

    error:
    close(dirp);
    return result;
}

int extract_task_information(const char* path){

}