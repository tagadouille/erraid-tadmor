#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include "tree-writer/task_creator.h"
#include "types/my_string.h"
#include "erraids/erraid.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <endian.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>

/**
 * @brief Find the next id not already used.
 * @return new uint64_t id or 0 if no tasks.
 */
uint64_t find_next_id() {
    // Use of the global tasksdir variable
    DIR *dir = opendir(tasksdir);
    if (!dir) {
        dprintf(STDERR_FILENO, "[find_next_id] Error: could not open tasks directory '%s'\n", tasksdir);
        return 0;
    }

    uint64_t max_id = 0;
    int found = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char *endptr;
        uint64_t id = strtoull(entry->d_name, &endptr, 10);
        if (*endptr == '\0' && entry->d_name[0] != '\0') {
            
            // Ensure the entire name was a number
            if (id > max_id || !found) {
                max_id = id;
                found = 1;
            }
        }
    }
    closedir(dir);
    
    if (found) {
        return max_id + 1;
    }
    else {
        dprintf(STDERR_FILENO, "[find_next_id] Warning: no valid task IDs found in '%s', starting from 1\n", tasksdir);
        return 0;
    }
}

int write_timing_file(const char *path, const timing_t *t) {
    
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0){
        perror("open");
        return -1;
    }  

    uint64_t min_be = htobe64(t->minutes);
    uint32_t hrs_be = htobe32(t->hours);

    if (write(fd, &min_be, 8) != 8) {
        perror("write minutes");
        close(fd);
        return -1;
    }

    if (write(fd, &hrs_be, 4) != 4) {
        perror("write hours");
        close(fd);
        return -1;
    }

    if (write(fd, &(t->daysofweek), 1) != 1) {
        perror("write daysofweek");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/**
 * @brief Write a simple task command (SI) to a file descriptor.
 * @param cmd_dir_path path for command.
 * @param args arguments of the command.
 * @return 0 on success, -1 on failure.
 */
int write_command_simple(const char *cmd_dir_path, const arguments_t *args) {
    if (mkdir(cmd_dir_path, 0755) < 0)
        return -1;

    char sub_path[PATH_MAX];
    int len;

    // 1. Create cmd/type file (uint16, 'SI' = 0x5349)
    len = snprintf(sub_path, sizeof(sub_path), "%s/type", cmd_dir_path);
    if (len < 0 || (size_t)len >= sizeof(sub_path)) {
        dprintf(STDERR_FILENO, "Error: path too long for 'type' file.\n");
        return -1;
    }

    int fd_type = open(sub_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd_type < 0){
        perror("open type file");
        return -1;
    }

    dprintf(1, "Writing command type 'SI' to %s\n", sub_path);

    uint16_t type_be = htobe16(0x5349);
    if(write(fd_type, &type_be, 2) != 2){
        perror("write type");
        close(fd_type);
        return -1;
    }
    close(fd_type);

    // 2. Create cmd/argv file
    len = snprintf(sub_path, sizeof(sub_path), "%s/argv", cmd_dir_path);
    if (len < 0 || (size_t)len >= sizeof(sub_path)) {
        dprintf(STDERR_FILENO, "Error: path too long for 'argv' file.\n");
        return -1;
    }

    int fd_argv = open(sub_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_argv < 0){
        perror("open argv file");
        return -1;
    }

    dprintf(1,"Writing command arguments to %s\n", sub_path);

    uint32_t argc_be = htobe32(args->argc);
    if(write(fd_argv, &argc_be, 4) != 4){
        perror("write argc");
        close(fd_argv);
        return -1;
    }

    dprintf(1, "Number of arguments: %u\n", args->argc);

    for (uint32_t i = 0; i < args->argc; i++)
    {
        string_t *s = args->argv[i];
        if (!s)
            continue;
        uint32_t len_be = htobe32(s->length);

        if(write(fd_argv, &len_be, 4) != 4){
            perror("write argument length");
            close(fd_argv);
            return -1;
        }

        if(write(fd_argv, s->data, s->length) != (ssize_t)s->length){
            perror("write argument data");
            close(fd_argv);
            return -1;
        }
    }
    close(fd_argv);
    return 0;
}

int64_t create_task_dir(const timing_t *timing, const arguments_t *args) {

    if (tasksdir[0] == '\0'){
        dprintf(STDERR_FILENO, "Error: tasksdir is not set.\n");
        return -1;
    }

    // Ensure tasks directory exists
    if(mkdir(tasksdir, 0755) < 0){
        if (errno != EEXIST){
            dprintf(STDERR_FILENO, "Error: could not create tasks directory '%s'\n", tasksdir);
            return -1;
        }
    }

    // Find next ID
    uint64_t task_id = find_next_id();

    // Create task directory: tasksdir/ID
    char task_path[PATH_MAX];
    int len;

    len = snprintf(task_path, sizeof(task_path), "%s/%lu", tasksdir, task_id);
    if (len < 0 || (size_t)len >= sizeof(task_path)) {
        dprintf(STDERR_FILENO, "Error: path too long for task directory.\n");
        return -1;
    }

    if (mkdir(task_path, 0755) < 0){
        perror("mkdir task directory");
        return -1;
    }

    // Create internal files
    char sub_path[PATH_MAX];

    // timing file
    len = snprintf(sub_path, sizeof(sub_path), "%s/timing", task_path);
    if (len < 0 || (size_t)len >= sizeof(sub_path)) {
        dprintf(STDERR_FILENO, "Error: path too long for 'timing' file.\n");
        return -1;
    }
    
    if(write_timing_file(sub_path, timing) < 0){
        dprintf(STDERR_FILENO, "Error: could not write timing file.\n");
        return -1;
    }

    // cmd directory
    len = snprintf(sub_path, sizeof(sub_path), "%s/cmd", task_path);
    if (len < 0 || (size_t)len >= sizeof(sub_path)) {
        dprintf(STDERR_FILENO, "Error: path too long for 'cmd' directory.\n");
        return -1;
    }
    if(write_command_simple(sub_path, args) < 0){
        dprintf(STDERR_FILENO, "Error: could not write command files.\n");
        return -1;
    }

    return (int64_t) task_id;
}