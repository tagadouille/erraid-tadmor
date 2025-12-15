#ifndef REQUEST_HANDLE_H
#define REQUEST_HANDLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "communication/answer.h"
#include "communication/request.h"

/**
 * @brief Create a list of all tasks.
 * @param rundir Base directory of task folders
 */
a_list_t *handle_ls(char *rundir);

/**
 * @brief Remove a task folder.
 * @param rundir Base directory of task folders
 * @param id Task id
 */
answer_t *handle_rm(char *rundir, uint64_t id);

/**
 * @brief Retrieve time-exitcodes of a task.
 * @param rundir Base directory of task folders
 * @param id Task id
 */
a_timecode_t *handle_tx(char *rundir, uint64_t id);

/**
 * @brief Retrieve output or error output of a task.
 * @param rundir Base directory of task folders
 * @param id Task id
 * @param is_stderr true = stderr, false = stdout
 */
a_output_t *handle_output(char *rundir, uint64_t id, bool is_stder);

/**
 * @brief Terminate the daemon.
 */
answer_t *handle_tm(void);

/**
 * @brief Handle Simple request.
 * @param req the simple_request_t structure
 * @param rundir Base directory of task folders
 */
void *simple_request_handle(simple_request_t *req, char *rundir);

#endif