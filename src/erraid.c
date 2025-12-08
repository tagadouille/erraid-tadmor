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

#include <stdarg.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/file.h>

#include "types/task.h"
#include "tree-reading/tree_reader.h"


/* ---------------------------- CONFIG ---------------------------------- */

static volatile int running = 1;
static char g_run_dir[PATH_MAX] = {0};

static int g_log_fd = -1;
static char g_log_path[PATH_MAX] = "log";
static char g_pid_path[PATH_MAX] = {0};
static char tasksdir[PATH_MAX];

string_t curr_output = {0};
time_array_t* curr_time = NULL;

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
    g_run_dir[sizeof(g_run_dir)-1] = '\0';
    return 0;
}

/* ----------------------------- UTILITIES ------------------------------- */

/**
 * equivalent de realpath
 */
char *my_realpath(const char *path, char *resolved_path) {
    if (!path) { errno = EINVAL; return NULL; }

    char temp[PATH_MAX];
    if (path[0] != '/') {
        if (!getcwd(temp, sizeof(temp))) return NULL;
        size_t need = strlen(temp) + 1 + strlen(path) + 1;
        if (need > sizeof(temp)) { errno = ENAMETOOLONG; return NULL; }
        strcat(temp, "/");
        strcat(temp, path);
    } else {
        if (strlen(path) >= sizeof(temp)) { errno = ENAMETOOLONG; return NULL; }
        strncpy(temp, path, sizeof(temp));
        temp[sizeof(temp)-1] = '\0';
    }

    /* split and normalize */
    char *copy = strdup(temp);
    if (!copy) return NULL;

    char *components[PATH_MAX];
    int top = -1;

    char *saveptr = NULL;
    for (char *tok = strtok_r(copy, "/", &saveptr); tok; tok = strtok_r(NULL, "/", &saveptr)) {
        if (strcmp(tok, ".") == 0) continue;
        if (strcmp(tok, "..") == 0) {
            if (top >= 0) top--;
            continue;
        }
        components[++top] = tok;
    }

    /* build result */
    char result[PATH_MAX];
    if (top == -1) {
        /* root */
        strncpy(result, "/", sizeof(result));
        result[sizeof(result)-1] = '\0';
    } else {
        result[0] = '\0';
        for (int i = 0; i <= top; ++i) {
            size_t need = strlen(result) + 1 + strlen(components[i]) + 1;
            if (need > sizeof(result)) {
                free(copy);
                errno = ENAMETOOLONG;
                return NULL;
            }
            strcat(result, "/");
            strcat(result, components[i]);
        }
    }

    free(copy);

    if (resolved_path) {
        strncpy(resolved_path, result, PATH_MAX);
        resolved_path[PATH_MAX - 1] = '\0';
        return resolved_path;
    } else {
        return strdup(result);
    }
}

int mkdir_p(const char *path) {
    if (!path || *path == '\0') {
        errno = EINVAL;
        return -1;
    }

    char tmp[PATH_MAX];
    size_t len = strlen(path);
    if (len >= sizeof(tmp)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    strncpy(tmp, path, sizeof(tmp));
    tmp[sizeof(tmp)-1] = '\0';

    // delete the final slash
    while (len > 1 && tmp[len-1] == '/') {
        tmp[len-1] = '\0';
        len--;
    }

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }

    // create the last directory
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;

    return 0;
}

static int ensure_rundir(void) {
    if (g_run_dir[0] == '\0') return -1;

    if (mkdir_p(g_run_dir) != 0) return -1;

    //Use absolute path for g_run_dir
    char abs_rundir[PATH_MAX];
    if (!my_realpath(g_run_dir, abs_rundir)) {
        perror("realpath");
        return -1;
    }
    strncpy(g_run_dir, abs_rundir, sizeof(g_run_dir)-1);
    g_run_dir[sizeof(g_run_dir)-1] = '\0';

    size_t len = strlen(g_run_dir);

    if (len + strlen("/tasks")+1 >= sizeof(tasksdir)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    //Add a slash if necessary
    if (g_run_dir[strlen(g_run_dir)-1] == '/'){
        snprintf(tasksdir, sizeof(tasksdir), "%stasks", g_run_dir);
    }
    else{
        snprintf(tasksdir, sizeof(tasksdir), "%s/tasks", g_run_dir);
    }

    if (mkdir_p(tasksdir) != 0) return -1;

    if (snprintf(g_pid_path, sizeof(g_pid_path), "%s/%s", g_run_dir, PIDFILE_NAME) < 0) return -1;

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
    /* use UTC so tests are deterministic */
    if (gmtime_r(&now, &tm) == NULL) return -1;

    int off = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S ", &tm);
    if (off < 0) off = 0;

    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf + off, (size_t)sizeof(buf) - (size_t)off, fmt, ap);
    va_end(ap);

    if (n < 0) return -1;
    ssize_t total = (off + n);
    if (total + 1 < (ssize_t)sizeof(buf)) buf[total++] = '\n';
    else buf[sizeof(buf)-1] = '\0';

    if (write(g_log_fd, buf, total) != total) {
        return -1;
    }
    return 0;
}

/* ----------------------------- PID FILE -------------------------------- */

static int write_pidfile(void) {
    if (g_pid_path[0] == '\0') return -1;
    int fd = open(g_pid_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;
    char pbuf[32];
    int n = snprintf(pbuf, sizeof(pbuf), "%ld\n", (long)getpid());
    if (n > 0) {
        ssize_t w = write(fd, pbuf, (size_t)n);
        if (w < 0 || w != n) {
            perror("write");
        }
    }
    close(fd);
    return 0;
}
static void remove_pidfile(void) {
    if (g_pid_path[0]) unlink(g_pid_path);
}

/* helper hton64/ntoh64 (portable) */
static uint64_t hton64(uint64_t x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return ((uint64_t)htonl((uint32_t)(x & 0xffffffffULL)) << 32) | htonl((uint32_t)(x >> 32));
#else
    return x;
#endif
}

/* ------------------------- EXECUTION ENGINE ----------------------------- */

/* Append one record to times-exitcodes: [be64 timestamp][be32 exitcode] atomically+safe */
static int append_times_exitcodes(const char* path, int exitcode) {

    int fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd < 0) return -1;

    struct __attribute__((packed)) rec {
        uint64_t ts;
        uint32_t code;
    } r;

    r.ts = hton64((uint64_t)time(NULL));
    r.code = htonl((uint32_t)exitcode);

    ssize_t w = write(fd, &r, sizeof(r));
    if (w != sizeof(r)) {
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


static int execute_complexe(const command_t *cmd, const char *timespath, int outfd, int errfd)
{
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

/* Execute command: SI or SQ */
static int execute_command(const command_t *cmd, const char *timespath, int outfd, int errfd)
{
    if (!cmd) return -1;

    if (cmd->type == SI)
        return execute_simple(cmd, timespath, outfd, errfd, 0);

    if (cmd->type == SQ)
        return execute_complexe(cmd, timespath, outfd, errfd);

    return -1;
}


/* Execute command: SI or SQ */
static int run_task_if_due(task_t *task)
{
    if (!task || !task->cmd || !task->timing) return -1;

    if (!timing_match_now(task->timing)) {
        write_log_msg("Task %u NOT due at this minute", task->id);
        return 0;
    }

    write_log_msg("Executing task %u", task->id);

    /* use correct format for uint32_t id */
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

    int outfd = open(outpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    int errfd = open(errpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);

    if (outfd < 0 || errfd < 0) {
        write_log_msg("Cannot open stdout/stderr for task %u: %s / %s", task->id, strerror(errno), tasksdir);
        if (outfd >= 0) close(outfd);
        if (errfd >= 0) close(errfd);
        return -1;
    }

    int res = execute_command(task->cmd, timespath, outfd, errfd);

    close(outfd);
    close(errfd);

    return res;
}

/* ---------------------------- DAEMON MODE ------------------------------ */

int daemon_init(void) {
    if (g_run_dir[0] == '\0') {
        const char *user = getenv("USER");
        if (!user) user = "nobody";
        snprintf(g_run_dir, sizeof(g_run_dir), "/tmp/%s/erraid", user);
    }

    if (ensure_rundir() != 0) return -1;

    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid > 0) _exit(EXIT_SUCCESS);

    if (setsid() < 0) return -1;

    pid = fork();
    if (pid < 0) return -1;
    if (pid > 0) _exit(EXIT_SUCCESS);

    umask(0);
    if (chdir(g_run_dir) != 0) { fprintf(stderr, "chdir failed: %s\n", strerror(errno)); }

    g_log_fd = open(g_log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (g_log_fd < 0) {
        fprintf(stderr, "Cannot open log file '%s': %s\n", g_log_path, strerror(errno));
        return -1;
    }

    dup2(g_log_fd, STDOUT_FILENO);
    dup2(g_log_fd, STDERR_FILENO);
    

    write_pidfile();

    struct sigaction sa = {0};
    sa.sa_handler = handle_signal;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);

    write_log_msg("Daemon initialized (rundir=%s)", g_run_dir);
    return 0;
}

/* ------------------------------ Timing util ---------------------------- */
/* wait until the next minute boundary (sleep until seconds == 0) */
static void wait_next_minute(void) {
    time_t now = time(NULL);
    struct tm tm_now;

    if (localtime_r(&now, &tm_now) == NULL) {
        sleep(60);
        return;
    }

    int sec = tm_now.tm_sec;
    int wait = 60 - sec;
    if (wait <= 0) wait = 60;

    for (int i = 0; i < wait && running; ++i)
        sleep(1);
}

/* ------------------------------ MAIN LOOP ------------------------------ */

void daemon_run(void) {
    write_log_msg("Daemon main loop started.");

    while (running) {
        write_log_msg("Scanning tasks directory…");

        DIR *d = opendir(tasksdir);

        if (d == NULL) {
            perror("opendir");
            write_log_msg("Cannot open tasks/ directory: %s", tasksdir);
            sleep(SLEEP_INTERVAL);
            continue;
        }

        struct dirent *ent;
        while ((ent = readdir(d))) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

            char *endptr = NULL;
            errno = 0;
            unsigned long idul = strtoul(ent->d_name, &endptr, 10);
            if (endptr == NULL || *endptr != '\0' || errno != 0) continue;
            uint32_t id = (uint32_t)idul;

            /* use existing tree reader which sets curr_task */
            if (task_reader(tasksdir, id, LIST) < 0) {
                write_log_msg("task_reader failed for %u", id);
                continue;
            }else{
                write_log_msg("task_reader worked for %u", id);
            }
            if (!curr_task) {
                write_log_msg("No curr_task for id %u", id);
                continue;
            }
            
            //task_display(curr_task);

            run_task_if_due(curr_task);
            task_destroy(curr_task);
            curr_task = NULL;
        }
        closedir(d);

        wait_next_minute();
    }

    write_log_msg("Daemon main loop stopping.");
}

/* ------------------------------ CLEANUP -------------------------------- */

void daemon_cleanup(void) {
    write_log_msg("Cleanup.");

    if (g_log_fd >= 0) close(g_log_fd);
    remove_pidfile();
}

/* ------------------------------ GET RUNDIR ------------------------------ */

int erraid_get_rundir(char *out, size_t len) {
    if (!out || len == 0) { errno = EINVAL; return -1; }
    strncpy(out, g_run_dir, len-1);
    out[len-1] = '\0';
    return 0;
}