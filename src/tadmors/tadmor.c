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
        dprintf(STDOUT_FILENO, "%lu\n", answer->task_id); // Print the task id
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
        dprintf(STDERR_FILENO, "No task found\n");
        return;
    }

    if(list->all_task->nbtask == 0){
        dprintf(STDOUT_FILENO, "No task found\n");
        return;
    }

    for (uint32_t i = 0; i < list->all_task->nbtask; i++) // Iterate through all tasks
    {
        task_t* t = &(list->all_task->all_task[i]);

        // check for NULL task
        if (t == NULL)
        {
            dprintf(STDERR_FILENO, "NULL task encountered\n");
            continue; // skip to next task
        }

        task_display(t);
    }
}

void tadmor_print_timecode(a_timecode_t* timecode)
{
    if (timecode == NULL)
    {
        dprintf(STDERR_FILENO, "Error : NULL timecode");
        return;
    }

    if (timecode->anstype == ERR)
    {
        dprintf(STDERR_FILENO, "Error: times-exitcodes file or task not found\n");
        return;
    }
    
    for (uint32_t i = 0; i < timecode->time_arr.nbruns; i++)
    {
        time_exitcode_t *tc = &(timecode->time_arr.all_timecode[i]); // Get each time_exitcode_t

        time_exitcode_show(tc); // Display each time_exitcode_t
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
        if (output->errcode == NF)
            dprintf(STDERR_FILENO, "Error: task not found\n");
        else if (output->errcode == NR)
            dprintf(STDERR_FILENO, "Error: no run available\n");
        return;
    }

    // Use write() for a safe display
    if (output->output->length > 0 && output->output->data != NULL)
    {
        if (write(STDOUT_FILENO, output->output->data, output->output->length) < 0) {
            perror("write tadmor_print_output");
        }
    }
}

void tadmor_print_response(uint16_t opcode, void* res)
{
    switch (opcode) {
        case LS:
            tadmor_print_list((a_list_t*)res);
            free_a_list((a_list_t*) res);
            break;
        case TX:
            tadmor_print_timecode((a_timecode_t*)res);
            free_a_timecode((a_timecode_t*) res);
            break;
        case SO: // same treatment for STDOUT and STDERR
        case SE:
            tadmor_print_output((a_output_t*)res);
            free_a_output((a_output_t*)res);
            break;
        default:
            tadmor_print_answer((answer_t*)res);
            free_answer((answer_t*) res);
            break;
    }
}