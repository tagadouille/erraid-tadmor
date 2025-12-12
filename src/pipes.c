#define _POSIX_C_SOURCE 200809L
#include "pipes.h"

#include <sys/stat.h>
#include <sys/types.h>
#include "tree-reading/tree_reader.h"

#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

/* ===========================
   DÉMON : création + ouverture
   =========================== */

int daemon_setup_pipes(const char *rundir, int *req_rd)
{
    char pipes_dir[PATH_MAX];
    snprintf(pipes_dir, sizeof(pipes_dir), "%s/pipes", rundir);

    /* créer répertoire pipes */
    if (mkdir(pipes_dir, 0777) != 0 && errno != EEXIST)
        return -1;

    char *req_path = make_path(pipes_dir, "erraid-request-pipe");

    char *rep_path = make_path(pipes_dir, "erraid-reply-pipe");

    /* mkfifo si n’existent pas */
    if (mkfifo(req_path, 0666) != 0 && errno != EEXIST)
        return -1;

    if (mkfifo(rep_path, 0666) != 0 && errno != EEXIST)
        return -1;

    /* ouvrir pipe de requêtes en lecture BLOQUANT */
    int r = open(req_path, O_RDONLY);
    if (r < 0)
        return -1;

    *req_rd = r;
    free(req_path);
    free(rep_path);
    return 0;
}

int daemon_open_reply(const char *rundir, int *rep_wr)
{
    char pipes_dir[PATH_MAX];
    snprintf(pipes_dir, sizeof(pipes_dir), "%s/pipes", rundir);

    char *rep_path = make_path(pipes_dir, "erraid-reply-pipe");
    if (!rep_path)return -1;
    /* ouvrir N’IMPORTE QUAND nécessaire */
    int w = open(rep_path, O_WRONLY);
    if (w < 0)
        return -1;

    *rep_wr = w;
    free(rep_path);
    return 0;
}

/* ===========================
   CLIENT : ouverture des pipes
   =========================== */

int client_open_request(const char *rundir, int *req_wr)
{
    char pipes_dir[PATH_MAX];
    snprintf(pipes_dir, sizeof(pipes_dir), "%s/pipes", rundir);

    char *req_path = make_path(pipes_dir, "erraid-request-pipe");
    if( !req_path)return -1;

    /* bloque jusqu’à ce que le démon ait open() en lecture */
    int w = open(req_path, O_WRONLY);
    if (w < 0)
        return -1;

    *req_wr = w;
    free(req_path);
    return 0;
}

int client_open_reply(const char *rundir, int *rep_rd)
{
    char pipes_dir[PATH_MAX];
    snprintf(pipes_dir, sizeof(pipes_dir), "%s/pipes", rundir);

    char *rep_path = make_path(pipes_dir, "erraid-reply-pipe");
    if( !rep_path)return -1;

    /* bloque jusqu’à ce que le démon ait open() en écriture */
    int r = open(rep_path, O_RDONLY);
    if (r < 0)
        return -1;

    *rep_rd = r;
    free(rep_path);
    return 0;
}
