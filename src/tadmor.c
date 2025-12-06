#define _POSIX_C_SOURCE 200809L

#include "tadmor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

static char g_run_dir[PATH_MAX] = {0}; 
static char path_in[PATH_MAX];
static char path_out[PATH_MAX];

static int fd_in = -1;   // client -> daemon
static int fd_out = -1;  // daemon -> client

// ? Ces méthodes rundir sont-elles correctes?
/* --------------------------- RUNDIR --------------------------- */

int client_set_rundir(const char *rundir)
{
    // Check if input is valid and fits in the buffer
    if (!rundir || strlen(rundir) >= sizeof(g_run_dir)) {
        errno = EINVAL; // signal une erreur : 'Invalid Value/argument'
        return -1;
    }
    // Copy path
    strncpy(g_run_dir, rundir, sizeof(g_run_dir)-1);
    g_run_dir[sizeof(g_run_dir)-1] = '\0';
    return 0;
}

int client_get_rundir(char *out, size_t outlen)
{
    // Check arguments
    if (!out || outlen == 0) {
        errno = EINVAL;
        return -1;
    }
    // Copy rundir
    strncpy(out, g_run_dir, outlen-1);
    out[outlen-1] = '\0';
    return 0;
}

/* ------------------------- CONNECT --------------------------- */

int client_connect(void)
{
    // If rundir not set, generate default rundir under /tmp/$USER/erraid
    const char *user = getenv("USER");
    if (g_run_dir[0] == '\0') {
        if (!user) user = "nobody";
        snprintf(g_run_dir, sizeof(g_run_dir), "/tmp/%s/erraid", user);
    }

    // Build path to the FIFO files
    snprintf(path_in,  sizeof(path_in),  "%s/daemon_in",  g_run_dir);
    snprintf(path_out, sizeof(path_out), "%s/daemon_out", g_run_dir);

    // Open pipe for writing requests
    fd_in = open(path_in, O_WRONLY);
    if (fd_in < 0) return -1;

    // Open pipe for reading answers
    fd_out = open(path_out, O_RDONLY);
    if (fd_out < 0) {
        close(fd_in);
        fd_in = -1;
        return -1;
    }
    return 0;
}

void client_disconnect(void)
{
    // Close both pipes if they are opened
    if (fd_in  >= 0) close(fd_in);
    if (fd_out >= 0) close(fd_out);
    fd_in  = -1;
    fd_out = -1;
}

/* ------------------------ SEND REQUEST ------------------------ */
/**
 * Write a structure to the daemon. We expect to write exactly 'sz' bytes.
 * Return 0 on success.
 */
static int send_request(const void *req, size_t sz)
{
    ssize_t w = write(fd_in, req, sz);
    return (w == (ssize_t)sz) ? 0 : -1;
}

/* ------------------------ READ ANSWER ------------------------- */

/**
 * Read an expected number of bytes from the daemon.
 */
static void *read_answer(size_t sz)
{
    void *buf = malloc(sz);
    if (!buf) return NULL;

    ssize_t r = read(fd_out, buf, sz);
    if (r != (ssize_t)sz) { // Not enough bytes read -> Error
        free(buf);
        return NULL;
    }
    return buf; // Need to be freed by caller
}

/* -------------------------- API CALLS -------------------------- */

/**
 * Auxiliary method to send simple request.
 */
static void *client_simple(uint16_t opcode, uint64_t task_id, size_t answer_t)
{
    simple_request_t req = {
        .opcode = opcode,
        .task_id = task_id
    };

    // Send the request to the daemon
    if (send_request(&req, sizeof(req)) < 0)
        return NULL;

    // Read exactly answer_size bytes from the daemon
    return read_answer(answer_size);
}

a_list_t *client_ls(void)
{
    return (a_list_t *)client_simple(LS, 0, sizeof(a_list_t));
}

answer_t *client_rm(uint64_t task_id)
{
    return (answer_t *)client_simple(RM, task_id, sizeof(answer_t));
}

a_output_t *client_stdout(uint64_t task_id)
{
    return (a_output_t *)client_simple(SO, task_id, sizeof(a_output_t));
}

a_output_t *client_stderr(uint64_t task_id)
{
    return (a_output_t *)client_simple(SE, task_id, sizeof(a_output_t));
}

a_timecode_t *client_times(uint64_t task_id)
{
    return (a_timecode_t *)client_simple(TX, task_id, sizeof(a_timecode_t));
}

answer_t *client_terminate(void)
{
    return (answer_t *)client_simple(TM, 0, sizeof(answer_t));
}

/*
answer_t *client_create(timing_t *timing, command_t *cmd)
{
    complex_request_t req = {
        .opcode = CR,
        .timing = *timing,
        .u.command = *cmd
    };

    if (send_request(&req, sizeof(req)) < 0)
        return NULL;

    return (answer_t *)read_answer(sizeof(answer_t));
}

answer_t *client_combine(timing_t *timing, composed_t *comp)
{
    complex_request_t req = {
        .opcode = CB,
        .timing = *timing,
        .u.composed = *comp
    };

    if (send_request(&req, sizeof(req)) < 0)
        return NULL;

    return (answer_t *)read_answer(sizeof(answer_t));
}*/