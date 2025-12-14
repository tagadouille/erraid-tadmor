#ifndef TADMOR_H
#define TADMOR_H

#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdint.h>
#include "communication/request.h"
#include "communication/answer.h"

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

/**
 * @brief Disconnect the client from the deamon.
 */
void tadmor_disconnect(void);

/**
 * @brief print an answer.
 */
void tadmor_print_answer(answer_t* answer);

/**
 * @brief print the list of tasks.
 */
void tadmor_print_list(a_list_t* list);

/**
 * @brief print a timecode.
 */
void tadmor_print_timecode(a_timecode_t* timecode);

/**
 * @brief print an output.
 */
void tadmor_print_output(a_output_t* output);

#endif