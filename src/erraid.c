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

#include <stdarg.h>    // va_list
#include <arpa/inet.h> // htonl / ntohl
#include <dirent.h>    // opendir / readdir

#include "types/task.h"
#include "tree-reading/tree_reader.h"

/* ---------------------------- CONFIG ---------------------------------- */

static volatile int running = 1;
static char g_run_dir[PATH_MAX] = {0};

static int g_log_fd = -1;
static char g_log_path[PATH_MAX] = "log";
static char g_pid_path[PATH_MAX] = {0};
static char tasksdir[PATH_MAX];

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
    if (!realpath(g_run_dir, abs_rundir)) {
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
    localtime_r(&now, &tm);

    int off = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S ", &tm);
    if (off < 0) off = 0;

    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf + off, (size_t)sizeof(buf) - (size_t)off, fmt, ap);
    va_end(ap);

    if (n < 0) return -1;
    ssize_t total = (off + n);
    if (total + 1 < sizeof(buf)) buf[total++] = '\n';
    else buf[sizeof(buf)-1] = '\0';

    if(write(g_log_fd, buf, total) != total){
        perror("write");
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

/* Return path "RUN_DIR/tasks/<id>", ensure it exists (caller must free) */
static char *task_dir_path(uint32_t task_id) {
    char *path = malloc(PATH_MAX);
    if (!path) return NULL;
    if (snprintf(path, PATH_MAX, "%s/%u", g_run_dir, task_id) < 0) {
        free(path);
        return NULL;
    }
    if (mkdir_p(path) != 0) {
        /* if fail, still return path (might already exist) */
        /* but if not exist and can't create, return NULL */
        struct stat st;
        if (stat(path, &st) != 0) {
            free(path);
            return NULL;
        }
    }
    return path;
}

/* Append one record to times-exitcodes: [be64 timestamp][be32 exitcode] */
static int append_times_exitcodes(uint32_t task_id, int exitcode) {
    char *td = task_dir_path(task_id);
    if (!td) return -1;
    char te_path[PATH_MAX];
    snprintf(te_path, sizeof(te_path), "%s/times-exitcodes", td);

    int fd = open(te_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd < 0) {
        free(td);
        return -1;
    }

    uint64_t now = (uint64_t)time(NULL);
    uint64_t be_ts = hton64(now);
    uint16_t code16 = (uint16_t)exitcode;
    uint16_t be_code = htons(code16); /* 16-bit network order (big-endian) */

    if (write(fd, &be_ts, sizeof(be_ts)) != (ssize_t)sizeof(be_ts)) { close(fd); free(td); return -1; }
    if (write(fd, &be_code, sizeof(be_code)) != (ssize_t)sizeof(be_code)) { close(fd); free(td); return -1; }


    close(fd);
    free(td);
    return 0;
}

/* Execute a simple command and write stdout/stderr into task dir (overwrite),
   append times-exitcodes entry. */
static int execute_simple(const command_t *cmd, uint32_t task_id, int outfd, int errfd)
{
    if (!cmd) return -1;

    pid_t pid = fork();
    if (pid < 0) {
        write_log_msg("fork failed: %s", strerror(errno));
        close(outfd);
        close(errfd);
        return -1;
    }

    if (pid == 0) {
        /* CHILD: redirect stdout/stderr */
        if (dup2(outfd, STDOUT_FILENO) < 0) _exit(127);
        if (dup2(errfd, STDERR_FILENO) < 0) _exit(127);
        close(outfd);
        close(errfd);

        const arguments_t *args = &cmd->args.simple;
        if (!args || !args->command) _exit(127);

         char **argv = arguments_to_argv(args);
        if (!argv) _exit(127);

        execvp(argv[0], argv);
        /* if execvp returns, it's an error */
        dprintf(STDERR_FILENO, "execvp failed: %s\n", strerror(errno));

        /* free argv before exit (child will exit anyway) */
        for (size_t i = 0; argv[i]; ++i) free(argv[i]);
        free(argv);
        _exit(127);
    }

    /* PARENT */
    close(outfd);
    close(errfd);

    int status = 0;
    waitpid(pid, &status, 0);
    int exitcode = WIFEXITED(status) ? WEXITSTATUS(status) : 255;

    if (append_times_exitcodes(task_id, exitcode) < 0)
        write_log_msg("Failed to append times-exitcodes for %u", task_id);

    write_log_msg("Task %u executed (retval=%d)", task_id, exitcode);

    return 0;
}

/* Execute command: SI or SQ */
static int execute_command(const command_t *cmd, uint32_t id, int outfd, int errfd){
    if (!cmd) return -1;

    if (outfd < 0 || errfd < 0) {
        if (outfd >= 0) close(outfd);
        if (errfd >= 0) close(errfd);
        return -1;
    }

    if (cmd->type == SI) {
        return execute_simple(cmd, id, outfd, errfd);
    } else if (cmd->type == SQ) {
        uint16_t count = cmd->args.composed.count;
        for (uint16_t i = 0; i < count; ++i) {
            command_t *sub = cmd->args.composed.cmds[i];
            if (execute_command(sub, id, outfd, errfd) < 0) {
                /* continue executing sequence even if one fails (spec choice) */
                write_log_msg("Subcommand %u of sequence failed (task %u)", i, id);
            }
        }
        return 0;
    }
    return -1;
}

static int run_task_if_due(task_t *task)
{
    if (!task || !task->cmd || !task->timing) return -1;

    if (!timing_match_now(task->timing)) {
        write_log_msg("Task %u NOT due at this minute", task->id);
        return 0;
    }

    write_log_msg("Executing task %u", task->id);

    /* prepare task directory */
    char *td = task_dir_path(task -> id);
    if (!td) {
        write_log_msg("Cannot locate/create task directory for %u", task->id);
        return -1;
    }

    char outpath[PATH_MAX], errpath[PATH_MAX];
    snprintf(outpath, sizeof(outpath), "%s/stdout", td);
    snprintf(errpath, sizeof(errpath), "%s/stderr", td);

    int outfd = open(outpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    int errfd = open(errpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);

    return execute_command(task->cmd, task->id, outfd, errfd);
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
    struct timespec ts;
    struct tm tm_now;
    time_t now = time(NULL);
    localtime_r(&now, &tm_now);
    int sec = tm_now.tm_sec;
    int wait = 60 - sec;
    if (wait <= 0) wait = 60;
    /* sleep in small steps so signals can interrupt */
    for (int i = 0; i < wait && running; ++i) sleep(1);
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
            if (task_reader(tasksdir, id, OUTPUT) < 0) {
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