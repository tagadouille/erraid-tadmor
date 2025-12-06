#ifndef TADMOR_H
#define TADMOR_H

#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdint.h>
#include "communication/request.h"
#include "communication/answer.h"

/** 
 * @brief Set the Default run directory: /tmp/$USER/erraid.
 * @return 0 on success.
 */
int client_set_rundir(const char *rundir);

/** 
 * @brief Get the run directory.
 * @return 0 on success.
 */
int client_get_rundir(char *out, size_t outlen);

/** 
 * @brief Open communication channels (FIFO or socket).
 * @return 0 on success.
*/
int client_connect(void);

/**
 * @brief Closes communication channels.
 */
void client_disconnect(void);

/* --------- HIGH LEVEL API ----------- */

/** 
 * @brief Send LS request.
 * @return a_list_t* or NULL 
 */
a_list_t *client_ls(void);

/** 
 * @brief Send RM request (task_id).
 * @return answer_t* or NULL
 */
answer_t *client_rm(uint64_t task_id);

/** 
 * @brief Send SO request (stdout of task).
 * @return a_output_t* or NULL
 */
a_output_t *client_stdout(uint64_t task_id);

/** 
 * @brief Send SE request (stderr of task).
 * @return a_output_t* or NULL
 */
a_output_t *client_stderr(uint64_t task_id);

/** 
 * @brief Send TX request (times/exitcodes).
 * @return a_timecode_t* or NULL
 */
a_timecode_t *client_times(uint64_t task_id);

/**
 * @brief Send TM (terminate daemon).
 * @return answer_t* or NULL
 */
answer_t *client_terminate(void);

/** Pas encore pour ce jalon
 * @brief Create a simple task (CR).
 * @return answer_t* or NULL
 *
answer_t *client_create(timing_t *timing, command_t *cmd);/

/** 
 * @brief Create a composed task (CB).
 * @return answer_t* or NULL
 *
answer_t *client_combine(timing_t *timing, composed_t *comp);/
#endif