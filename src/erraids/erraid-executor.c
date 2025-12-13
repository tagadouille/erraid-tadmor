#include "types/task.h"
#include "erraid.h"
#include "erraids/erraid-helper.h"
#include "types/time_exitcode.h"

#include <errno.h>
#include <stdint.h>
#include <sys/wait.h>
#include <stdint.h>
#include <arpa/inet.h>

#define MAX_TASKS sizeof(uint64_t)

static time_t last_run_minute[MAX_TASKS];

/**
 * @brief Append one record to times-exitcodes 
 * @return 0 if on success, -1 if an error occured
 */
/*static int append_times_exitcodes(const char* path, uint16_t exitcode, time_t timestamp) {

    int fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0644);

    if (fd < 0){
        perror("open");
        return -1;
    }

    time_exitcode_t te;
    te.time = hton64((int64_t)timestamp);
    te.exitcode = htons(exitcode);

    if (write(fd, &te, sizeof(te)) != sizeof(te)) {
        perror("write");
        close(fd);
        return -1;
    }

    fsync(fd);
    close(fd);
    return 0;
}*/

static int append_times_exitcodes(const char *path, uint16_t exitcode, time_t timestamp)
{
    int fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd < 0)
        return -1;

    uint64_t t_be = htobe64((uint64_t)timestamp);
    uint16_t e_be = htobe16(exitcode);

    if (write(fd, &t_be, 8) != 8) {
        close(fd);
        return -1;
    }

    if (write(fd, &e_be, 2) != 2) {
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}


/* Execute a simple command and write stdout/stderr into task dir (overwrite),
   append times-exitcodes entry. */
static int execute_simple(const command_t *cmd, const char *timespath, const char * outpath, const char * errpath,
                          int is_subcmd, time_t minute_now)
{
    if (!cmd){
        write_log_msg("Error : The given command is NULL for execute_simple");
        return -1;
    }

    int outfd = open(outpath, O_CREAT | O_WRONLY | O_APPEND, 0644);
    int errfd = open(errpath, O_CREAT | O_WRONLY | O_APPEND, 0644);

    if (outfd < 0 || errfd < 0) {
        write_log_msg("Cannot open stdout/stderr at path %s", tasksdir);
        if (outfd >= 0) close(outfd);
        if (errfd >= 0) close(errfd);
        return -1;
    }

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

    close(outfd);
    close(errfd);

    int status;
    waitpid(pid, &status, 0);
    int exitcode = WIFEXITED(status) ? WEXITSTATUS(status) : 255;

    if (is_subcmd == 0) {
        append_times_exitcodes(timespath, exitcode, minute_now);
    }

    return exitcode;
}


static int execute_complexe(const command_t *cmd, const char *timespath, const char * outpath, const char * errpath, time_t minute_now){
    
    if (!cmd){
        write_log_msg("Error : The given command is NULL for execute_complex");
        return -1;
    }
    if(cmd->type != SQ){
        write_log_msg("Error : The type of the given command is not SQ for execute_complex");
        return -1;
    }

    int final_exitcode = 0;

    uint32_t count = cmd->args.composed.count;
    for (uint32_t i = 0; i < count; i++) {

        int ret = execute_simple(cmd->args.composed.cmds[i], timespath, outpath, errpath, 1, minute_now);

        final_exitcode = ret;
    }

    append_times_exitcodes(timespath, final_exitcode, minute_now);

    return final_exitcode;
}

/**
 * Execute a command in function of the type
 * @param cmd the command to execute
 * @param timespath the path where the times-exitcodes file is
 * @param outfd the file descriptor of the stdout file
 * @param errfd the file descriptor of the stderr file
 * @param minute_now the minute when the task will be executed
 */
static int execute_command(const command_t *cmd, const char *timespath, const char * outpath, const char * errpath, time_t minute_now){

    if (!cmd){
        write_log_msg("Error : The given command is NULL for execute_command");
        return -1;
    }

    if (cmd->type == SI)
        return execute_simple(cmd, timespath, outpath, errpath, 0, minute_now);

    if (cmd->type == SQ)
        return execute_complexe(cmd, timespath, outpath, errpath, minute_now);

    return -1;
}

/**
 * @brief proceed of the execution of the task
 * @param task the task to be executed
 */
static int execute_task(task_t* task, time_t minute_now){
    write_log_msg("Executing task %u", task->id);

    char id[32];
    snprintf(id, sizeof(id), "%u", (unsigned)task->id);

    // Creation of the pathes to the outputs files
    char outpath[PATH_MAX], errpath[PATH_MAX], timespath[PATH_MAX];

    if (snprintf(outpath, sizeof(outpath), "%s/%s/stdout", tasksdir, id) >= (int)sizeof(outpath)) {
        write_log_msg("outpath too long for task %u", task->id);
        return -1;
    }

    if (snprintf(errpath, sizeof(errpath), "%s/%s/stderr", tasksdir, id) >= (int)sizeof(errpath)) {
        write_log_msg("errpath too long for task %u", task->id);
        return -1;
    }

    if (snprintf(timespath, sizeof(timespath), "%s/%s/times-exitcodes", tasksdir, id) >= (int)sizeof(timespath)) {
        write_log_msg("timespath too long for task %u", task->id);
        return -1;
    }

    // Delete the file if they already exist : 

    if (unlink(outpath) < 0 && errno != ENOENT) {
        perror("unlink");
        write_log_msg("Error: can't delete file at path %s", outpath);
        return -1;
    }

    if (unlink(errpath) < 0 && errno != ENOENT) {
        perror("unlink");
        write_log_msg("Error: can't delete file at path %s", errpath);
        return -1;
    }

    // Execution
    return execute_command(task->cmd, timespath, outpath, errpath, minute_now);
}


int run_task_if_due(task_t *task, time_t minute_now){
    
    // Some verification of if the task must be execute
     if (!task || !task->cmd || !task->timing)
        return -1;

    // If the task was already launched
    if (last_run_minute[task->id] == minute_now)
        return 0;

    if (!timing_match_at(task->timing, minute_now))
        return 0;

    int ret = execute_task(task, minute_now);
    if (ret >= 0)
        last_run_minute[task->id] = minute_now;

    return ret;
}