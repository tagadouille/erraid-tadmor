#include "types/task.h"
/**
 * Execute a command in function of the type
 * @param cmd the command to execute
 * @param timespath the path where the times-exitcodes file is
 * @param outfd the file descriptor of the stdout file
 * @param errfd the file descriptor of the stderr file
 */
int execute_command(const command_t *cmd, const char *timespath, int outfd, int errfd);