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
 * @brief Create a basic answer (OK or ERR).
 */
answer_t* tadmor_create_answer(uint16_t anstype, uint64_t task_id, uint16_t errcode);

/**
 * @brief Create a_list_t object (caller provides task array or NULL).
 */
a_list_t* tadmor_create_list(uint16_t anstype, uint32_t nbtask, task_t* all_task);

/**
 * @brief Create a_timecode_t object (caller provides array or NULL).
 */
a_timecode_t* tadmor_create_timecode(uint16_t anstype, uint32_t nbrun, time_exitcode_t* all_timecode);

/**
 * @brief Create a_output_t object (caller provides string_t output).
 */
a_output_t* tadmor_create_output(uint16_t anstype, string_t output, uint16_t errcode);

#endif