#define _POSIX_C_SOURCE 200809L

#include "communication/request.h"
#include "communication/serialization/serialization.h"
#include "communication/serialization/en_decode_struct.h"

#include <stdlib.h>
#include <stdio.h>

/**
 * @brief Decode a complex request from a file descriptor.
 * @param fd The file descriptor to read from.
 * @param r Pointer to the complex_request_t structure to populate.
 * @return 0 on success, -1 on failure.
 */
static int decode_complex_request(int fd, complex_request_t *r, uint16_t opcode)
{
    if (!r) {
        dprintf(STDERR_FILENO, "Error: request pointer is NULL in decode_complex_request.\n");
        return -1;
    }

    dprintf(STDOUT_FILENO, "[decode_complex_request] Decoding complex request...\n");

    r->opcode = opcode; // Use the opcode that was already read
    
    dprintf(STDOUT_FILENO, "[decode_complex_request] Opcode: %u\n", r->opcode);

    if (decode_timing(fd, &r->timing) < 0) {
        dprintf(STDERR_FILENO, "[decode_complex_request] Error: Failed to decode timing.\n");
        return -1;
    }
    dprintf(STDOUT_FILENO, "[decode_complex_request]  Timing decoded successfully.\n");

    // Create :
    if (r->opcode == CR) {
        dprintf(STDOUT_FILENO, "[decode_complex_request] Opcode is CR, decoding command...\n");
        
        // Let decode_command handle the full decoding process
        if (decode_command(fd, &r->u.command) < 0) {
            dprintf(STDERR_FILENO, "[decode_complex_request] Error: Failed to decode command for CR request.\n");
            return -1;
        }
        dprintf(STDOUT_FILENO, "[decode_complex_request] Command decoded successfully.\n");
        return 0;

    }
    // Combine : 
    else if (r->opcode == CB) {
        dprintf(STDOUT_FILENO, "[decode_complex_request] Opcode is CB, decoding composed task...\n");

        // Allocate memory for the composed structure
        r->u.composed = calloc(1, sizeof(composed_t));
        if (!r->u.composed) {
            perror("calloc for composed failed");
            return -1;
        }

        if (decode_uint16(fd, &r->u.composed->type) < 0) {
            free(r->u.composed);
            r->u.composed = NULL;
            return -1;
        }

        uint32_t n;
        if (decode_uint32(fd, &n) < 0) {
            free(r->u.composed);
            r->u.composed = NULL;
            return -1;
        }

        if (n > LIMIT_MAX_TASKS) {
            dprintf(STDERR_FILENO, "[decode_complex_request] Error: Task count %u exceeds limit %d.\n", n, LIMIT_MAX_TASKS);
            free(r->u.composed);
            r->u.composed = NULL;
            return -1;
        }
        r->u.composed->nb_task = n;
        dprintf(STDOUT_FILENO, "[decode_complex_request] Number of tasks: %u\n", n);

        r->u.composed->task_ids = calloc(n, sizeof(uint64_t));
        if (!r->u.composed->task_ids) {
            free(r->u.composed);
            r->u.composed = NULL;
            return -1;
        }

        for (uint32_t i = 0; i < n; ++i) {
            if (decode_uint64(fd, &r->u.composed->task_ids[i]) < 0) {
                dprintf(STDERR_FILENO, "Error: Failed to decode task_id at index %u.\n", i);
                free(r->u.composed->task_ids);
                free(r->u.composed);
                r->u.composed = NULL;
                return -1;
            }
        }
        dprintf(STDOUT_FILENO, "[decode_complex_request] All task IDs decoded successfully.\n");
        return 0;
    }

    dprintf(STDERR_FILENO, "Error: Unknown opcode %u in decode_complex_request.\n", r->opcode);
    return -1;
}

/**
 * @brief Decode a simple request from a file descriptor.
 * @param fd The file descriptor to read from.
 * @param r Pointer to the simple_request_t structure to populate.
 * @return 0 on success, -1 on failure.
 */
static int decode_simple_request(int fd, simple_request_t *r, uint16_t opcode)
{
    if (!r){
        dprintf(STDERR_FILENO, "Error : the request can't be NULL\n");
        return -1;
    }
    r->opcode = opcode; // Use the opcode that was already read
    dprintf(2, "[daemon] opcode is %u\n", r->opcode);

    if (r->opcode == LS || r->opcode == TM){
        return 0;
    }

    dprintf(2, "[daemon] decoding task_id\n");
    if (decode_uint64(fd, &r->task_id) < 0){
        dprintf(STDERR_FILENO, "Error : there's no uint64 here\n");
        return -1;
    }
    return 0;
}

int decode_request(int fd, void *r) {

    // Read the opcode to determine the request type
    uint16_t opcode;
    if (decode_uint16(fd, &opcode) < 0) {
        dprintf(STDERR_FILENO, "[decode_request] Error: Failed to decode opcode.\n");
        return -1;
    }

    if (opcode == CR || opcode == CB) {
        if(decode_complex_request(fd, (complex_request_t *)r, opcode) < 0){
            dprintf(STDERR_FILENO, "[decode_request] Error: Failed to decode complex request.\n");
            return -1;
        }
        return 2; // Represents a complex request
    }
    else {
        if(decode_simple_request(fd, (simple_request_t *)r, opcode) < 0){
            dprintf(STDERR_FILENO, "[decode_request] Error: Failed to decode simple request.\n");
            return -1;
        }
        return 1; // Represents a simple request
    }
}