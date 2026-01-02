#include "erraids/erraid.h"
#include "erraids/erraid-helper.h"
#include "types/time_exitcode.h"
#include "erraids/erraid-executor.h"


int execute_if(const command_t *cmd, int outfd, int errfd, const char *timespath, time_t minute_now,
                                  int is_top_level) {

    if (cmd->args.composed.count < 2 || cmd->args.composed.count > 3) {
        write_log_msg("Invalid number of sub-commands for IF");
        return -1;
    }

    // If condition :
    int condition_exitcode = execute_any_command_fd(
        cmd->args.composed.cmds[0],
        outfd, errfd,
        NULL, minute_now,
        0
    );

    int exitcode = 0;

    // Then :
    if (condition_exitcode == 0) {
        exitcode = execute_any_command_fd(
            cmd->args.composed.cmds[1],
            outfd, errfd,
            NULL, minute_now,
            0
        );
    }
    // Else :
    else if (cmd->args.composed.count == 3) {
        exitcode = execute_any_command_fd(
            cmd->args.composed.cmds[2],
            outfd, errfd,
            NULL, minute_now,
            0
        );
    }

    if (is_top_level && timespath) {
        append_times_exitcodes(timespath, exitcode, minute_now);
    }
    return exitcode;
}