#define _POSIX_C_SOURCE 200809L /* for PATH_MAX, etc. */

#include "erraid.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>          
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <stdint.h>

#include <stdarg.h>    // pour va_start, va_end
#include <arpa/inet.h> // pour htonl / ntohl
#include <dirent.h>    // pour DIR / opendir / readdir

#include "types/task.h" 
#include "tree-reading/tree_reader.h"

/* ---------------------------- CONFIG ---------------------------------- */

static volatile int running = 1;

static char g_run_dir[PATH_MAX] = {0}; // buffer for run directory

static int g_log_fd = -1; // File descriptor for log file
static char g_log_path[PATH_MAX] = {0}; // buffer for log path
static char g_pid_path[PATH_MAX] = {0}; // buffer for pid file path

/* --------------------------- SIGNAL HANDLER ---------------------------- */

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

/* ---------------------------- SET RUNDIR ------------------------------- */

int erraid_set_rundir(const char *rundir) {
    if (!rundir || strlen(rundir) >= sizeof(g_run_dir)) {
        errno = EINVAL;
        return -1;
    }
    strncpy(g_run_dir, rundir, sizeof(g_run_dir)-1);
    return 0;
}

/* ----------------------------- UTILITIES ------------------------------- */

static int mkdir_p(const char *path) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') tmp[len - 1] = 0; // enlever le slash final

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                perror("mkdir_p");
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        perror("mkdir_p final");
        return -1;
    }
    return 0;
}

static int ensure_rundir(void) {
    if (g_run_dir[0] == '\0') return -1;

    if (mkdir_p(g_run_dir) < 0) return -1;

    char *tasksdir = make_path(g_run_dir, "tasks");
    if (!tasksdir) return -1;
    if (mkdir_p(tasksdir) < 0) {
        free(tasksdir);
        return -1;
    }
    free(tasksdir);

    snprintf(g_log_path, sizeof(g_log_path), "%s/%s", g_run_dir, LOG_NAME);
    snprintf(g_pid_path, sizeof(g_pid_path), "%s/%s", g_run_dir, PIDFILE_NAME);

    return 0;
}

/* ------------------------------- LOG ---------------------------------- */

int write_log_msg(const char *fmt, ...) {
    if (g_log_fd < 0) {
        g_log_fd = open(g_log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (g_log_fd < 0) return -1;
    }

    char buf[1024];
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);

    int off = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S ", &tm);
    if (off < 0) off = 0;

    va_list ap; 
    va_start(ap, fmt); // va_start initializes ap to retrieve the additional arguments after fmt.
    int n = vsnprintf(buf + off, sizeof(buf)-off, fmt, ap); // vsnprintf is a function that formats and stores a series of characters and values in the array buffer.
    va_end(ap); // va_end cleans up the va_list object ap.

    size_t total = off + (n > 0 ? n : 0);
    buf[total++] = '\n';

    write(g_log_fd, buf, total); 
    return 0;
}

/* ----------------------------- PID FILE -------------------------------- */

// Write current PID to pidfile
static int write_pidfile(void) {
    int fd = open(g_pid_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;
    char pbuf[32];
    int n = snprintf(pbuf, sizeof(pbuf), "%ld\n", (long)getpid());
    write(fd, pbuf, n);
    close(fd);
    return 0;
}

static void remove_pidfile(void) {
    unlink(g_pid_path);
}

/* ------------------------- EXECUTION ENGINE ----------------------------- */

/* Create run directory for task */
static int create_task_run_dir(uint32_t task_id, char *out, size_t outlen) {
    time_t now = time(NULL);

    snprintf(out, outlen, "%s/tasks/%u/runs/%ld", g_run_dir, task_id, (long)now);

    if (mkdir(out, 0755) != 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

/* Execute a simple command and write retval/stdout/stderr */
static int execute_simple(const command_t *cmd, uint32_t task_id)
{
    char run_dir[PATH_MAX];
    if (create_task_run_dir(task_id, run_dir, sizeof(run_dir)) < 0) {
        write_log_msg("Cannot create run dir for task %u", task_id);
        return -1;
    }

    /* open output files */
    char outpath[PATH_MAX], errpath[PATH_MAX];
    snprintf(outpath, sizeof(outpath), "%s/stdout", run_dir);
    snprintf(errpath, sizeof(errpath), "%s/stderr", run_dir);

    int outfd = open(outpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    int errfd = open(errpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);

    if (outfd < 0 || errfd < 0) {
        write_log_msg("Cannot open stdout/stderr for task %u", task_id);
        return -1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        /* CHILD */
        dup2(outfd, STDOUT_FILENO);
        dup2(errfd, STDERR_FILENO);

        close(outfd);
        close(errfd);

        const arguments_t *args = &cmd->args.simple;

        char **argv = arguments_to_argv(args); 
        execvp(argv[0], argv);

        dprintf(STDERR_FILENO, "execvp failed: %s\n", strerror(errno));
        exit(127);
    }

    /* PARENT */
    close(outfd);
    close(errfd);

    int status;
    waitpid(pid, &status, 0);

    /* write retval */
    char retvalpath[PATH_MAX];
    snprintf(retvalpath, sizeof(retvalpath), "%s/retval", run_dir);

    int rfd = open(retvalpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    uint32_t retval = htonl(WIFEXITED(status) ? WEXITSTATUS(status) : 255);
    write(rfd, &retval, sizeof(retval));
    close(rfd);

    write_log_msg("Task %u executed (retval=%u)", task_id, ntohl(retval));
    return 0;
}

/* Execute command: SI or SQ */
static int execute_command(const command_t *cmd, uint32_t id)
{
    if (cmd->type == SI)
        return execute_simple(cmd, id);

    if (cmd->type == SQ) {
        for (uint16_t i = 0; i < cmd->args.composed.count; i++) {
            execute_command(cmd->args.composed.cmds[i], id);
        }
        return 0;
    }
    return -1;
}

static int run_task_if_due(task_t *task)
{
    if (!task || !task->cmd || !task->timing)
        return -1;

    /* timing_should_run() handled inside timing_interpreter already */
    if (!timing_match_now(task->timing))
    write_log_msg("Task %u NOT due at this minute", task->id);
        return 0;

    write_log_msg("Executing task %u", task->id);
    return execute_command(task->cmd, task->id);
}

/* --------------------------- DAEMON INIT ------------------------------- */

int erraid_init_foreground(void) {
    ensure_rundir();

    g_log_fd = open(g_log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);

    write_pidfile();

    struct sigaction sa = {0};
    sa.sa_handler = handle_signal;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);

    write_log_msg("Foreground erraid init.");
    return 0;
}

/* ---------------------------- DAEMON MODE ------------------------------ */

int daemon_init(void) {
    if (g_run_dir[0] == '\0')
        snprintf(g_run_dir, sizeof(g_run_dir), "/tmp/%s/erraid", getenv("USER") ?: "nobody");

    ensure_rundir();

    pid_t pid = fork();
    if (pid > 0) _exit(EXIT_SUCCESS);
    if (pid < 0) return -1;

    setsid();

    pid = fork();
    if (pid > 0) _exit(EXIT_SUCCESS);
    if (pid < 0) return -1;

    umask(0); 
    chdir(g_run_dir);

    int devnull = open("/dev/null", O_RDWR);
    dup2(devnull, STDIN_FILENO);

    g_log_fd = open(g_log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    dup2(g_log_fd, STDOUT_FILENO);
    dup2(g_log_fd, STDERR_FILENO);

    write_pidfile();

    struct sigaction sa = {0};
    sa.sa_handler = handle_signal;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);

    write_log_msg("Daemon initialized.");
    return 0;
}

/* ------------------------------ MAIN LOOP ------------------------------ */

void daemon_run(void) {
    write_log_msg("Daemon main loop started.");

    while (running) {

        write_log_msg("Scanning tasks directory…");

        char tasksdir[PATH_MAX];
        snprintf(tasksdir, sizeof(tasksdir), "%s/tasks", g_run_dir);

        DIR *d = opendir(tasksdir);
        if (!d) {
            write_log_msg("Cannot open tasks/");
            sleep(SLEEP_INTERVAL);
            continue;
        }

        struct dirent *ent;
        while ((ent = readdir(d))) {
            if (ent->d_name[0] == '.')
                continue;

            uint32_t id = atoi(ent->d_name);
            if (id == 0)
                continue;

            task_t *task = task_create(id);
            if (task_reader(tasksdir, id, LIST) < 0) {
                task_destroy(task);
                continue;
            }

            run_task_if_due(task);
            task_destroy(task);
        }

        closedir(d);
        sleep(SLEEP_INTERVAL);
    }

    write_log_msg("Daemon main loop stopping.");
}

/* ------------------------------ CLEANUP -------------------------------- */

void daemon_cleanup(void) {
    write_log_msg("Cleanup.");

    if (g_log_fd >= 0)
        close(g_log_fd);

    remove_pidfile();
}

/* ------------------------------ GET RUNDIR ------------------------------ */

int erraid_get_rundir(char *out, size_t len) {
    if (!out) return -1;
    strncpy(out, g_run_dir, len);
    return 0;
}
