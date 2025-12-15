#ifndef PIPES_H
#define PIPES_H

/**
 * @brief rename the pipe_path
 * @param new_path the new path name
 * @return 0 if succes, -1 if failure
 */
int pipe_path_rename(char* new_path);

int daemon_setup_pipes(int *req_rd);
int daemon_open_reply(int *rep_wr);

int client_open_request(int *req_wr);
int client_open_reply(int *rep_rd);

#endif
