#ifndef TADMOR_H
#define TADMOR_H

#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdint.h>
#include "communication/request.h"
#include "communication/answer.h"

/* Default run directory: /tmp/$USER/erraid */
int client_set_rundir(const char *rundir);

/* Returns 0 on success */
int client_get_rundir(char *out, size_t outlen);

/* Open communication channels (FIFO or socket) */
int client_connect(void);

/* Close communication channels */
void client_disconnect(void);

/* --------- HIGH LEVEL API ----------- */

/* Send LS request, return a_list_t* or NULL */
a_list_t *client_ls(void);

/* Send RM request (task_id) */
answer_t *client_rm(uint64_t task_id);

/* Send SO request (stdout of task) */
a_output_t *client_stdout(uint64_t task_id);

/* Send SE request (stderr of task) */
a_output_t *client_stderr(uint64_t task_id);

/* Send TX request (times/exitcodes) */
a_timecode_t *client_times(uint64_t task_id);

/* Send TM (terminate daemon) */
answer_t *client_terminate(void);

/* Create a simple task (CR) */
answer_t *client_create(timing_t *timing, command_t *cmd);

/* Create a composed task (CB) */
answer_t *client_combine(timing_t *timing, composed_t *comp);

#endif