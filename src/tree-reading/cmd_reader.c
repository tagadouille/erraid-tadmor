#define _GNU_SOURCE

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdint.h>

#include "tree-reading/tree_reader.h"
#include "tree-reading/cmd_reader.h"
#include "types/argument.h"
#include "types/task.h"
#include "erraids/erraid.h"

int cmd_reader(const char *path){

    int result = 0;

    char *type_path = make_path(path, "type");

    if (type_path == NULL){
        result = -1;
        goto error;
    }

    // Trying to acces to the type file
    command_type_t type = type_reader(type_path);

    if (type == INVALID){
        dprintf(STDERR_FILENO, "Error while reading type file of task at path %s\n", path);
        result = -1;
        goto error;
    }
    // If the type is SI, construct the task by reading the argv file
    if (type == SI){
        curr_task->cmd = type_processor(path, SI, curr_task->cmd, NULL);

        if(curr_task->cmd == NULL){
            result = -1;
            goto error;
        }
    } // Else command_parser is used
    else{
        command_t *cmd = create_command(type);

        if(cmd == NULL){
            result = -1;
            goto error;
        }
        cmd = command_parser(path, cmd);

        if(cmd == NULL){
            result = -1;
            goto error;
        }
        curr_task->cmd = cmd;
    }

    error:
    if (result == -1){
        dprintf(STDERR_FILENO, "Error while reading cmd folder of task at path %s\n", path);
    }

    free(type_path);
    type_path = NULL;
    return result;
}

/* Comparator for qsort*/
static int numeric_cmp(const void *a, const void *b) {
    const char * const *pa = a;
    const char * const *pb = b;

    if (pa == NULL || pb == NULL) return 0;

    errno = 0;
    char *end;

    long long na = strtoll(*pa, &end, 10);
    if (*end != '\0' || errno == ERANGE) {
        na = 0;
        errno = 0;
    }

    errno = 0;
    long long nb = strtoll(*pb, &end, 10);
    if(*end != '\0' || errno == ERANGE){
        nb = 0;
        errno = 0;
    }

    if(na < nb){
        return -1;
    }
    if(na > nb){
        return 1;
    }
    return 0;
}

/* Helper : free an array of strings (count elements may be less than capacity) */
static void free_str_array(char **arr, size_t count) {
    if (arr == NULL){
        return;
    }
    for (size_t i = 0; i < count; ++i) {
        free(arr[i]);
    }
    free(arr);
}

/* Helper : check if entry is directory, using d_type with fallback to stat */
static int is_directory_entry(const char *dirpath, const struct dirent *entry) {

    #if defined(DT_UNKNOWN)
        if(entry->d_type != DT_UNKNOWN){
            return entry->d_type == DT_DIR;
        }
    #endif
    char fullpath[PATH_MAX];

    if(snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name) >= (int)sizeof(fullpath)){
        return 0;
    }

    struct stat st;
    if(stat(fullpath, &st) != 0){
        return 0;
    }
    if (S_ISLNK(st.st_mode)){
        return 0;
    }

    return S_ISDIR(st.st_mode);
}

command_t *command_parser(const char *path, command_t *cmd) {
    if (path == NULL){
        return NULL;
    }

    //Initialization
    char *argv_path = NULL;
    char *curr_type_path = NULL;
    DIR *dirp = NULL;
    char **subdirs = NULL;
    size_t cap = 0, count = 0;
    command_t *result = NULL;

    //Build paths
    argv_path = make_path_no_test(path, "argv");
    if(argv_path == NULL){
        goto error;
    }

    curr_type_path = make_path(path, "type");
    if(curr_type_path == NULL){
        goto error;
    }

    //If this node has an argv file, it's a leaf
    if (access(argv_path, F_OK) == 0) {
        command_t *leaf = argv_reader(path, cmd);

        if (leaf == NULL) {
            dprintf(STDERR_FILENO, "Error while reading argv file of task at path %s\n", path);
            goto error;
        }
        free(argv_path); argv_path = NULL;
        free(curr_type_path); curr_type_path = NULL;
        return leaf;
    }

    //Not a leaf
    free(argv_path); 
    argv_path = NULL;

    dirp = opendir(path);
    if (dirp == NULL) {
        perror("opendir for command tree parsing");
        goto error;
    }

    //Read current type
    command_type_t type = type_reader(curr_type_path);
    free(curr_type_path); curr_type_path = NULL;
    if (type == INVALID) {
        dprintf(STDERR_FILENO, "Error while reading type file of task at path %s\n", path);
        goto error;
    }

    //Prepare dynamic array for subdirectories (only numeric names)
    cap = 8;
    subdirs = malloc(cap * sizeof(char*));

    if(subdirs == NULL){
        perror("malloc subdirs");
        goto error;
    }
    count = 0;

    struct dirent *entry;
    while ((entry = readdir(dirp)) != NULL) {

        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if(!is_directory_entry(path, entry)) continue;

        /* ensure name is numeric */
        char *endptr = NULL;
        errno = 0;
        strtol(entry->d_name, &endptr, 10);

        if(errno == ERANGE || *endptr != '\0'){
            continue;
        }

        //add to array
        if(count == cap){
            size_t newcap = cap * 2;
            char **tmp = realloc(subdirs, newcap * sizeof(char*));

            if(tmp == NULL){
                perror("realloc subdirs");
                goto error;
            }
            subdirs = tmp;
            cap = newcap;
        }

        subdirs[count] = strdup(entry->d_name);
        if(subdirs[count] == NULL){
            perror("strdup");
            goto error;
        }
        ++count;
    }

    if(closedir(dirp) != 0){
        perror("closedir");
        dirp = NULL;
        goto error;
    }
    dirp = NULL;

    //Sort numerically
    if(count > 1){
        qsort(subdirs, count, sizeof(char*), numeric_cmp);
    }

    //ERROR: a node without argv must have at least one numeric subdirectory
    if (count == 0) {
        dprintf(STDERR_FILENO,"Error: directory %s has no argv file and no numeric subdirectories\n",path);
        goto error;
    }

    //iterate children in sorted order
    for (size_t i = 0; i < count; ++i) {
        char *sub_path = NULL;
        char *type_path = NULL;
        command_t *son_cmd = NULL;

        sub_path = make_path(path, subdirs[i]);
        if (sub_path == NULL) {
            dprintf(STDERR_FILENO, "make_path failed for %s/%s\n", path, subdirs[i]);
            goto child_error;
        }

        type_path = make_path(sub_path, "type");
        if(type_path == NULL){
            dprintf(STDERR_FILENO, "make_path failed for %s/type\n", sub_path);
            goto child_error;
        }

        //Creation of the son command
        command_type_t son_type = type_reader(type_path);
        if(son_type == INVALID){
            dprintf(STDERR_FILENO, "Error while reading type file of task at path %s\n", sub_path);
            goto child_error;
        }

        son_cmd = create_command(son_type);
        if(son_cmd == NULL){
            dprintf(STDERR_FILENO, "create_command failed for path %s\n", sub_path);
            goto child_error;
        }

        son_cmd = command_parser(sub_path, son_cmd);
        if(son_cmd == NULL){
            dprintf(STDERR_FILENO, "command_parser failed for child %s\n", sub_path);
            goto child_error;
        }

        cmd = type_processor(path, type, cmd, son_cmd);
        if (cmd == NULL) {
            dprintf(STDERR_FILENO, "type_processor failed for path %s\n", path);
            command_free(son_cmd);
            goto child_error;
        }

        free(type_path); type_path = NULL;
        free(sub_path); sub_path = NULL;
        continue;

    child_error:
        if(type_path){ 
            free(type_path);
            type_path = NULL;
        }
        if(sub_path){
            free(sub_path);
            sub_path = NULL;
        }
        if(son_cmd){
            command_free(son_cmd);
            son_cmd = NULL;
        }

        goto error;
    }
    free_str_array(subdirs, count);
    subdirs = NULL;

    result = cmd;
    return result;

    error:
    if(dirp){
        closedir(dirp);
        dirp = NULL;
    }
    free_str_array(subdirs, count);
    subdirs = NULL;

    if(curr_type_path){ 
        free(curr_type_path);
         curr_type_path = NULL;
    }
    if(argv_path){
        free(argv_path);
        argv_path = NULL;
    }

    dprintf(STDERR_FILENO, "Error while parsing command tree at path %s\n", path);

    command_free(cmd);

    return NULL;
}


command_t *argv_reader(const char *path, command_t *og_command){

    char *buffer = NULL;
    int fd = -1;

    if(buffer_init(&buffer) < 0){
        command_free(og_command);
        return NULL;
    }

    // Construction of the path to the argv file
    char *argv_path = make_path(path, "argv");
    if (argv_path == NULL){
        command_free(og_command);
        goto error;
    }

    // Reading the type file
    fd = open(argv_path, O_RDONLY);

    if(fd < 0){
        perror("open argv file");
        command_free(og_command);
        goto error;
    }
    free(argv_path);
    argv_path = NULL;

    // Reading the argv file and extracting the information
    ssize_t buffer_size = BUFFER_SIZE;
    unsigned int buf_ptr = 0; // Pointer to the current position in the buffer
    unsigned int filesize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET); // Resetting the file descriptor to the beginning of the file

    while (buf_ptr < filesize){

        // Reallocation of the buffer if needed
        if(buf_ptr == buffer_size){
            buffer_size *= 2;
            char *new_buffer = realloc(buffer, buffer_size);

            if(new_buffer == NULL){
                perror("realloc");
                command_free(og_command);
                goto error;
            }
            buffer = new_buffer;
        }
        // Reading from the file
        ssize_t nread = read(fd, buffer + buf_ptr, buffer_size - buf_ptr);

        if(nread < 0){
            perror("nread");
            command_free(og_command);
            goto error;
        }

        else if(nread == 0){
            break;
        }

        else{
            buf_ptr += nread;
        }
    }

    og_command = command_filler(buffer, buf_ptr, og_command, SI);

    if(og_command == NULL){
        dprintf(STDERR_FILENO, "Error while filling the command structure from argv file at path %s\n", path);
        goto error;
    }

    error:
    if(buffer != NULL){
        free(buffer);
        buffer = NULL;
    }

    if(fd != -1 && close(fd) != 0){
        perror("close");
        command_free(og_command);
    }
    return og_command;
}

command_type_t type_reader(const char *path){

    char *buffer = NULL;
    int result = INVALID;
    int fd = -1;

    if(buffer_init(&buffer) < 0){
        return INVALID;
    }
    // Reading the type file
    fd = open(path, O_RDONLY);

    if(fd < 0){
        perror("open type file");
        goto error;
    }
    ssize_t nread = read(fd, buffer, BUFFER_SIZE);

    if (nread < 0){
        perror("nread");
        goto error;
    }
    else{
        // Detection of an anomaly
        if(nread == 2){
            buffer[nread] = '\0';

            command_type_t type = type_interpreter(buffer);

            if (type == INVALID)
            {
                goto error;
            }
            result = type;
        }
        else{
            dprintf(STDERR_FILENO, "Error : type file has an invalid size\n");
            goto error;
        }
    }
    error:
    if (buffer != NULL){
        free(buffer);
    }

    buffer = NULL;

    if (fd != -1 && close(fd) != 0){
        perror("close");
        result = INVALID;
    }
    return result;
}

// TODO adapt the functions according to the different types
command_type_t type_interpreter(char *buffer){

    if(strcmp(buffer, "SI") == 0){
        return SI;
    }
    else if(strcmp(buffer, "SQ") == 0){
        return SQ;
    }
    else if(strcmp(buffer, "IF") == 0){
        return IF;
    }
    else if(strcmp(buffer, "PL") == 0){
        return PL;
    }
    else{
        dprintf(STDERR_FILENO, "Unknown command type : %s\n", buffer);
        return INVALID;
    }
}

command_t *type_processor(const char *path, command_type_t type, command_t *og_command, command_t *cmd){

    if(type == SI){
        og_command = argv_reader(path, og_command);
        return og_command;

    } else if(type == SQ || type == IF || type == PL){
        og_command = add_complex_command(og_command, cmd);

        if (og_command == NULL) {
            dprintf(STDERR_FILENO, "Error while adding command to composed command at path %s\n", path);
            goto error;
        }
        return og_command;

    }else{
        dprintf(STDERR_FILENO, "Error : invalid command type");
        goto error;
    }
    error:
    if(og_command != NULL){
        command_free(og_command);
        og_command = NULL;
    }

    if(cmd != NULL){
        command_free(cmd);
        cmd = NULL;
    }
    return NULL;
}