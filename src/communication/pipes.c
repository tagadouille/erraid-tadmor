#define _POSIX_C_SOURCE 200809L
#include "communication/pipes.h"
#include "communication/communication.h"
#include "erraids/erraid-helper.h"

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


static char* pipe_file_path_creator(){
    const char *user = getenv("USER");
    if (!user) user = "nobody";

    char* path = make_path_no_test("/home", user);

    if(path == NULL){
        dprintf(STDERR_FILENO, "The path is NULL at pipe_file_read\n");
        return NULL;
    }
    char* pipe_file_path = make_path_no_test(path, PIPE_FILE);

    if(pipe_file_path == NULL){
        dprintf(STDERR_FILENO, "The pipe_file_path is NULL at pipe_file_read\n");
    }

    free(path);

    if(pipe_file_path != NULL){
        dprintf(STDOUT_FILENO, "The pipe_file_path is %s\n", pipe_file_path);
    }
    return pipe_file_path;
}
int pipe_file_write(){

    char* pipe_file_path = pipe_file_path_creator();

    int fd = open(pipe_file_path, O_CREAT | O_TRUNC | O_WRONLY, 0677);

    free(pipe_file_path);

    if(fd < 0){
        perror("open");
        return -1;
    }

    if(write(fd, pipe_path, sizeof(pipe_path)) != sizeof(pipe_path)){
        perror("write");
        return -1;
    }
    return 0;
}

int pipe_file_read(){

    char* pipe_file_path = pipe_file_path_creator();

    int fd = open(pipe_file_path, O_RDONLY);

    free(pipe_file_path);

    if(fd < 0){
        perror("open");
        return -1;
    }

    ssize_t nread = read(fd, pipe_path, sizeof(pipe_path));

    if(nread < 0){
        perror("read");
        return -1;
    }

    if(nread == 0){
        dprintf(STDERR_FILENO, "The pipe_path is empty\n");
    }
    return 0;
}


/**
 * @brief rename the pipe_path
 * @param new_path the new path name
 * @return 0 if succes, -1 if failure
 */
int pipe_path_rename(char* new_path){
    
    if(new_path == NULL){
        dprintf(STDERR_FILENO, "Error : the new_path can't be null\n");
        return -1;
    }

    // delete the pipes if they already exit
    char* reply = make_path_no_test(pipe_path, REPLY_PIPE);
    char* request = make_path_no_test(pipe_path, REQUEST_PIPE);

    if(reply == NULL || request == NULL){
        dprintf(STDERR_FILENO, "Error : make_path_no_test fail at pipe_path_rename\n");
        return -1;
    }

    if(unlink(request) < 0 && errno != ENOENT){
        dprintf(STDERR_FILENO, "Error : can't delete the request pipe, it's not serious\n");
    }
    if(unlink(reply) < 0 && errno != ENOENT){
        dprintf(STDERR_FILENO, "Error : can't delete the request pipe, it's not serious\n");
    }

    free(request);
    free(reply);

    strcpy(pipe_path, new_path);

    if (pipe_path[0] == '\0') return -1;

    if (mkdir_p(pipe_path) != 0) return -1;

    //Use absolute path for pipe_path
    char abs_pipe_path[PATH_MAX];

    if (!my_realpath(pipe_path, abs_pipe_path)) {
        perror("realpath");
        return -1;
    }

    size_t len = strlen(abs_pipe_path);

    if (len >= sizeof(pipe_path))
        return -1;

    memcpy(pipe_path, abs_pipe_path, len + 1);

    len = strlen(pipe_path);
    const char *suffix = "/pipes";
    size_t suffix_len = strlen(suffix);

    if (len + suffix_len + 1 > sizeof(pipe_path)) {
        return -1;
    }

    memcpy(pipe_path + len, suffix, suffix_len + 1);

    pipe_path[sizeof(pipe_path) - 1] = '\0';

    return 0;
}

/* ===========================
   DÉMON : création
   =========================== */
int daemon_setup_pipes()
{
    char *req_path = make_path_no_test(pipe_path, REQUEST_PIPE);

    char *rep_path = make_path_no_test(pipe_path, REPLY_PIPE);

    if(req_path == NULL){
        dprintf(STDERR_FILENO, "req_path is null at daemon_setup_pipes");
        return -1;
    }

    if(rep_path == NULL){
        dprintf(STDERR_FILENO, "rep_path is null at daemon_setup_pipes");
        return -1;
    }

    /* mkfifo si n’existent pas */
    if (mkfifo(req_path, 0666) != 0 && errno != EEXIST)
        return -1;

    if (mkfifo(rep_path, 0666) != 0 && errno != EEXIST)
        return -1;

    free(req_path);
    free(rep_path);
    return 0;
}

int daemon_open_reply(int *rep_wr)
{
    char *rep_path = make_path(pipe_path, REPLY_PIPE);
    if (!rep_path)return -1;

    dprintf(STDOUT_FILENO, "[daemon] The pipe reply path is %s\n", rep_path);
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

int client_open_request(int *req_wr){

    char *req_path = make_path_no_test(pipe_path, REQUEST_PIPE);

    if(req_path == NULL){
        dprintf(STDERR_FILENO, "Error : an error occured while creating the path to the request pipe\n");
        return -1;
    }

    /* bloque jusqu’à ce que le démon ait open() en lecture */
    int w = open(req_path, O_WRONLY);
    if (w < 0)
        return -1;

    *req_wr = w;

    free(req_path);
    return 0;
}

int client_open_reply(int *rep_rd)
{
    char *rep_path = make_path(pipe_path, REPLY_PIPE);
    if( !rep_path)return -1;

    /* bloque jusqu’à ce que le démon ait open() en écriture */
    int r = open(rep_path, O_RDONLY);

    dprintf(STDOUT_FILENO, "[client] The pipe reply path is %s\n", rep_path);

    if (r < 0)
        return -1;

    *rep_rd = r;
    free(rep_path);
    return 0;
}
