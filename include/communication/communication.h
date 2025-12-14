#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "request.h"
#include "answer.h"

#define REQUEST_PIPE "erraid-request-pipe/"
#define REPLY_PIPE "erraid-reply-pipe/"

extern char* pipe_path;

/* CLIENT */
int client_send_simple(const char *pipe_dir, const simple_request_t *req, answer_t *ans, int has_task);
/* DEMON */
int daemon_read_simple(int fd_req, simple_request_t *req);
int daemon_reply_simple(const char *pipe_dir, const answer_t *ans, int has_task);

#endif
