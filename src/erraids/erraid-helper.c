#include "erraids/erraid.h"
#include "erraids/erraid-helper.h"
#include "communication/communication.h"
#include "tree-reading/tree_reader.h"

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <signal.h>

/* ------------------------------ GET RUNDIR ------------------------------ */

int erraid_get_rundir(char *out, size_t len) {
    if (!out || len == 0) { errno = EINVAL; return -1; }
    strncpy(out, g_run_dir, len-1);
    out[len-1] = '\0';
    return 0;
}

/* ---------------------------- SET RUNDIR ------------------------------- */

int erraid_set_rundir(const char *rundir, const char* pipedir)
{
    if (!rundir || !pipedir) {
        errno = EINVAL;
        dprintf(STDERR_FILENO, "Error : rundir or pipedir is NULL\n");
        return -1;
    }

    if (strlen(rundir) >= sizeof(g_run_dir)) {
        dprintf(1, "Error : rundir path too long\n");
        errno = ENAMETOOLONG;
        return -1;
    }

    if (strlen(pipedir) >= sizeof(pipe_path)) {
        dprintf(1, "Error : pipedir path too long\n");
        errno = ENAMETOOLONG;
        return -1;
    }

    strncpy(g_run_dir, rundir, sizeof(g_run_dir) - 1);
    g_run_dir[sizeof(g_run_dir) - 1] = '\0';

    strncpy(pipe_path, pipedir, sizeof(pipe_path) - 1);
    pipe_path[sizeof(pipe_path) - 1] = '\0';

    return 0;
}

static int pipe_path_initialization(){

    if (pipe_path[0] == '\0') {
        const char *user = getenv("USER");
        if (!user) user = "nobody";
        snprintf(pipe_path, sizeof(pipe_path), "/tmp/%s/pipes", user);
    }

    if (pipe_path[0] == '\0') return -1;

    if (mkdir_p(pipe_path) != 0) return -1;

    //Use absolute path for pipe_path
    char abs_pipe_path[PATH_MAX];

    if (!realpath(pipe_path, abs_pipe_path)) {
        perror("realpath");
        return -1;
    }
    strncpy(pipe_path, abs_pipe_path, sizeof(pipe_path)-1);
    pipe_path[sizeof(pipe_path)-1] = '\0';

    return 0;
}

int ensure_rundir(void) {

    if (g_run_dir[0] == '\0'){
        dprintf(STDERR_FILENO, "Error : rundir not set\n");
        return -1;
    }
    
    if (mkdir_p(g_run_dir) != 0){
        dprintf(STDERR_FILENO, "Error : creating rundir failed\n");
        return -1;
    }

    //Use absolute path for g_run_dir
    char abs_rundir[PATH_MAX];

    if (!realpath(g_run_dir, abs_rundir)) {
        perror("realpath");
        return -1;
    }
    strncpy(g_run_dir, abs_rundir, sizeof(g_run_dir)-1);
    g_run_dir[sizeof(g_run_dir)-1] = '\0';

    size_t len = strlen(g_run_dir);

    if (len + strlen("/tasks")+1 >= sizeof(tasksdir)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    //Add a slash if necessary for g_run_dir
    if (g_run_dir[strlen(g_run_dir)-1] == '/'){
        snprintf(tasksdir, sizeof(tasksdir), "%stasks", g_run_dir);
    }
    else{
        snprintf(tasksdir, sizeof(tasksdir), "%s/tasks", g_run_dir);
    }

    if (mkdir_p(tasksdir) != 0) return -1;

    if(pipe_path_initialization() < 0){
        dprintf(STDERR_FILENO, "Error : pipe_path initialization failed\n");
        return -1;
    }

    return 0;
}

int mkdir_p(const char *path) {

    if (!path || *path == '\0') {
        errno = EINVAL;
        return -1;
    }

    char tmp[PATH_MAX];
    size_t len = strlen(path);

    if (len >= sizeof(tmp)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    strncpy(tmp, path, sizeof(tmp));
    tmp[sizeof(tmp)-1] = '\0';

    // delete the final slash
    while (len > 1 && tmp[len-1] == '/') {
        tmp[len-1] = '\0';
        len--;
    }

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }

    // create the last directory
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;

    return 0;
}

uint64_t hton64(uint64_t x) {
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        return ((uint64_t)htonl((uint32_t)(x & 0xffffffffULL)) << 32) | htonl((uint32_t)(x >> 32));
    #else
        return x;
    #endif
}

/* ------------------------------ CLEANUP -------------------------------- */

void daemon_cleanup(void) {
    write_log_msg("Cleanup.");

    if (g_log_fd >= 0) close(g_log_fd);

    raise(SIGKILL);
}