#include "erraids/erraid.h"
#include "erraids/erraid-helper.h"
#include "communication/communication.h"

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <arpa/inet.h>

/* ------------------------------ GET RUNDIR ------------------------------ */

int erraid_get_rundir(char *out, size_t len) {
    if (!out || len == 0) { errno = EINVAL; return -1; }
    strncpy(out, g_run_dir, len-1);
    out[len-1] = '\0';
    return 0;
}

/* ---------------------------- SET RUNDIR ------------------------------- */

int erraid_set_rundir(const char *rundir, const char* pipedir) {

    // Verification
    if(pipedir == NULL){
        dprintf(STDERR_FILENO, "Error : The pipe directory can't be null\n");
        return -1;
    }
    if(rundir == NULL){
        dprintf(STDERR_FILENO, "Error : The rundir directory can't be null\n");
        return -1;
    }
    if (strlen(rundir) >= sizeof(g_run_dir)) {
        dprintf(STDERR_FILENO, "Error : The rundir directory is too big\n");
        errno = EINVAL;
        return -1;
    }
    if (strlen(pipedir) >= sizeof(pipe_path)) {
        dprintf(STDERR_FILENO, "Error : The pipepath directory is too big\n");
        errno = EINVAL;
        return -1;
    }
    // Copie of the pathes
    strncpy(g_run_dir, rundir, sizeof(g_run_dir)-1);
    g_run_dir[sizeof(g_run_dir)-1] = '\0';

    strncpy(pipe_path, pipedir, sizeof(pipe_path)-1);

    size_t len = strlen(pipe_path);

    if (len + strlen("/pipes") + 1 > sizeof(pipe_path))
        return -1;

    memcpy(pipe_path + len, "/pipes", strlen("/pipes") + 1);
    
    pipe_path[sizeof(pipe_path)-1] = '\0';

    dprintf(STDOUT_FILENO, "Pipe_path %s\n", pipe_path);

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

    if (!my_realpath(pipe_path, abs_pipe_path)) {
        perror("realpath");
        return -1;
    }
    strncpy(pipe_path, abs_pipe_path, sizeof(pipe_path)-1);
    pipe_path[sizeof(pipe_path)-1] = '\0';

    dprintf(STDOUT_FILENO, "Pipe_path %s\n", pipe_path);

    return 0;
}

int ensure_rundir(void) {
    if (g_run_dir[0] == '\0') return -1;
    
    if (mkdir_p(g_run_dir) != 0) return -1;

    //Use absolute path for g_run_dir
    char abs_rundir[PATH_MAX];

    if (!my_realpath(g_run_dir, abs_rundir)) {
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
//TODO refaire struct de cette fnct
/**
 * equivalent of realpath
 */
char *my_realpath(const char *path, char *resolved_path) {
    if (!path) { errno = EINVAL; return NULL; }

    char temp[PATH_MAX];
    if (path[0] != '/') {
        if (!getcwd(temp, sizeof(temp))) return NULL;
        size_t need = strlen(temp) + 1 + strlen(path) + 1;
        if (need > sizeof(temp)) { errno = ENAMETOOLONG; return NULL; }
        strcat(temp, "/");
        strcat(temp, path);
    } else {
        if (strlen(path) >= sizeof(temp)) { errno = ENAMETOOLONG; return NULL; }
        strncpy(temp, path, sizeof(temp));
        temp[sizeof(temp)-1] = '\0';
    }

    /* split and normalize */
    char *copy = strdup(temp);
    if (!copy) return NULL;

    char *components[PATH_MAX];
    int top = -1;

    char *saveptr = NULL;
    for (char *tok = strtok_r(copy, "/", &saveptr); tok; tok = strtok_r(NULL, "/", &saveptr)) {
        if (strcmp(tok, ".") == 0) continue;
        if (strcmp(tok, "..") == 0) {
            if (top >= 0) top--;
            continue;
        }
        components[++top] = tok;
    }

    /* build result */
    char result[PATH_MAX];
    if (top == -1) {
        /* root */
        strncpy(result, "/", sizeof(result));
        result[sizeof(result)-1] = '\0';
    } else {
        result[0] = '\0';
        for (int i = 0; i <= top; ++i) {
            size_t need = strlen(result) + 1 + strlen(components[i]) + 1;
            if (need > sizeof(result)) {
                free(copy);
                errno = ENAMETOOLONG;
                return NULL;
            }
            strcat(result, "/");
            strcat(result, components[i]);
        }
    }

    free(copy);

    if (resolved_path) {
        strncpy(resolved_path, result, PATH_MAX);
        resolved_path[PATH_MAX - 1] = '\0';
        return resolved_path;
    } else {
        return strdup(result);
    }
}

/* ------------------------------ CLEANUP -------------------------------- */

void daemon_cleanup(void) {
    write_log_msg("Cleanup.");

    if (g_log_fd >= 0) close(g_log_fd);
}