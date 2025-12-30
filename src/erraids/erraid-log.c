#include "erraids/erraid.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#include "erraids/erraid-log.h"

/* ------------------------------- LOG ---------------------------------- */

int write_log_msg(const char *fmt, ...) {
    if (g_log_fd < 0) {
        g_log_fd = open(g_log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);

        if (g_log_fd < 0){
            perror("open");
            return -1;
        }
    }

    char buf[1024];
    time_t now = time(NULL);
    struct tm tm;

    if (gmtime_r(&now, &tm) == NULL) return -1;

    int off = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S ", &tm);

    if (off < 0){
        off = 0;
    }

    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf + off, (size_t)sizeof(buf) - (size_t)off, fmt, ap);
    va_end(ap);

    if (n < 0){
        return -1;
    } 
    ssize_t total = (off + n);

    if (total + 1 < (ssize_t)sizeof(buf)){
        buf[total++] = '\n';
    }
    else{
        buf[sizeof(buf)-1] = '\0';
    }

    if (write(g_log_fd, buf, total) != total) {
        perror("write");
        return -1;
    }
    return 0;
}