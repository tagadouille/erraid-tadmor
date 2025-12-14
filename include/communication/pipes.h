#ifndef PIPES_H
#define PIPES_H

int daemon_setup_pipes(const char *rundir, int *req_rd);
int daemon_open_reply(const char *rundir, int *rep_wr);

int client_open_request(const char *rundir, int *req_wr);
int client_open_reply(const char *rundir, int *rep_rd);

#endif
