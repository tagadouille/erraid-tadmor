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
int client_send_simple(const simple_request_t *req);
void* client_recv_answer(uint16_t opcode);
/* DEMON */
int daemon_read_simple(int* fd_req, simple_request_t *req);
int daemon_reply_simple(const answer_t *ans);

#endif
