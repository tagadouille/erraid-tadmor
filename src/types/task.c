#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "types/task.h"
#include "types/argument.h"
#include "types/timing.h"
#include "types/time_exitcode.h"
#include "tree-reading/tree_reader.h"
#include "erraid.h"

/**
 * @brief Create a new empty task with the given ID.
 * ! The internal fields still need to be filled by the tree reader.
 */
task_t *task_create(uint64_t id){
    task_t *t = calloc(1, sizeof(task_t));
    if (!t){
        perror("calloc");
        return NULL;
    }

    t->id = id;
    t->cmd = NULL;         // Parsed later by cmd_reader

    // Initialisation of timing
    t -> timing = malloc(sizeof(timing_t));

    if(t -> timing == NULL){
        perror("malloc");
        free(t);
        return NULL;
    }

    t->timing -> minutes = 0;     // Parsed later by timing_reader
    t->timing -> hours = 0;
    t->timing -> daysofweek = 0;

    return t;
}

//! Maybe change STDOUT_FILENO to something else
void task_display(task_t* task){
    dprintf(STDOUT_FILENO, "%lu: ", task->id);

    timing_show(task->timing);
    command_display(task->cmd);
    dprintf(STDOUT_FILENO, "\n");
}

void command_display(command_t *cmd){
    if (cmd == NULL){
        dprintf(STDERR_FILENO, "NULL command\n");
        return;
    }

    switch (cmd->type){
        case SI:
            if(cmd->args.simple->command == NULL){
                dprintf(STDERR_FILENO, "Error : NULL command string\n");
                return;
            }
            if(cmd->args.simple->command->data == NULL){
                dprintf(STDERR_FILENO, "Error : NULL command data\n");
                return;
            }
            dprintf(STDERR_FILENO, "%s ", cmd->args.simple->command->data);

            if(cmd->args.simple->argv != NULL){
                for (uint32_t i = 0; i < cmd->args.simple->argc - 1; i++){
                    dprintf(STDERR_FILENO, "%s", cmd->args.simple->argv[i]->data);

                    if(i != cmd->args.simple->argc - 1){
                        dprintf(STDERR_FILENO, " ");
                    }
                }
            }
            break;
        case SQ:
            dprintf(STDOUT_FILENO, "(");
            for (uint32_t i = 0; i < cmd->args.composed.count; i++){
                command_display(cmd->args.composed.cmds[i]);

                if(i != cmd->args.composed.count - 1){
                    dprintf(STDOUT_FILENO, "; ");
                }
            }
            dprintf(STDOUT_FILENO, ")");
            break;
        default:
            dprintf(STDERR_FILENO, "Unknown command type: %d\n", cmd->type);
            return;
    }
}

command_t* add_simple_command(command_t* command, const arguments_t* simple_args){
    
    if (command == NULL){
        dprintf(STDERR_FILENO, "The command that has been passed is NULL\n");
        return NULL;
    }
    if (simple_args == NULL){
        dprintf(STDERR_FILENO, "simple_args is NULL\n");
        return command;
    }

    command->type = SI;

    /* Ensure destination simple struct is in a known state */
    command->args.simple = copy_arguments(simple_args);

    if (&(command->args.simple) == NULL){
        dprintf(STDERR_FILENO, "Failed to copy arguments\n");
        return NULL;
    }
    return command;
}

command_t* add_complex_command(command_t* og_command, command_t* command){
    if(command == NULL){
        dprintf(STDERR_FILENO, "the command passed in parameter is NULL\n");
        return NULL;
    }
    if(og_command == NULL){
        dprintf(STDERR_FILENO, "the original command is NULL or not of type SQ\n");
        return NULL;
    }
    // Add the new command to the composed commands array
    if(og_command->args.composed.count % 10 == 0){
        command_t** new_cmds = realloc(
            og_command->args.composed.cmds, 
            sizeof(command_t*) * (og_command->args.composed.count + 10)
        );
        if(new_cmds == NULL){
            perror("realloc");
            return NULL;
        }
        og_command->args.composed.cmds = new_cmds;
    }
    og_command->args.composed.cmds[og_command->args.composed.count++] = command;

    return og_command;
}

command_t* create_command(command_t* command, command_type_t type){

    command = malloc(sizeof(command_t));

    if(command == NULL){
        perror("malloc");
        return NULL;
    }
    command->type = type;

    if(type != SI){ // SQ
        command->args.composed.count = 0;
        command->args.composed.cmds = malloc(sizeof(command_t*) * 10); // Initial allocation for 10 commands
        
        if(command->args.composed.cmds == NULL){
            perror("malloc");
            free(command);
            return NULL;
        }
    }
    return command;
}

command_t* command_filler(char* buffer, unsigned int size, command_t* cmd, command_type_t type){
    
    if(cmd == NULL){
        if(curr_task == NULL){
            dprintf(STDERR_FILENO, "curr_task must be initialize before reading task\n");
        }
        curr_task->cmd = create_command(cmd, type);

        if(curr_task->cmd == NULL){
            dprintf(STDERR_FILENO, "Error while creating command structure\n");
            return NULL;
        }
        cmd = curr_task->cmd;
    }

    // Remove trailing newlines
    while (size > 0 && (buffer[size - 1] == '\n' || buffer[size - 1] == '\r'))
        size--;

    // Ignore empty or whitespace-only content
    int only_spaces = 1;
    for (unsigned int i = 0; i < size; i++) {
        if (buffer[i] != ' ' && buffer[i] != '\t') {
            only_spaces = 0;
            break;
        }
    }
    if (size == 0 || only_spaces) {
        return cmd; // skip empty argument
    }

    // Parsing the buffer to extract arguments
    arguments_t* arg = arguments_parse(buffer, size);
    
    if(arg == NULL){   
        dprintf(STDERR_FILENO, "Error while parsing argv content\n");
        return NULL;
    }

    // Filling the command structure based on its type
    if(type == SI){

        cmd = add_simple_command(cmd, arg);

        if(cmd == NULL){
            dprintf(STDERR_FILENO, "Error while creating simple command\n");
            arguments_free(arg);
            free(arg);
        }
        arguments_free(arg);
        free(arg);
    } else {
        dprintf(STDERR_FILENO, "Error : Complex commands are not accepted for command_filler\n");
        arguments_free(arg);
        free(arg);
        command_free(cmd);
        return NULL;
    }
    return cmd;
}

/**
 * @brief Helper to recursively free a command structure.
 *        Works for both SI and SQ commands.
 */
void command_free(command_t *cmd){
    if (cmd == NULL)
        return;

    if (cmd->type == SI)
    {
        arguments_free(cmd->args.simple);
    }
    else if (cmd->type == SQ)
    {
        for (uint32_t i = 0; i < cmd->args.composed.count; i++)
        {
            command_free(cmd->args.composed.cmds[i]);
            cmd->args.composed.cmds[i] = NULL;
        }
        free(cmd->args.composed.cmds);
        cmd->args.composed.cmds = NULL;
    }

    cmd->args.composed.count = 0;
    free(cmd);
}

command_t* command_copy(const command_t* src) {

    if (src == NULL) {
        dprintf(STDERR_FILENO, "The command can't be null\n");
        return NULL;
    }

    command_t *cmd = calloc(1, sizeof(command_t));
    if (cmd == NULL) {
        perror("calloc");
        return NULL;
    }

    cmd->type = src->type;

    if (src->type == SI) {

        if(src->args.simple == NULL){
            dprintf(STDERR_FILENO, "The argument of the source task can't be null.\n");
            return NULL;
        }
        else{
            cmd->args.simple = copy_arguments(src->args.simple);

            if (cmd->args.simple == NULL) {
                dprintf(STDERR_FILENO, "Error copy arguments\n");
                command_free(cmd);
                return NULL;
            }
            if (cmd->args.simple->command == NULL || cmd->args.simple->argv == NULL) {
                dprintf(STDERR_FILENO, "Error copy arguments\n");
                command_free(cmd);
                return NULL;
            }
        }
        
    } else {
        /* composed command */
        uint32_t count = src->args.composed.count;
        cmd->args.composed.count = count;

        if (count == 0) {
            cmd->args.composed.cmds = NULL;
        }
        else {
            cmd->args.composed.cmds = calloc(count, sizeof(command_t *));

            if (cmd->args.composed.cmds == NULL) {
                perror("calloc composed.cmds");
                command_free(cmd);
                return NULL;
            }

            for (uint32_t i = 0; i < count; ++i) {
                /* copy child recursively */
                cmd->args.composed.cmds[i] = command_copy(src->args.composed.cmds[i]);

                if (cmd->args.composed.cmds[i] == NULL) {
                    command_free(cmd);
                    return NULL;
                }
            }
        }
    }

    return cmd;
}


task_t* task_copy(task_t* og_task){

    if(og_task == NULL){
        dprintf(STDERR_FILENO, "the original task is NULL\n");
        return NULL;
    }

    task_t* t = malloc(sizeof(task_t));
    if(t == NULL){
        perror("malloc");
        return NULL;
    }

    t->id = og_task->id;

    //copy command
    dprintf(STDOUT_FILENO, "bef\n");
    t->cmd = command_copy(og_task->cmd);

    if (t->cmd == NULL){
        task_destroy(t);
        return NULL;
    }
    dprintf(STDOUT_FILENO, "cmd cpy reussi\n");
    //copy timing
    t->timing = timing_copy(og_task->timing);
    if (t->timing == NULL) {
        task_destroy(t);
        return NULL;
    }
    return t;
}

/**
 * @brief Destroy an entire task and free all associated memory.
 */
void task_destroy(task_t *task){
    if (task == NULL){
        return;
    }

    // Free the command (SI or SQ)
    if (task->cmd != NULL){
        command_free(task->cmd);
        task->cmd = NULL;
    }

    if (task->timing != NULL) {
        free(task->timing);
        task->timing = NULL;
    }

    // If we're destroying the current global task, clear the global too
    if (curr_task == task) {
        curr_task = NULL;
    }

    free(task);
}