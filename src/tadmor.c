#define _POSIX_C_SOURCE 200809L

#include "tadmor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

#define TADMOR_REQ_PIPE "daemon_in"
#define TADMOR_REP_PIPE "daemon_out"

static char g_run_dir[PATH_MAX] = {0};
static char pipe_in[PATH_MAX] = {0};  /* client -> daemon */
static char pipe_out[PATH_MAX] = {0}; /* daemon -> client */

static int fd_in = -1;  // client -> daemon
static int fd_out = -1; // daemon -> client

/* --------------------------- RUNDIR --------------------------- */

int client_set_rundir(const char *rundir)
{
    // Check if input is valid and fits in the buffer
    if (!rundir || strlen(rundir) >= sizeof(g_run_dir))
    {
        errno = EINVAL; // signal une erreur : 'Invalid Value/argument'
        return -1;
    }
    // Copy path
    strncpy(g_run_dir, rundir, sizeof(g_run_dir) - 1);
    g_run_dir[sizeof(g_run_dir) - 1] = '\0';
    return 0;
}

int client_get_rundir(char *out, size_t outlen)
{
    // Check arguments
    if (!out || outlen == 0)
    {
        errno = EINVAL;
        return -1;
    }
    // Copy rundir
    strncpy(out, g_run_dir, outlen - 1);
    out[outlen - 1] = '\0';
    return 0;
}

/* ---------------------- internal helpers --------------------- */

/* write exactly 'count' bytes (handle short writes and EINTR) */
static ssize_t write_all(int fd, const void *buf, size_t count)
{
    const unsigned char *p = buf;
    size_t left = count;
    while (left > 0)
    {
        ssize_t w = write(fd, p, left);
        if (w < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        p += w;
        left -= (size_t)w;
    }
    return (ssize_t)count;
}

/** @brief read exactly 'count' bytes (handle short reads and EINTR)
 *  @return number of bytes read (== count) or -1 on error or 0 on EOF */
static ssize_t read_all(int fd, void *buf, size_t count)
{
    unsigned char *p = buf;
    size_t left = count;
    while (left > 0)
    {
        ssize_t r = read(fd, p, left);
        if (r < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (r == 0)
        {
            /* EOF */
            return 0;
        }
        p += r;
        left -= (size_t)r;
    }
    return (ssize_t)count;
}

/* allocate buffer and read exactly sz bytes from reply pipe */
/* caller must free returned pointer (on success) 
static void *read_answer(size_t sz)
{
    if (fd_out < 0)
    {
        errno = EBADF;
        return NULL;
    }

    void *buf = malloc(sz);
    if (!buf)
        return NULL;

    ssize_t r = read_all(fd_out, buf, sz);
    if (r != (ssize_t)sz)
    {
        free(buf);
        if EOF (r==0) propagate as error 
        return NULL;
    }
    return buf;
}*/

/* ------------------------- CONNECT --------------------------- */

int client_connect(void)
{
    // If rundir not set, generate default rundir under /tmp/$USER/erraid
    const char *user = getenv("USER");
    if (g_run_dir[0] == '\0')
    {
        if (!user)
            user = "nobody";
        if (snprintf(g_run_dir, sizeof(g_run_dir), "/tmp/%s/erraid", user) < 0)
        {
            errno = ENAMETOOLONG; // nom de chemin trop long
            return -1;
        }
    }

    /* build pipes directory: "<rundir>/pipes" */
    char pipes_dir[PATH_MAX];
    if (snprintf(pipes_dir, sizeof(pipes_dir), "%s/pipes", g_run_dir) < 0)
    {
        errno = ENAMETOOLONG;
        return -1;
    }

    /* build exact full paths */
    if (snprintf(pipe_in, sizeof(pipe_in), "%s/%s", pipes_dir, TADMOR_REQ_PIPE) < 0)
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    if (snprintf(pipe_out, sizeof(pipe_out), "%s/%s", pipes_dir, TADMOR_REP_PIPE) < 0)
    {
        errno = ENAMETOOLONG;
        return -1;
    }

    // Open pipe for writing requests
    fd_in = open(pipe_in, O_WRONLY);
    if (fd_in < 0)
        return -1;

    // Open pipe for reading answers
    fd_out = open(pipe_out, O_RDONLY);
    if (fd_out < 0)
    {
        int save = errno;
        close(fd_in);
        fd_in = -1;
        errno = save;
        return -1;
    }
    return 0;
}

void client_disconnect(void)
{
    // Close both pipes if they are opened
    if (fd_in >= 0)
        close(fd_in);
    if (fd_out >= 0)
        close(fd_out);
    fd_in = -1;
    fd_out = -1;
}

/* -------------------------- API CALLS -------------------------- */

/**
 * Auxiliary method to send simple request.
 
static void *client_simple(uint16_t opcode, uint64_t task_id, size_t answer_size)
{
    simple_request_t req = {
        .opcode = opcode,
        .task_id = task_id};

    // Send the request to the daemon
    if (send_request(&req, sizeof(req)) < 0)
        return NULL;

    // Read exactly answer_size bytes from the daemon
    return read_answer(answer_size);
}*/

answer_t *client_terminate(void)
{
    //return (answer_t *)client_simple(TM, 0, sizeof(answer_t));
    return NULL;
}