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

extern char tasksdir[PATH_MAX];

uint64_t find_next_id()
{
    // Use of the global tasksdir variable
    DIR *dir = opendir(tasksdir);
    if (!dir)
    {
        return 0;
    }

    uint64_t max_id = 0;
    int found = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        char *endptr;
        uint64_t id = strtoull(entry->d_name, &endptr, 10);
        if (*endptr == '\0' && entry->d_name[0] != '\0')
        { // Ensure the entire name was a number
            if (id > max_id || !found)
            {
                max_id = id;
                found = 1;
            }
        }
    }
    closedir(dir);
    
    if (found)
    {
        return max_id + 1;
    }
    else
    {
        return 0;
    }
}

int write_timing_file(const char *path, const timing_t *t)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return -1;

    uint64_t min_be = htobe64(t->minutes);
    uint32_t hrs_be = htobe32(t->hours);

    if (write(fd, &min_be, 8) != 8)
    {
        close(fd);
        return -1;
    }
    if (write(fd, &hrs_be, 4) != 4)
    {
        close(fd);
        return -1;
    }
    if (write(fd, &(t->daysofweek), 1) != 1)
    {
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int write_command_simple(const char *cmd_dir_path, const arguments_t *args)
{
    if (mkdir(cmd_dir_path, 0755) < 0)
        return -1;

    char sub_path[PATH_MAX];

    // 1. Create cmd/type file (uint16, 'SI' = 0x5349)
    snprintf(sub_path, sizeof(sub_path), "%s/type", cmd_dir_path);
    int fd_type = open(sub_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_type < 0)
        return -1;

    uint16_t type_be = htobe16(0x5349);
    write(fd_type, &type_be, 2);
    close(fd_type);

    // 2. Create cmd/argv file
    snprintf(sub_path, sizeof(sub_path), "%s/argv", cmd_dir_path);
    int fd_argv = open(sub_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_argv < 0)
        return -1;

    uint32_t argc_be = htobe32(args->argc);
    write(fd_argv, &argc_be, 4);

    for (uint32_t i = 0; i < args->argc; i++)
    {
        string_t *s = args->argv[i];
        if (!s)
            continue;
        uint32_t len_be = htobe32(s->length);
        write(fd_argv, &len_be, 4);
        write(fd_argv, s->data, s->length);
    }
    close(fd_argv);
    return 0;
}

int create_task_dir(const timing_t *timing, const arguments_t *args)
{
    if (tasksdir[0] == '\0')
        return -1; // Ensure config is initialized

    // Ensure tasks directory exists
    mkdir(tasksdir, 0755);

    // Find next ID
    uint64_t task_id = find_next_id();

    // Create task directory: tasksdir/ID
    char task_path[PATH_MAX];
    snprintf(task_path, sizeof(task_path), "%s/%lu", tasksdir, task_id);
    if (mkdir(task_path, 0755) < 0)
        return -1;

    // Create internal files
    char sub_path[PATH_MAX];

    // timing file
    snprintf(sub_path, sizeof(sub_path), "%s/timing", task_path);
    write_timing_file(sub_path, timing);

    // cmd directory
    snprintf(sub_path, sizeof(sub_path), "%s/cmd", task_path);
    write_command_simple_dir(sub_path, args);

    // Initial empty files for logs as per specification
    snprintf(sub_path, sizeof(sub_path), "%s/times-exitcodes", task_path);
    close(open(sub_path, O_WRONLY | O_CREAT | O_TRUNC, 0644));

    return (int)task_id;
}