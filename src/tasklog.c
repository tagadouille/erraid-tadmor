#define _POSIX_C_SOURCE 200809L
#include "tasklog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h> 
#include <limits.h>

// Global variable for run directory
static char *g_run_dir = NULL;

// Sets the root directory of the tasks tree. 
int tasklog_set_rundir(const char *run_dir) { 
    if (!run_dir) {
        return -1; 
    } 
    char *new = strdup(run_dir); 
    if (!new) return -1; 
    free(g_run_dir); 
    g_run_dir = new; 
    return 0; 
}

// Helper: build full path to a task file
static int build_task_path(char *out, size_t outlen, int taskid, const char *fname) {
    const char *base = g_run_dir ? g_run_dir : ".";
    int ret = snprintf(out, outlen, "%s/tasks/%d/%s", base, taskid, fname);
    if (ret < 0 || (size_t)ret >= outlen) { return -1; }
    return 0;
}

// Helper: format time in "YYYY-MM-DD HH:MM:SS"
static int format_time_rfc(time_t t, char *buf, size_t buflen) {
    struct tm tm;
    if (!localtime_r(&t, &tm)) return -1;
    if (strftime(buf, buflen, "%Y-%m-%d %H:%M:%S", &tm) == 0) return -1;
    return 0;
}

// Adds a line "YYYY-MM-DD HH:MM:SS EXIT_CODE\n" to tasks/<id>/times-exitcodes
int log_add_execution(uint64_t taskid, time_t when, int exit_code) {
    char path[PATH_MAX];
    if (build_task_path(path, sizeof(path), taskid, "times-exitcodes") != 0) return -1;

    // Ensure task dir exists
    char taskdir[PATH_MAX];
    const char *base = g_run_dir ? g_run_dir : ".";
    if (snprintf(taskdir, sizeof(taskdir), "%s/tasks/%ld", base, taskid) < 0) { return -1; }

    // Open file for appending
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return -1; 

    char timestr[34];
    if (format_time_rfc(when, timestr, sizeof(timestr)) != 0) {
        close(fd);
        return -1;
    }

    // Prepare line to write
    char line[sizeof(timestr) + sizeof(int) + 3]; // space, newline, null
    int n = snprintf(line, sizeof(line), "%s %d\n", timestr, exit_code);
    if (n < 0) { close(fd); errno = EIO; return -1; }

    // Write line
    ssize_t w = write(fd, line, (size_t)n);
    if (w < 0 || w != n) {
        int saved = errno;
        close(fd);
        errno = saved;
        return -1;
    }

    close(fd);
    return 0;
}

// Helper: atomic replace of a file
static int atomic_replace_file(uint64_t taskid, const char *fname, const char *buf, size_t len) {
    char taskdir[PATH_MAX];
    const char *base = g_run_dir ? g_run_dir : ".";
    if (snprintf(taskdir, sizeof(taskdir), "%s/tasks/%ld", base, taskid) < 0) { return -1; }

    // Create temporary file 
    char tmp_template[PATH_MAX];
    if (snprintf(tmp_template, sizeof(tmp_template), "%s/.%s.tmp.XXXXXX", taskdir, fname) < 0) { return -1; }

    int tmpfd = mkstemp(tmp_template);
    if (tmpfd < 0) return -1;

    // Write content to temporary file
    size_t written = 0;
    while (written < len) {
        ssize_t w = write(tmpfd, buf + written, len - written);
        if (w < 0) {
            int saved = errno;
            close(tmpfd);
            unlink(tmp_template);
            errno = saved;
            return -1;
        }
        written += (size_t)w;
    }

    close(tmpfd);

    // Build destination path
    char dest[PATH_MAX];
    if (snprintf(dest, sizeof(dest), "%s/%s", taskdir, fname) < 0) { unlink(tmp_template); return -1; }

    // Rename temporary file to destination
    if (rename(tmp_template, dest) != 0) {
        int saved = errno;
        unlink(tmp_template);
        errno = saved;
        return -1;
    }

    return 0;
}

int log_write_stdout(uint64_t taskid, const char *buf, size_t len) {
    if (!buf && len != 0) { return -1; }
    return atomic_replace_file(taskid, "stdout", buf ? buf : "", len);
}

int log_write_stderr(uint64_t taskid, const char *buf, size_t len) {
    if (!buf && len != 0) { return -1; }
    return atomic_replace_file(taskid, "stderr", buf ? buf : "", len);
}
