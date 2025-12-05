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

static int fd_in = -1;   // client → daemon
static int fd_out = -1;  // daemon → client

/* --------------------------- RUNDIR --------------------------- */

int client_set_rundir(const char *rundir)
{
    if (!rundir || strlen(rundir) >= sizeof(g_run_dir)) {
        errno = EINVAL;
        return -1;
    }
    strncpy(g_run_dir, rundir, sizeof(g_run_dir)-1);
    g_run_dir[sizeof(g_run_dir)-1] = '\0';
    return 0;
}

int client_get_rundir(char *out, size_t outlen)
{
    if (!out || outlen == 0) {
        errno = EINVAL;
        return -1;
    }
    strncpy(out, g_run_dir, outlen-1);
    out[outlen-1] = '\0';
    return 0;
}

/* ------------------------- CONNECT --------------------------- */

int client_connect(void)
{
    const char *user = getenv("USER");
    if (g_run_dir[0] == '\0') {
        if (!user) user = "nobody";
        snprintf(g_run_dir, sizeof(g_run_dir), "/tmp/%s/erraid", user);
    }

    snprintf(path_in,  sizeof(path_in),  "%s/daemon_in",  g_run_dir);
    snprintf(path_out, sizeof(path_out), "%s/daemon_out", g_run_dir);

    fd_in = open(path_in, O_WRONLY);
    if (fd_in < 0) return -1;

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
    if (fd_in  >= 0) close(fd_in);
    if (fd_out >= 0) close(fd_out);
    fd_in  = -1;
    fd_out = -1;
}

/* ------------------------ SEND REQUEST ------------------------ */

static int send_request(const void *req, size_t sz)
{
    ssize_t w = write(fd_in, req, sz);
    return (w == (ssize_t)sz) ? 0 : -1;
}

/* ------------------------ READ ANSWER ------------------------- */

static void *read_answer(size_t sz)
{
    void *buf = malloc(sz);
    if (!buf) return NULL;

    ssize_t r = read(fd_out, buf, sz);
    if (r != (ssize_t)sz) {
        free(buf);
        return NULL;
    }
    return buf;
}

/* -------------------------- API CALLS -------------------------- */

a_list_t *client_ls(void)
{
    simple_request_t req = { .opcode = LS, .task_id = 0 };

    if (send_request(&req, sizeof(req)) < 0)
        return NULL;

    return (a_list_t *)read_answer(sizeof(a_list_t));
}

answer_t *client_rm(uint64_t task_id)
{
    simple_request_t req = { .opcode = RM, .task_id = task_id };

    if (send_request(&req, sizeof(req)) < 0)
        return NULL;

    return (answer_t *)read_answer(sizeof(answer_t));
}

a_output_t *client_stdout(uint64_t task_id)
{
    simple_request_t req = { .opcode = SO, .task_id = task_id };

    if (send_request(&req, sizeof(req)) < 0)
        return NULL;

    return (a_output_t *)read_answer(sizeof(a_output_t));
}

a_output_t *client_stderr(uint64_t task_id)
{
    simple_request_t req = { .opcode = SE, .task_id = task_id };

    if (send_request(&req, sizeof(req)) < 0)
        return NULL;

    return (a_output_t *)read_answer(sizeof(a_output_t));
}

a_timecode_t *client_times(uint64_t task_id)
{
    simple_request_t req = { .opcode = TX, .task_id = task_id };

    if (send_request(&req, sizeof(req)) < 0)
        return NULL;

    return (a_timecode_t *)read_answer(sizeof(a_timecode_t));
}

answer_t *client_terminate(void)
{
    simple_request_t req = { .opcode = TM, .task_id = 0 };

    if (send_request(&req, sizeof(req)) < 0)
        return NULL;

    return (answer_t *)read_answer(sizeof(answer_t));
}

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
}