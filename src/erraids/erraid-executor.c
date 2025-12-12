#include "types/task.h"
#include "erraid.h"
#include "erraids/erraid-helper.h"
#include "types/time_exitcode.h"

#include <errno.h>
#include <stdint.h>
#include <sys/wait.h>
#include <stdint.h>
#include <arpa/inet.h>

/* Append one record to times-exitcodes: [be64 timestamp][be32 exitcode] atomically+safe */
static int append_times_exitcodes(const char* path, int exitcode) {

    int fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd < 0) {
        write_log_msg("Error: failed to open times-exitcodes file at path %s\n", path);
        perror("open");
        return -1;
    }

    time_exitcode_t te = {hton64((int64_t)time(NULL)), htons((uint16_t)exitcode)};

    ssize_t w = write(fd, &te, sizeof(time_exitcode_t));
    write_log_msg("Number of bytes write : %zd\n", w);
    if (w != sizeof(time_exitcode_t)) {
        perror("write");
        close(fd);
        return -1;
    }

    fsync(fd);
    close(fd);
    return 0;
}

/* Execute a simple command and write stdout/stderr into task dir (overwrite),
   append times-exitcodes entry. */
static int execute_simple(const command_t *cmd, const char *timespath, int outfd, int errfd,
                          int is_subcmd)
{
    if (!cmd) return -1;

    pid_t pid = fork();
    if (pid < 0) {
        write_log_msg("fork failed: %s", strerror(errno));
        close(outfd); close(errfd);
        return -1;
    }

    if (pid == 0) {
        if (dup2(outfd, STDOUT_FILENO) < 0) _exit(127);
        if (dup2(errfd, STDERR_FILENO) < 0) _exit(127);

        char **argv = arguments_to_argv(&cmd->args.simple);
        if (!argv) _exit(127);

        execvp(argv[0], argv);
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0);
    int exitcode = WIFEXITED(status) ? WEXITSTATUS(status) : 255;

    if (is_subcmd == 0) {
        append_times_exitcodes(timespath, exitcode);
    }

    return exitcode;
}


static int execute_complexe(const command_t *cmd, const char *timespath, int outfd, int errfd){
    
    if (!cmd || cmd->type != SQ)
        return -1;

    int final_exitcode = 0;

    uint32_t count = cmd->args.composed.count;
    for (uint32_t i = 0; i < count; i++) {

        int fd_out = dup(outfd);
        int fd_err = dup(errfd);

        int ret = execute_simple(cmd->args.composed.cmds[i], timespath, fd_out, fd_err, 1);

        final_exitcode = ret;
    }

    append_times_exitcodes(timespath, final_exitcode);

    return final_exitcode;
}

int execute_command(const command_t *cmd, const char *timespath, int outfd, int errfd){

    if (!cmd) return -1;

    if (cmd->type == SI)
        return execute_simple(cmd, timespath, outfd, errfd, 0);

    if (cmd->type == SQ)
        return execute_complexe(cmd, timespath, outfd, errfd);

    return -1;
}