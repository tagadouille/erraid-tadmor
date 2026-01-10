#include "erraids/erraid.h"
#include "erraids/erraid-scanner.h"
#include "erraids/erraid-helper.h"
#include "erraids/erraid-servant.h"
#include "communication/pipes.h"
#include "communication/communication.h"

#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/wait.h>


/* ---------------------------- CONFIG ---------------------------------- */

task_t* curr_task = NULL;

volatile int running = 1;

static pid_t servant_pid = -1;


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

string_t* curr_output = NULL;
time_array_t* curr_time = NULL;

int g_foreground_mode = 0;

/**
 * @brief Start the servant process
 */
static void start_son(){

    servant_pid = fork();

    if (servant_pid < 0) {
        perror("fork");
        return;
    }

    if (servant_pid == 0) {
        // servant
        start_serve(getppid());
        _exit(EXIT_SUCCESS);
    }
}

/* --------------------------- SIGNAL HANDLER ---------------------------- */

static void handle_sigchld(int sig) {
    (void)sig;
    int saved_errno = errno;
    pid_t pid;

    // Reap all terminated children to avoid zombies
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        if (pid == servant_pid) {
            servant_pid = -1; // Mark servant as dead
        }
    }
    errno = saved_errno;
}

static void handle_sigusr2(int sig){
    (void)sig;
    int saved_errno = errno;
    pid_t pid;

    // Reap all terminated children to avoid zombies
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        if (pid == servant_pid) {
            servant_pid = -1; // Mark servant as dead
        }
    }

    dprintf(STDOUT_FILENO, "Handled SIGUSR2, reaped child processes.\n");

    start_son();

    errno = saved_errno;
}

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

    if(write_pid_file() < 0){
        dprintf(STDERR_FILENO, "Error : can't write the pid file\n");
        return -1;
    }

    if (mkdir_p(pipe_path) != 0) {
        perror("mkdir_p(pipe_path)");
        return -1;
    }

    if(pipe_file_write() < 0){
        dprintf(STDERR_FILENO, "Error : can't write in the pipe_file\n");
        return -1;
    }

    if (!g_foreground_mode) {
        // Daemonize only if not in foreground mode
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
    }else{
        // In foreground mode
        write_log_msg("[erraid] Running in foreground mode");
    }

    if (g_foreground_mode) {
        struct sigaction sa_tstp = {0};
        sa_tstp.sa_handler = SIG_IGN;
        sigaction(SIGTSTP, &sa_tstp, NULL);
    }   

    struct sigaction sa_chld = {0};
    sa_chld.sa_handler = handle_sigchld;
    sigaction(SIGCHLD, &sa_chld, NULL);

    struct sigaction sa_usr = {0};
    sa_usr.sa_handler = handle_sigusr2;
    sigaction(SIGUSR2, &sa_usr, NULL);

    struct sigaction sa = {0};
    sa.sa_handler = handle_signal;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);

    write_log_msg("Daemon initialized (rundir=%s)", g_run_dir);
    return 0;
}

/* ------------------------------ MAIN LOOP ------------------------------ */

void daemon_run(void) {

    start_son();

    // scanner (father)
    erraid_scan_loop();

    daemon_cleanup();
}