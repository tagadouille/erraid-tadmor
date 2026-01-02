#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include "tree-writer/task_creator.h"
#include "communication/request.h"
#include "types/timing.h"
#include "erraids/erraid.h"
#include "tree-writer/task_combinator.h"
#include "tree-writer/task_annihilator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <endian.h>


int64_t combine_and_destroy_tasks(const timing_t *timing, const composed_t *composed) {

    if (!timing || !composed || composed->nb_task == 0) {
        dprintf(STDERR_FILENO, "Error: Invalid arguments for task combination.\n");
        return -1;
    }

    if(composed->type != SQ && composed->type != PL && composed->type != IF) {
        dprintf(STDERR_FILENO, "Error: Unsupported combination type.\n");
        return -1;
    }

    if(composed->type == IF && (composed->nb_task < 2 || composed->nb_task > 3)) {
        dprintf(STDERR_FILENO, "Error: IF combination requires exactly 2-3 tasks.\n");
        return -1;
    }

    // 1. Find a new ID and create the task directory :
    uint64_t new_task_id = find_next_id(tasksdir);

    char task_path[PATH_MAX];
    int written = snprintf(task_path, sizeof(task_path), "%s/%lu", tasksdir, new_task_id);
    if (written < 0 || (size_t)written >= sizeof(task_path)) {
        perror("snprintf task_path");
        return -1;
    }

    if (mkdir(task_path, 0777) < 0) {
        perror("mkdir task_path");
        return -1;
    }

    // 2. Write the timing file :
    char sub_path[PATH_MAX];
    written = snprintf(sub_path, sizeof(sub_path), "%s/timing", task_path);
    if (written < 0 || (size_t)written >= sizeof(sub_path)) {
        perror("snprintf sub_path for timing");
        return -1;
    }
    if (write_timing_file(sub_path, timing) < 0) {
        perror("write_timing");
        return -1;
    }

    // 3. Create the 'cmd' directory :
    char cmd_path[PATH_MAX];
    written = snprintf(cmd_path, sizeof(cmd_path), "%s/cmd", task_path);
    if (written < 0 || (size_t)written >= sizeof(cmd_path)) {
        perror("snprintf cmd_path");
        return -1;
    }
    if (mkdir(cmd_path, 0777) < 0) {
        perror("mkdir cmd_path");
        return -1;
    }

    // 4. Write the 'type' file :
    written = snprintf(sub_path, sizeof(sub_path), "%s/type", cmd_path);
    if (written < 0 || (size_t)written >= sizeof(sub_path)) {
        perror("snprintf sub_path for type");
        return -1;
    }

    int fd = open(sub_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if (fd < 0) {
        perror("open type file");
        return -1;
    }

    char type_str[2];
    switch (composed->type) {
        case SQ:
            type_str[0] = 'S';
            type_str[1] = 'Q';
            break;
        case PL:
            type_str[0] = 'P';
            type_str[1] = 'L';
            break;
        case IF:
            type_str[0] = 'I';
            type_str[1] = 'F';
            break;
        default:
            dprintf(STDERR_FILENO, "Error: Unknown combination type.\n");
            close(fd);
            return -1;
            break;
    }

    if (write(fd, type_str, 2) != 2) {
        perror("dprintf type");
        close(fd);
        return -1;
    }
    close(fd);

    // 5. Move the old tasks' cmd directories and delete the old tasks :
    for (uint32_t i = 0; i < composed->nb_task; i++) {

        uint64_t old_id = composed->task_ids[i];
        char old_task_path[PATH_MAX];
        char old_cmd_path[PATH_MAX];
        char new_cmd_subpath[PATH_MAX];

        written = snprintf(old_task_path, sizeof(old_task_path), "%s/%lu", tasksdir, old_id);
        if (written < 0 || (size_t)written >= sizeof(old_task_path)) {
            perror("snprintf old_task_path");
            return -1;
        }
        written = snprintf(old_cmd_path, sizeof(old_cmd_path), "%s/cmd", old_task_path);
        if (written < 0 || (size_t)written >= sizeof(old_cmd_path)) {
            perror("snprintf old_cmd_path");
            return -1;
        }
        written = snprintf(new_cmd_subpath, sizeof(new_cmd_subpath), "%s/%u", cmd_path, i);
        if (written < 0 || (size_t)written >= sizeof(new_cmd_subpath)) {
            perror("snprintf new_cmd_subpath");
            return -1;
        }

        // Move the old cmd directory :
        if (rename(old_cmd_path, new_cmd_subpath) < 0) {
            perror("rename old cmd directory");
            // We can restaure but it's complex, so we just clean up and return error
            delete_directory(task_path);
            return -1;
        }

        // Delete the old task :
        if (delete_directory(old_task_path) < 0) {
            dprintf(STDERR_FILENO, "Warning: Failed to delete old task directory %s\n", old_task_path);
            // Continue anyway because the main operation succeeded
        }
    }

    return (int64_t)new_task_id;
}