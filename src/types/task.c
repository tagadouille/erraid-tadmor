#include <stdlib.h>
#include <stdio.h>

#include "types/task.h"
#include "types/command.h"
#include "types/argument.h"
#include "types/timing.h"
#include "types/time_exitcode.h"

/**
 * @brief Create a new empty task with the given ID.
 * ! The internal fields still need to be filled by the tree reader.
 */
task_t *task_create(uint32_t id)
{
    task_t *t = calloc(1, sizeof(task_t));
    if (!t)
        return NULL;

    t->id = id;
    t->command = NULL;         // Parsed later by cmd_reader
    t->timing.minutes = 0;     // Parsed later by timing_reader
    t->timing.hours = 0;
    t->timing.daysofweek = 0;

    // Exitcode history empty for now.
    // You can later store an array if needed.
    t->history.count = 0;
    t->history.records = NULL;

    return t;
}

/**
 * @brief Helper to recursively free a command structure.
 *        Works for both SI and SQ commands.
 */
static void command_free_recursive(command_t *cmd)
{
    if (!cmd)
        return;

    if (cmd->type == SI)
    {
        arguments_free(&cmd->u.simple);
    }
    else if (cmd->type == SQ)
    {
        for (uint32_t i = 0; i < cmd->u.composed.count; i++)
        {
            command_free_recursive(cmd->u.composed.cmds[i]);
        }
        free(cmd->u.composed.cmds);
        cmd->u.composed.cmds = NULL;
    }

    free(cmd);
}

/**
 * @brief Destroy an entire task and free all associated memory.
 */
void task_destroy(task_t *task)
{
    if (!task)
        return;

    // Free the command (SI or SQ)
    if (task->command)
        command_free_recursive(task->command);

    // Free timing (nothing dynamic inside timing_t)
    // Just primitive types → nothing to do.

    // Free exitcode history
    if (task->history.records)
        free(task->history.records);

    free(task);
}