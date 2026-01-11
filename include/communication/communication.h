#define _POSIX_C_SOURCE 200809L

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "request.h"
#include "answer.h"

#include <limits.h>
#include <stddef.h>

#define REQUEST_PIPE "erraid-request-pipe"
#define REPLY_PIPE "erraid-reply-pipe"

extern char pipe_path[PATH_MAX];

/* CLIENT */
/**
 * @brief Send a simple request to the daemon.
 * @param req Pointer to the simple_request_t structure.
 * @return 0 on success, -1 on failure.
 */
int client_send_simple(const simple_request_t *req);

/**
 * @brief Send a complex request to the daemon.
 * @param req Pointer to the complex_request_t structure.
 * @return 0 on success, -1 on failure.
 */
int client_send_complex(const complex_request_t *req);

/**
 * @brief Receive an answer from the daemon based on the opcode.
 * @param opcode The operation code to determine the type of answer.
 * @return Pointer to the answer structure, or NULL on failure.
 */
void* client_recv_answer(uint16_t opcode);

/* DAEMON */

/**
 * @brief Read a request from the daemon pipes.
 * @param fd_req Pointer to store the file descriptor of the request pipe.
 * @param req Pointer to the request structure to populate.
 * @return 1 or 2 on success, -1 on failure. 
 * If it returns 1 the request is simple, if 2 complex.
 */
int daemon_read(int* fd_req, void *req);

/**
 * @brief Send a simple answer through the daemon pipes.
 * @param ans Pointer to the answer_t structure.
 * @return 0 on success, -1 on failure.
 */
int daemon_reply_simple(const answer_t *ans);

#endif
