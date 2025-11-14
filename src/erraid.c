#include "erraid.h"
#include "tree-reading/tree_reader.h"

static volatile int running = 1;

void handle_signal(int sig) {
    running = 0;
}

int daemon_init(void) {
    pid_t pid, sid;

    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    }

    // Parent process exits
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Child process continues
    sid = setsid();
    if (sid < 0) {
        perror("setsid failed");
        return -1;
    }

    umask(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    signal(SIGTERM, handle_signal);

    write_log("Daemon initialized.");
    return 0;
}

void daemon_run(void) {
    write_log("Daemon started.");

    const char *task_root = "../Consignes/exemples-arborescences/exemple-arborescence-1/";  

    while (running) {
        write_log("Scanning task tree...");

        // Reading the task tree
        int result = task_reader(task_root, 0, LIST); 
        if (result < 0) {
            write_log("Error while reading task tree.");
        } else {
            write_log("Task tree successfully read.");
        }

        sleep(SLEEP_INTERVAL);
    }

    write_log("Erraid daemon stopped.");
}


void daemon_cleanup(void) {
    write_log("Erraid daemon stopped.");
}

void write_log(const char *message) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;
    time_t now = time(NULL);
    fprintf(log, "[%ld] %s\n", now, message);
    fclose(log);
}
