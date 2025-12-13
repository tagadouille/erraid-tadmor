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

void tadmor_disconnect(void)
{
    // Todo: implement disconnection logic if needed, but delete if not necessary
}

void tadmor_print_answer(answer_t* answer)
{
    if (answer == NULL)
    {
        dprintf(STDERR_FILENO, "NULL answer");
        return;
    }

    switch (answer->anstype)
    {
    case OK:
        dprintf(STDOUT_FILENO, "%lu\n", answer->task_id);
        break;
    
    case ERR:
        switch (answer->errcode)
        {
        case NF:
            dprintf(STDERR_FILENO, "Error: Not Found\n");
            break;
        case NR:
            dprintf(STDERR_FILENO, "Error: Not Running\n");
            break;
        default:
            dprintf(STDERR_FILENO, "Error: Unknown error\n");
            break;
        }
        break;
    default:
        dprintf(STDERR_FILENO, "Error: Invalid answer type\n");
        break;
    }
}

void tadmor_print_list(a_list_t* list)
{
    if (list == NULL)
    {
        dprintf(STDERR_FILENO, "NULL list");
        return;
    }

    for (uint32_t i = 0; i < list->nbtask; i++)
    {
        task_t* t = &(list->all_task[i]);

        // check for NULL task
        if (t == NULL)
        {
            dprintf(STDERR_FILENO, "NULL task encountered\n");
            continue; // skip to next task
        }

        task_display(t);
        // ! Il manque la partie où les timings sont comme ça : * * * dans timing_to_string
    }
}

void tadmor_print_timecode(a_timecode_t* timecode)
{
    if (timecode == NULL)
    {
        perror("NULL timecode");
        return;
    }

    if (timecode->anstype == ERR)
    {
        dprintf(STDERR_FILENO, "Error: %u\n", timecode->errcode);
        return;
    }
    
    for (uint32_t i = 0; i < timecode->nbrun; i++)
    {
        time_exitcode_t *tc = &(timecode->all_timecode[i]);

        time_exitcode_show(tc);
    }
}

void tadmor_print_output(a_output_t* output)
{
    if (output == NULL)
    {
        dprintf(STDERR_FILENO, "NULL output");
        return;
    }

    if (output->anstype == ERR)
    {
        if (out->errcode == NF)
            dprintf(STDERR_FILENO, "Error: task not found\n");
        else if (out->errcode == NR)
            dprintf(STDERR_FILENO, "Error: no run available\n");
        return;
    }

    // Print the output data
    if (output->output.data != NULL)
    {
        dprintf(STDOUT_FILENO, "%s", output->output.data);
    }
}

void tadmor_print_response(uint16_t opcode, void* res)
{
    switch (opcode)
    {
    case LS:
        tadmor_print_list((a_list_t*)res);
        break;
    case TX:
        tadmor_print_timecode((a_timecode_t*)res);
        break;
    case SO: // Même traitement pour STDOUT et STDERR
    case SE:
        tadmor_print_output((a_output_t*)res);
        break;
    default:
        tadmor_print_answer((answer_t*)res);
        break;
    }
}