#ifndef PIPES_H
#define PIPES_H

#define PIPE_FILE "pipe_file.te"

#ifndef PIPES_H
#define PIPES_H

#define PIPE_FILE "pipe_file.te"

/**
 * @brief Rename the pipe_path
 * @param new_path the new path name
 * @return 0 on success, -1 on failure.
 */
int pipe_path_rename(char *new_path);

/**
 * @brief Create and open request pipe for the daemon.
 * The daemon creates the FIFO if needed and opens it in read mode.
 * @return 0 on success, -1 on failure.
 */
int daemon_setup_pipes();

/**
 * @brief Open the reply pipe for the daemon.
 * @param rep_wr File descriptor opened in write mode.
 * @return 0 on success, -1 on failure.
 */
int daemon_open_reply(int *rep_wr);

/**
 * @brief Open the request pipe for the client.
 * @param req_wr File descriptor opened in write mode.
 * @return 0 on success, -1 on failure.
 */
int client_open_request(int *req_wr);

/**
 * @brief Open the reply pipe for the client.
 * @param rep_rd File descriptor opened in read mode.
 * @return 0 on success, -1 on failure.
 */
int client_open_reply(int *rep_rd);

/**
 * @brief Write data to the pipe file.
 * @return 0 on success, -1 on failure.
 */
int pipe_file_write(void);

/**
 * @brief Read data from the pipe file.
 * @return 0 on success, -1 on failure.
 */
int pipe_file_read(void);

#endif

