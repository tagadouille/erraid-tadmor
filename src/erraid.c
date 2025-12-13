#include "erraid.h"
#include "erraids/erraid-helper.h"
#include "erraids/erraid-log.h"
#include "erraids/erraid-executor.h"

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

/**
 * The current log path
 */
char g_log_path[PATH_MAX] = "log";
/**
 * The current running directory
 */
char g_run_dir[PATH_MAX] = {0};

int g_log_fd = -1;
char tasksdir[PATH_MAX] = "";

/* --------------------------- SIGNAL HANDLER ---------------------------- */

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
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

    struct sigaction sa = {0};
    sa.sa_handler = handle_signal;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);

    write_log_msg("Daemon initialized (rundir=%s)", g_run_dir);
    return 0;
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


/* ------------------------------ Timing util ---------------------------- */
/* wait until the next minute boundary (sleep until seconds == 0) */
void wait_next_minute(void)
{
    struct timespec ts;

    // Lire temps actuel
    clock_gettime(CLOCK_REALTIME, &ts);

    // Arrondir à la minute suivante
    ts.tv_sec = ts.tv_sec - (ts.tv_sec % 60) + 60;
    ts.tv_nsec = 0;

    // Dormir jusqu'à cette date absolue
    while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts, NULL) == EINTR);
}


/* ------------------------------ MAIN LOOP ------------------------------ */
static void scan_directory(DIR* dirp){

    if(dirp == NULL) {
        write_log_msg("Error : The DIR struct can't be null");
        running = 0;
        return;
    }

    struct dirent *ent;
    while ((ent = readdir(dirp))) {
        if (ent->d_name[0] == '.') continue;

        // Conversion of char* to uint64_t
        char *endptr = NULL;
        errno = 0;
        unsigned long idul = strtoul(ent->d_name, &endptr, 10);
        if (endptr == NULL || *endptr != '\0' || errno != 0) continue;
        uint64_t id = (uint64_t)idul;

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
    closedir(dirp);
}
void daemon_run(void) {
    write_log_msg("Daemon main loop started.");

    while (running) {

        wait_next_minute();

        write_log_msg("Scanning tasks directory…");

        DIR *d = opendir(tasksdir);

        if (d == NULL) {
            perror("opendir");
            write_log_msg("Cannot open tasks/ directory: %s", tasksdir);
            sleep(SLEEP_INTERVAL);
            continue;
        }

        scan_directory(d);       
    }

    write_log_msg("Daemon main loop stopping.");
}