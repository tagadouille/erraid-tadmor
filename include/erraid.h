#ifndef ERRAID_H
#define ERRAID_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define LOG_FILE "/tmp/erraid.log"
#define SLEEP_INTERVAL 60

int daemon_init(void);
void daemon_run(void);
void daemon_cleanup(void);
void write_log(const char *message);

#endif 
