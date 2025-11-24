#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "types/task.h"
#include "types/argument.h"
#include "types/timing.h"
#include "types/time_exitcode.h"
#include "tree-reading/tree_reader.h"

task_t* curr_task = NULL;

/**
 * @brief Create a new empty task with the given ID.
 * ! The internal fields still need to be filled by the tree reader.
 */
task_t *task_create(uint16_t id){
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
    dprintf(STDOUT_FILENO, "%u: ", task->id);

    //timing_show(task->timing); //TODO finish timing_show
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
            if(cmd->args.simple.command == NULL){
                dprintf(STDERR_FILENO, "Error : NULL command string\n");
                return;
            }
            if(cmd->args.simple.command->data == NULL){
                dprintf(STDERR_FILENO, "Error : NULL command data\n");
                return;
            }
            dprintf(STDERR_FILENO, "%s ", cmd->args.simple.command->data);
            if(cmd->args.simple.argv != NULL){
                for (uint32_t i = 0; i < cmd->args.simple.argc - 1; i++){
                    dprintf(STDERR_FILENO, "%s", cmd->args.simple.argv[i]->data);

                    if(i != cmd->args.simple.argc - 1){
                        dprintf(STDERR_FILENO, " ");
                    }
                }
            }
            break;
        case SQ:
            dprintf(STDOUT_FILENO, "(");
            for (uint16_t i = 0; i < cmd->args.composed.count; i++){
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
    if (!command){
        dprintf(STDERR_FILENO, "The command that has been passed is NULL\n");
        return NULL;
    }
    if (!simple_args){
        dprintf(STDERR_FILENO, "simple_args is NULL\n");
        return command;
    }

    command->type = SI;

    /* Ensure destination simple struct is in a known state */
    command->args.simple.argc = 0;
    command->args.simple.command = NULL;
    command->args.simple.argv = NULL;

    // Copy arguments safely
    if (!copy_arguments(&command->args.simple, simple_args)){
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

    if(type != SI){
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
        curr_task->cmd = create_command(cmd, type);

        if(curr_task->cmd == NULL){
            dprintf(STDERR_FILENO, "Error while creating command structure\n");
            return NULL;
        }
        cmd = curr_task->cmd;
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
    } else {
        dprintf(STDERR_FILENO, "Error : Complex commands are not accepted for command_filler\n");
        arguments_free(arg);
        command_free(cmd);
        free(arg);
        free(cmd);
        return NULL;
    }
    return cmd;
}

/**
 * @brief Helper to recursively free a command structure.
 *        Works for both SI and SQ commands.
 */
void command_free(command_t *cmd){
    if (!cmd)
        return;

    if (cmd->type == SI)
    {
        arguments_free(&cmd->args.simple);
    }
    else if (cmd->type == SQ)
    {
        for (uint16_t i = 0; i < cmd->args.composed.count; i++)
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

    free(task->timing);
    task->timing = NULL;
    free(task);
}

// ! Modify it

/*
void command_execute(const command_t *cmd)
{
    if (cmd == NULL) // Security check
        return;

    if (cmd->type == SI)
    {
        // Execute simple command
        if (cmd->u.simple.argc > 0) // On vérifie qu'il y a au moins un argument
        {
                // nom de la commande    // arguments
            execvp(cmd->u.simple.command->data, cmd->u.simple.argv);
            perror("execvp failed"); // Si execvp échoue, on affiche une erreur
        }
    }
    else // SQ
    {
        // Execute composed commands sequentially
        for (uint32_t i = 1; i < cmd->u.composed.count; i++)
        {
            command_execute(cmd->u.composed.cmds[i]); // Appel récursif
        }
    }
    command_free((command_t *)cmd); // Free the command after execution
}*/