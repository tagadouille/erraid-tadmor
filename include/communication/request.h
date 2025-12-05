#ifndef REQUEST_H
#define REQUEST_H

#include "communication/code.h"
#include  "types/timing.h"
#include <stdint.h>
#include "types/task.h"

/**
 * Define the structure of a simple request
 * A simple request is has RM, TX, SO, SE, LS and TM as opcode
 * Can be send directly into a pipe
 */
typedef struct{

    uint16_t opcode;
    uint64_t task_id; //optional

}simple_request_t;

typedef struct{

    uint16_t type;
    uint32_t nb_task;
    uint64_t* task_ids; //An array of task id with a size of nb_task

} composed_t;

//TODO trouver un moyen d'envoyer les données séparemment dans le pipe
/**
 * Define the structure of a complex request
 *  A complex request is has CR and CB as opcode
 *  Can be send directly into a pipe
 */
typedef struct{

    uint16_t opcode;
    timing_t timing;

    union{
        command_t command;
        composed_t composed; 
    }u; //TODO find a better name

}complex_request_t;

/**
 * @brief  creating a simple resquest
 * @param opcode the code of the request
 * @param task_id the task_id
 */
simple_request_t* create_simple_request(uint16_t opcode, uint64_t task_id);

/**
 * @brief free a simple request
 * @param request the request to free
 */
void free_simple_request(simple_request_t* request);

/**
 * @brief  creating a complex resquest
 * @param opcode the code of the request
 * @param timing the timing of the task
 * @param command the command (can be null if it's a composed task)
 * @param composed the composed command (can be null if it's a simple command)
 */
complex_request_t* create_complex_request(uint16_t opcode, timing_t* timing, command_t* command, composed_t* composed);

/**
 * @brief free a complex request
 * @param request the request to free
 */
void free_complex_request(complex_request_t* request);

#endif