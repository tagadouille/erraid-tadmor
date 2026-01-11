#include "erraids/erraid.h"
#include "erraids/erraid-helper.h"
#include "types/time_exitcode.h"
#include "erraids/erraid-executor.h"


int execute_if(const command_t *cmd, int outfd, int errfd, int infd, const char *timespath, time_t minute_now,
                                  int is_top_level) {

    if (cmd->args.composed.count < 2 || cmd->args.composed.count > 3) {
        write_log_msg("Invalid number of sub-commands for IF");
        return -1;
    }

    // If condition :
    int condition_exitcode = execute_any_command_fd(
        cmd->args.composed.cmds[0],
        outfd, errfd, infd,
        NULL, minute_now,
        0
    );

    int exitcode = 0;

    // Then :
    if (condition_exitcode == 0) {
        exitcode = execute_any_command_fd(
            cmd->args.composed.cmds[1],
            outfd, errfd, infd,
            NULL, minute_now,
            0
        );
    }
    // Else :
    else if (cmd->args.composed.count == 3) {
        exitcode = execute_any_command_fd(
            cmd->args.composed.cmds[2],
            outfd, errfd, infd,
            NULL, minute_now,
            0
        );
    }

    if (is_top_level && timespath) {
        append_times_exitcodes(timespath, exitcode, minute_now);
    }
    return exitcode;
}

int execute_pipe(const command_t *cmd, int outfd, int errfd, int infd, const char *timespath, time_t minute_now,
                                  int is_top_level){
    
    if (cmd->args.composed.count < 2) { 
        write_log_msg("Invalid number of sub-commands for PIPE");
        return -1; 
    }

    int prev_fd = -1;
    int exitcode = 0;

    // Iterate over each command in the pipe
    for (uint32_t i = 0; i < cmd->args.composed.count; i++) {
        int pipefd[2];

        if (i < cmd->args.composed.count - 1) {

            if (pipe(pipefd) == -1) {
                write_log_msg("Failed to create pipe");
                if (prev_fd != -1){
                    close(prev_fd);
                }
                return -1;
            }
        }
        else {
            pipefd[0] = -1;
            pipefd[1] = outfd;
        }

        int current_outfd = (i < cmd->args.composed.count - 1) ? pipefd[1] : outfd;

        exitcode = execute_any_command_fd(
            cmd->args.composed.cmds[i],
            current_outfd,
            errfd,
            prev_fd != -1 ? prev_fd : infd,
            NULL,
            minute_now,
            0
        );

        if (prev_fd != -1){
            close(prev_fd);
        }

        if (pipefd[1] != -1 && pipefd[1] != outfd){
            close(pipefd[1]);
        }

        prev_fd = pipefd[0];
    }

    if(is_top_level && timespath) {
        append_times_exitcodes(timespath, exitcode, minute_now);
    }

    return exitcode;
}