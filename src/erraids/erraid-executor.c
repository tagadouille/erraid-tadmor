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
static int append_times_exitcodes(const char* path, uint64_t timestamp, uint16_t exitcode) {
    int fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd < 0) return -1;

    uint8_t buf[10];
    uint64_t ts_be = hton64(timestamp);
    uint16_t ec_be = htons(exitcode);

    // copier les 8 octets du timestamp
    for (int i = 0; i < 8; i++)
        buf[i] = (ts_be >> (56 - i*8)) & 0xFF;

    // copier les 2 octets de l'exit code
    buf[8] = (ec_be >> 8) & 0xFF;
    buf[9] = ec_be & 0xFF;

    // boucle d'écriture complète
    ssize_t total = 0;
    while (total < 10) {
        ssize_t w = write(fd, buf + total, 10 - total);
        if (w <= 0) { close(fd); return -1; }
        total += w;
    }

    fsync(fd);
    close(fd);
    return 0;
}

/* Execute a simple command and write stdout/stderr into task dir (overwrite),
   append times-exitcodes entry. */
static int execute_simple(const command_t *cmd, const char *timespath, int outfd, int errfd,
                          int is_subcmd, time_t minute_now)
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

        char **argv = arguments_to_argv(cmd->args.simple);
        if (!argv) _exit(127);

        execvp(argv[0], argv);
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0);
    int exitcode = WIFEXITED(status) ? WEXITSTATUS(status) : 255;

    if (is_subcmd == 0) {
        append_times_exitcodes(timespath, exitcode, minute_now);
    }

    return exitcode;
}


static int execute_complexe(const command_t *cmd, const char *timespath, int outfd, int errfd, time_t minute_now){
    
    if (!cmd || cmd->type != SQ)
        return -1;

    int final_exitcode = 0;

    uint32_t count = cmd->args.composed.count;
    for (uint32_t i = 0; i < count; i++) {

        int fd_out = dup(outfd);
        int fd_err = dup(errfd);

        int ret = execute_simple(cmd->args.composed.cmds[i], timespath, fd_out, fd_err, 1, minute_now);

        final_exitcode = ret;
    }

    append_times_exitcodes(timespath, final_exitcode, minute_now);

    return final_exitcode;
}

int execute_command(const command_t *cmd, const char *timespath, int outfd, int errfd, time_t minute_now){

    if (!cmd) return -1;

    if (cmd->type == SI)
        return execute_simple(cmd, timespath, outfd, errfd, 0, minute_now);

    if (cmd->type == SQ)
        return execute_complexe(cmd, timespath, outfd, errfd, minute_now);

    return -1;
}