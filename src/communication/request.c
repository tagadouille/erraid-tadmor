#define _GNU_SOURCE

#include "communication/code.h"
#include "communication/request.h"


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

simple_request_t* create_simple_request(uint16_t opcode, uint64_t task_id){
    
    simple_request_t* request = malloc(sizeof(simple_request_t));

    if(request == NULL){
        perror("malloc");
        return NULL;
    }
    request -> opcode = opcode;
    request -> task_id = task_id;

    return request;
}

void free_simple_request(simple_request_t* request){
    if(request == NULL){
        return;
    }
    free(request);
}

complex_request_t* create_complex_request(uint16_t opcode, timing_t* timing, command_t* command, composed_t* composed){
    
    complex_request_t* request = malloc(sizeof(complex_request_t));

    if(request == NULL){
        perror("malloc");
        return NULL;
    }
    request->opcode = opcode;

    // Copy the content of the timing structure
    if (timing != NULL) {
        memcpy(&request->timing, timing, sizeof(timing_t));
    } else {
        // Handle case where timing is NULL if necessary, e.g., zero it out
        memset(&request->timing, 0, sizeof(timing_t));
    }

    if(command == NULL && composed == NULL){
        dprintf(STDERR_FILENO, "ERROR: both command and composed are NULL in create_complex_request\n");
        free(request);
        return NULL;
    }

    if(command != NULL && composed != NULL){
        dprintf(STDERR_FILENO, "ERROR: both command and composed are not NULL in create_complex_request\n");
        free(request);
        return NULL;
    }

    // Assign the correct member of the union
    if (command != NULL) {
        request->u.command = command;
    } else {
        request->u.composed = composed;
    }

    return request;
}

/**
 * @brief free a composed_t structure
 * @param composed the composed_t structure to free
 */
static void composed_free(composed_t* composed){

    if(composed == NULL){
        return;
    }

    if(composed -> task_ids){
        free(composed -> task_ids);
    }
    free(composed);
}

void free_complex_request(complex_request_t* request){
    
    if(request == NULL){
        return;
    }

    // The timing struct is part of the request, no need to free it separately.

    // Free the correct member of the union based on the opcode
    if (request->opcode == CR) {
        if(request->u.command){
            command_free(request->u.command);
        }
    } else if (request->opcode == CB) {
        if(request->u.composed){
            composed_free(request->u.composed);
        }
    }
    free(request);
}