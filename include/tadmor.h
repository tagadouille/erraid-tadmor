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

/**
 * @brief Send TM (terminate daemon).
 * @return answer_t* or NULL
 */
answer_t *client_terminate(void);

/** Pas encore pour ce jalon
 * @brief Create a simple task (CR).
 * @return answer_t* or NULL
 */
answer_t *client_create(timing_t *timing, command_t *cmd);

/** 
 * @brief Create a composed task (CB).
 * @return answer_t* or NULL
 */
answer_t *client_combine(timing_t *timing, composed_t *comp);
#endif