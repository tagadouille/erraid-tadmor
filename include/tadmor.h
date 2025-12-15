#ifndef TADMOR_H
#define TADMOR_H

#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdint.h>
#include "communication/request.h"
#include "communication/answer.h"

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

void tadmor_print_response(uint16_t opcode, void* res);

#endif