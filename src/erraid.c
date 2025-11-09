#include "erraid.h"

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
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    sid = setsid();
    if (sid < 0) {
        perror("setsid failed");
        return -1;
    }

    if (chdir("/") < 0) {
        perror("chdir failed");
        return -1;
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    write_log("Erraid daemon initialized.");
    return 0;
}

void daemon_run(void) {
    write_log("Erraid daemon started.");

    while (running) {
        // (plus tard) lecture de l’arborescence et exécution des tâches
        write_log("Daemon is alive...");
        sleep(SLEEP_INTERVAL);
    }
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
