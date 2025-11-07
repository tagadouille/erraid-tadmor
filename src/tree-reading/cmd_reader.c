#include <dirent.h>
#include <string.h>

int cmd_reader(char* path){
    DIR* dirp = opendir(path);

    if(dirp == NULL){
        perror("opendir");
        return 1;
    }
    struct dirent* entry;
    size_t result = 0;

    //Finding all the file and extracting some informations
    while ((entry=readdir(dirp))){
        if(entry -> d_name[0] == '.') continue;
        
        if(strcmp(entry -> d_name, "argv") == 0){
            argv_reader(/*TODO put the path*/);
        }
        else if (strcmp(entry -> d_name, "type") == 0){
            type_reader(/*TODO put the path*/);
        }
        
    }
    if(closedir(dirp) != 0){
        perror("closedir");
        return 1;
    }
    return result;
}

int argv_reader(char* path){
    //TODO
    return 0;
}

int type_reader(char* path){
    //TODO
    return 0;
}