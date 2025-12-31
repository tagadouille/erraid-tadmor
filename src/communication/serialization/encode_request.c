#define _POSIX_C_SOURCE 200809L

#include "communication/request.h"
#include "communication/serialization/serialization.h"
#include "communication/serialization/en_decode_struct.h"

#include <stdlib.h>
#include <stdio.h>

/* Complex request:
   - CR: OPCODE + TIMING + COMMAND (arguments)
   - CB: OPCODE + TIMING + TYPE(uint16) + NBTASKS(uint32) + TASKID[...]
*/
int encode_complex_request(int fd, const complex_request_t *r)
{
    if (!r) {
        dprintf(STDERR_FILENO, "Error: request pointer is NULL in encode_complex_request.\n");
        return -1;
    }

    dprintf(STDOUT_FILENO, "[encode_complex_request] Encoding complex request...\n");

    // Encode the base fields : 
    if (encode_uint16(fd, r->opcode) < 0) {
        dprintf(STDERR_FILENO, "[encode_complex_request] Error: Failed to encode opcode.\n");
        return -1;
    }
    dprintf(STDOUT_FILENO, "[encode_complex_request] Opcode: %u\n", r->opcode);

    if (encode_timing(fd, &r->timing) < 0) {
        dprintf(STDERR_FILENO, "[encode_complex_request] Error: Failed to encode timing.\n");
        return -1;
    }
    dprintf(STDOUT_FILENO, "[encode_complex_request] Timing encoded successfully.\n");

    // Create :
    if (r->opcode == CR) {

        dprintf(STDOUT_FILENO, "[encode_complex_request] Opcode is CR, encoding command...\n");
        if (!r->u.command) {
            dprintf(STDERR_FILENO, "[encode_complex_request] Error: command is NULL for CR request.\n");
            return -1;
        }

        // Encode the command
        if (encode_command(fd, r->u.command) < 0) {
            dprintf(STDERR_FILENO, "[encode_complex_request] Error: Failed to encode command.\n");
            return -1;
        }
        dprintf(STDOUT_FILENO, "[encode_complex_request] Command encoded successfully.\n");
        return 0;

    }
    // Combine :
    else if (r->opcode == CB) {
        
        dprintf(STDOUT_FILENO, "[encode_complex_request] Opcode is CB, encoding composed task...\n");
        if (!r->u.composed) {
            dprintf(STDERR_FILENO, "[encode_complex_request] Error: composed is NULL for CB request.\n");
            return -1;
        }

        if (encode_uint16(fd, r->u.composed->type) < 0){
            dprintf(STDERR_FILENO, "[encode_complex_request] Error: Failed to encode composed type.\n");
            return -1;
        }

        if (r->u.composed->nb_task > LIMIT_MAX_TASKS) {
            dprintf(STDERR_FILENO, "[encode_complex_request] Error: Task count %u exceeds limit %d.\n", r->u.composed->nb_task, LIMIT_MAX_TASKS);
            return -1;
        }

        if (encode_uint32(fd, r->u.composed->nb_task) < 0){
            dprintf(STDERR_FILENO, "[encode_complex_request] Error: Failed to encode number of tasks.\n");
            return -1;
        }
        dprintf(STDOUT_FILENO, "[encode_complex_request]  Number of tasks: %u\n", r->u.composed->nb_task);

        for (uint32_t i = 0; i < r->u.composed->nb_task; ++i) {
            if (encode_uint64(fd, r->u.composed->task_ids[i]) < 0){
                dprintf(STDERR_FILENO, "[encode_complex_request] Error: Failed to encode task ID at index %u.\n", i);
                return -1;
            }
        }
        dprintf(STDOUT_FILENO, "[encode_complex_request]  All task IDs encoded successfully.\n");
        return 0;
    }

    dprintf(STDERR_FILENO, "[encode_complex_request] Error: Unknown opcode %u in encode_complex_request.\n", r->opcode);
    return -1;
}

int encode_simple_request(int fd, const simple_request_t *r)
{
    if (!r){
        dprintf(STDERR_FILENO, "Error : The request can't be null\n");
        return -1;
    }

    if (encode_uint16(fd, r->opcode) < 0){
        dprintf(STDERR_FILENO, "Error : an error occured while encoding uint16\n");
        return -1;
    }

    /* LS and TM have no task id; other opcodes have task id */
    if (r->opcode == LS || r->opcode == TM) return 0;
    int ret = encode_uint64(fd, r->task_id);

    if(ret != 0){
        dprintf(STDERR_FILENO, "Error : an error occured while encoding uint64\n");
    }

    return ret;
}