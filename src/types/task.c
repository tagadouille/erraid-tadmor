#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "types/task.h"
#include "types/argument.h"
#include "types/timing.h"
#include "types/time_exitcode.h"
#include "tree-reading/tree_reader.h"
#include "erraids/erraid.h"
#include <ctype.h>

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

void task_display(task_t* task){
    dprintf(STDOUT_FILENO, "%lu: ", task->id);

    timing_show(task->timing);
    command_display(task->cmd);
    dprintf(STDOUT_FILENO, "\n");
}

command_t* add_simple_command(command_t* command, arguments_t* simple_args){

    if (command == NULL){
        dprintf(STDERR_FILENO, "add_simple_command: command == NULL\n");
        return NULL;
    }
    if (simple_args == NULL){
        dprintf(STDERR_FILENO, "add_simple_command: simple_args == NULL\n");
        return command;
    }

    command->type = SI;

    arguments_t *copied = copy_arguments(simple_args);
    if (copied == NULL) {
        dprintf(STDERR_FILENO, "add_simple_command: copy_arguments failed\n");
        return NULL;
    }

    command->args.simple = copied;

    return command;
}


command_t* add_complex_command(command_t* og_command, command_t* command){

    if(command == NULL){
        dprintf(STDERR_FILENO, "the command passed in parameter is NULL\n");
        return NULL;
    }
    if(og_command == NULL){
        dprintf(STDERR_FILENO, "the original command is NULL or not complex\n");
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

command_t* command_filler(char* buffer, unsigned int size, command_t* cmd, command_type_t type) {

    if (curr_task == NULL) {
        dprintf(STDERR_FILENO, "curr_task must be initialized before reading task\n");
        return NULL;
    }

    if (cmd == NULL) {

        curr_task->cmd = create_command(type);

        if (curr_task->cmd == NULL) {
            dprintf(STDERR_FILENO, "Error while creating command structure\n");
            return NULL;
        }
        cmd = curr_task->cmd;
    }

    // Parsing the buffer to extract arguments
    arguments_t* arg = arguments_parse(buffer, size);

    if(type == SI){
        command_t *new_cmd = add_simple_command(cmd, arg);

        if(new_cmd == NULL){
            dprintf(STDERR_FILENO, "Error while creating simple command\n");
            arguments_free(arg);
            return NULL;
        }
        arguments_free(arg);
        return new_cmd;
    }
    else {
        dprintf(STDERR_FILENO, "Error : Complex commands are not accepted for command_filler\n");
        arguments_free(arg);
        command_free(cmd);
        return NULL;
    }
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
    t->cmd = command_copy(og_task->cmd);

    if (t->cmd == NULL){
        task_destroy(t);
        return NULL;
    }

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

    // Free the command
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

/**
 * @brief Cleans up the content of a task (frees internal pointers)
 * but does not free the task structure itself.
 * This is useful for tasks allocated in an array.
 */
static void task_cleanup(task_t *task) {
    if (task == NULL) {
        return;
    }

    // Free the command
    if (task->cmd != NULL) {
        command_free(task->cmd);
        task->cmd = NULL;
    }

    if (task->timing != NULL) {
        free(task->timing);
        task->timing = NULL;
    }
}

void free_all_task(all_task_t* all_task){
    if(all_task == NULL){
        return;
    }

    for(uint32_t i = 0; i < all_task->nbtask; i++){
        task_cleanup(&all_task->all_task[i]);
    }
    free(all_task->all_task);
    free(all_task);
}