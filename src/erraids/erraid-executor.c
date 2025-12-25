#include "types/task.h"
#include "erraids/erraid.h"
#include "erraids/erraid-helper.h"
#include "types/time_exitcode.h"

#include <errno.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <endian.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define MAX_TASKS 1000000

/* ---------------------------------------------------------------
 * Helpers: convert to big endian
 * --------------------------------------------------------------- */
static inline uint64_t to_be64(uint64_t x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return ((uint64_t)htonl(x & 0xFFFFFFFFULL) << 32) | htonl(x >> 32);
#else
    return x;
#endif
}

static inline uint16_t to_be16(uint16_t x) {
    return htons(x);
}

/* ---------------------------------------------------------------
 * Append one record to times-exitcodes
 * --------------------------------------------------------------- */
static int append_times_exitcodes(const char *path, uint16_t exitcode, time_t timestamp) {
    int fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd < 0) { perror("open"); return -1; }

    uint8_t buffer[10];
    uint64_t t_be = to_be64((uint64_t)timestamp);
    uint16_t e_be = to_be16(exitcode);
    memcpy(buffer, &t_be, 8);
    memcpy(buffer + 8, &e_be, 2);

    ssize_t w = write(fd, buffer, sizeof(buffer));
    if (w != sizeof(buffer)) { perror("write"); close(fd); return -1; }

    fsync(fd);
    close(fd);
    return 0;
}

static int needs_shell(const char *cmd) {
    if (!cmd || !cmd[0]) return 0;
    
    // Métacaractères shell incontournables
    const char *shell_chars = ";|&<>()$`";
    
    // Vérification en tenant compte des guillemets
    int in_single_quote = 0;
    int in_double_quote = 0;
    int escaped = 0;
    
    for (const char *p = cmd; *p; p++) {
        if (escaped) {
            escaped = 0;
            continue;
        }
        
        if (*p == '\\') {
            escaped = 1;
            continue;
        }
        
        if (!in_double_quote && *p == '\'') {
            in_single_quote = !in_single_quote;
            continue;
        }
        
        if (!in_single_quote && *p == '"') {
            in_double_quote = !in_double_quote;
            continue;
        }
        
        // Si on est dans des guillemets, ignorer la plupart des métacaractères
        if (in_single_quote) continue;
        
        // En double quote, $ et ` sont toujours spéciaux
        if (in_double_quote && (*p == '$' || *p == '`')) {
            return 1;
        }
        
        // Hors des guillemets
        if (!in_single_quote && !in_double_quote) {
            if (strchr(shell_chars, *p)) {
                return 1;
            }
            
            // Vérifier les combinaisons de 2 caractères
            if ((*p == '&' && *(p+1) == '&') ||
                (*p == '|' && *(p+1) == '|')) {
                return 1;
            }
        }
    }
    
    return 0;
}

/* ---------------------------------------------------------------
 * Execute a command using argv (simple command)
 * Returns the exit code
 * --------------------------------------------------------------- */
static int execute_simple_fd(char **argv, int outfd, int errfd) {
    if (!argv || !argv[0]) return -1;

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return -1; }

    if (pid == 0) {
        dup2(outfd, STDOUT_FILENO);
        dup2(errfd, STDERR_FILENO);

        execvp(argv[0], argv);
        perror("execvp");
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 255;
}

/* ---------------------------------------------------------------
 * Execute a shell line (supports ;, |, &&, ||, redirections)
 * Returns exit code
 * --------------------------------------------------------------- */
static int execute_shell_line(const char *line, int outfd, int errfd) {
    if (!line) return -1;

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return -1; }

    if (pid == 0) {
        dup2(outfd, STDOUT_FILENO);
        dup2(errfd, STDERR_FILENO);

        char *argv[] = { "sh", "-c", (char *)line, NULL };
        execvp(argv[0], argv);
        perror("execvp");
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 255;
}

/* ---------------------------------------------------------------
 * Execute a simple command
 * Determines if it should run via shell or execvp
 * --------------------------------------------------------------- */
static int execute_simple_fd_only(const command_t *cmd, int outfd, int errfd) {

    if(!cmd){
        write_log_msg("The command can't be null");
        return -1;
    }
    if (cmd->type != SI) {
        write_log_msg("Invalid command type for simple execution");
        return -1;
    }

    if(!cmd->args.simple){
        write_log_msg("args.simple is null");
        return -1;
    }
    
    int exitcode = 0;
    int use_shell = 0;

    if (cmd->args.simple->argc == 1) {
        use_shell = needs_shell(cmd->args.simple->argv[0]->data);
    }

    if (use_shell) {
        exitcode = execute_shell_line(cmd->args.simple->argv[0]->data, outfd, errfd);
    } else {
        char **argv = arguments_to_argv(cmd->args.simple);
        if (!argv) {
            write_log_msg("Failed to convert arguments to argv for command");
            return -1;
        }
        
        exitcode = execute_simple_fd(argv, outfd, errfd);
        
        if (argv) {

            for (uint32_t i = 0; argv[i]; i++) {
                free(argv[i]);
            }
            free(argv);
        }
    }
    
    return exitcode;
}

/* ---------------------------------------------------------------
 * Execute any command with given file descriptors
 * --------------------------------------------------------------- */
static int execute_any_command_fd(const command_t *cmd, int outfd, int errfd,
                                  const char *timespath, time_t minute_now,
                                  int is_top_level) {
    if (!cmd) return -1;
    
    switch (cmd->type) {
        case SI: {
            int exitcode = execute_simple_fd_only(cmd, outfd, errfd);
            
            if (is_top_level && timespath) {
                append_times_exitcodes(timespath, exitcode, minute_now);
            }
            return exitcode;
        }
            
        case SQ: {
            int final_exitcode = 0;
            
            for (uint32_t i = 0; i < cmd->args.composed.count; i++) {
                int exitcode = execute_any_command_fd(
                    cmd->args.composed.cmds[i], 
                    outfd, errfd, 
                    NULL, minute_now,
                    0  // Pas top-level pour les sous-commandes
                );
                
                final_exitcode = exitcode;
            }
            
            if (is_top_level && timespath) {
                append_times_exitcodes(timespath, final_exitcode, minute_now);
            }
            return final_exitcode;
        }
            
        default:
            write_log_msg("Unknown command type: %d", cmd->type);
            return -1;
    }
}

/* ---------------------------------------------------------------
 * Dispatch execution depending on command type
 * --------------------------------------------------------------- */
static int execute_command(const command_t *cmd, const char *timespath,
                           const char *outpath, const char *errpath,
                           time_t minute_now) {
    if (!cmd) {
        write_log_msg("The command can't be null");
        return -1;
    }

    // Ouvrir fichiers
    int outfd = open(outpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    int errfd = open(errpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (outfd < 0 || errfd < 0) {
        perror("open");
        if (outfd >= 0) close(outfd);
        if (errfd >= 0) close(errfd);
        return -1;
    }

    // Exécuter
    int exitcode = execute_any_command_fd(cmd, outfd, errfd, 
                                          timespath, minute_now, 1);
    
    close(outfd);
    close(errfd);
    return exitcode;
}

/* ---------------------------------------------------------------
 * Execute a task
 * --------------------------------------------------------------- */
static int execute_task(task_t* task, time_t minute_now) {
    write_log_msg("Executing task %u", task->id);

    char id[32];
    snprintf(id, sizeof(id), "%u", (unsigned)task->id);

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

    return execute_command(task->cmd, timespath, outpath, errpath, minute_now);
}

/* ---------------------------------------------------------------
 * Run a task if its timing matches
 * --------------------------------------------------------------- */
int run_task_if_due(task_t *task, time_t minute_now) {
    if (!task) { write_log_msg("The task can't be null"); return -1; }
    if (!task->cmd) { write_log_msg("The task command can't be null"); return -1; }
    if (!task->timing) { write_log_msg("The task timing can't be null"); return -1; }
    if (!timing_match_at(task->timing, minute_now)) return 0;

    return execute_task(task, minute_now);
}
