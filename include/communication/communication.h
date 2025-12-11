#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "request.h"
#include "answer.h"

/* CLIENT */
int client_send_simple(const char *rundir, const simple_request_t *req, answer_t *ans);

/* DEMON */
int daemon_read_simple(int fd_req, simple_request_t *req);
int daemon_reply_simple(const char *rundir, const answer_t *ans);

#endif
