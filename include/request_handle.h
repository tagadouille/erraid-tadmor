#ifndef REQUEST_HANDLE_H
#define REQUEST_HANDLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "communication/answer.h"
#include "communication/request.h"

a_list_t *handle_ls(char *rundir);

a_timecode_t *handle_tx(char *rundir, uint64_t id);

a_output_t *handle_output(char *rundir, uint64_t id, bool is_stder);

answer_t *handle_rm(char *rundir, uint64_t id);

answer_t *handle_tm(void);

/**
 * @brief Handle Simple request.
 * @param req the simple_request_t structure
 * @param rundir Base directory of task folders
 */
void *simple_request_handle(simple_request_t *req, char *rundir);

#endif