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

answer_t* tadmor_create_answer(uint16_t anstype, uint64_t task_id, uint16_t errcode)
{
    return create_answer(anstype, task_id, errcode);
}

a_list_t* tadmor_create_list(uint16_t anstype, uint32_t nbtask, task_t* all_task)
{
    return create_a_list(anstype, nbtask, all_task);
}

a_timecode_t* tadmor_create_timecode(uint16_t anstype, uint32_t nbrun, time_exitcode_t* all_timecode)
{
    return create_a_timecode_t(anstype, nbrun, all_timecode);
}

a_output_t* tadmor_create_output(uint16_t anstype, string_t output, uint16_t errcode)
{
    return create_a_output_t(anstype, output, errcode);
}