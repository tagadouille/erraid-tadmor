/**
 * @brief Execute a command_t of type IF
 * @param cmd the command to execute
 * @param outfd the file descriptor of the stdout file
 * @param errfd the file descriptor of the stderr file
 * @param infd the file descriptor of the input
 * @param timespath the path to the times-exitcodes file
 * @param minute_now the minute to execute the task
 * @param is_top_level 1 if we are on the top of the command tree, 0 if not
 * @return 0 on success, -1 on failure
 */
int execute_if(const command_t *cmd, int outfd, int errfd, int infd, const char *timespath, 
    time_t minute_now,int is_top_level);

/**
 * @brief Execute a command_t of type PL
 * @param cmd the command to execute
 * @param outfd the file descriptor of the stdout file
 * @param errfd the file descriptor of the stderr file
 * @param infd the file descriptor of the input
 * @param timespath the path to the times-exitcodes file
 * @param minute_now the minute to execute the task
 * @param is_top_level 1 if we are on the top of the command tree, 0 if not
 * @return 0 on success, -1 on failure
 */
int execute_pipe(const command_t *cmd, int outfd, int errfd, int infd, const char *timespath, time_t minute_now,
                                  int is_top_level);