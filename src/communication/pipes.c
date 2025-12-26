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

/**
 * @brief create a string that contains the default pipe_path 
 * @return the string
 */
static char* pipe_file_path_creator() {

    const char *user = getenv("USER");
    if (!user) user = "nobody";

    char* path = make_path_no_test("/home", user);

    if(path == NULL){
        dprintf(STDERR_FILENO, "The path is NULL at pipe_file_read\n");
        return NULL;
    }
    
    char* pipe_file_path = make_path_no_test(path, PIPE_FILE);
    free(path);
    
    if(pipe_file_path == NULL){
        dprintf(STDERR_FILENO, "The pipe_file_path is NULL at pipe_file_read\n");
        return NULL;
    }
    
    dprintf(STDOUT_FILENO, "DEBUG: pipe_file_path_creator() -> %s\n", pipe_file_path);
    return pipe_file_path;
}

int pipe_file_write() {

    char* pipe_file_path = pipe_file_path_creator();

    if (!pipe_file_path){
        dprintf(2, "pipe_file_path_creator failed\n");
        return -1;
    }
    
    dprintf(STDOUT_FILENO, "DEBUG: Writing pipe_path='%s' to file '%s'\n", 
            pipe_path, pipe_file_path);
    
    int fd = open(pipe_file_path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    free(pipe_file_path);
    
    if(fd < 0){
        perror("open pipe file for writing");
        return -1;
    }
    
    // Écrit la chaîne, pas le pointeur !
    size_t len = strlen(pipe_path) + 1; // +1 pour le '\0'
    ssize_t written = write(fd, pipe_path, len);
    
    close(fd);
    
    if(written != (ssize_t)len){
        dprintf(STDERR_FILENO, "Error: wrote %zd bytes, expected %zu\n", written, len);
        return -1;
    }
    
    dprintf(STDOUT_FILENO, "DEBUG: Successfully wrote pipe_path to file\n");
    return 0;
}

int pipe_file_read() {
    char* pipe_file_path = pipe_file_path_creator();
    if (!pipe_file_path) return -1;
    
    dprintf(STDOUT_FILENO, "DEBUG: Reading pipe path from '%s'\n", pipe_file_path);
    
    int fd = open(pipe_file_path, O_RDONLY);
    free(pipe_file_path);
    
    if(fd < 0){
        perror("open pipe file for reading");
        return -1;
    }
    
    // Lit la chaîne complète
    ssize_t nread = read(fd, pipe_path, PATH_MAX - 1);
    close(fd);
    
    if(nread < 0){
        perror("read pipe file");
        return -1;
    }
    
    if(nread == 0){
        dprintf(STDERR_FILENO, "The pipe file is empty\n");
        pipe_path[0] = '\0';
        return -1;
    }
    
    // Assure la terminaison nulle
    pipe_path[nread] = '\0';
    
    dprintf(STDOUT_FILENO, "DEBUG: Read pipe_path='%s' (%zd bytes)\n", pipe_path, nread);
    return 0;
}

int pipe_path_rename(char* new_path) {
    if(new_path == NULL || new_path[0] == '\0'){
        dprintf(STDERR_FILENO, "Error: new_path cannot be null or empty\n");
        return -1;
    }
    
    // Convertir "." en chemin absolu
    char base_path[PATH_MAX];
    if (strcmp(new_path, ".") == 0) {
        if (getcwd(base_path, sizeof(base_path)) == NULL) {
            perror("getcwd");
            return -1;
        }
    } else {
        strncpy(base_path, new_path, sizeof(base_path) - 1);
        base_path[sizeof(base_path) - 1] = '\0';
    }
    
    // Construire le chemin complet avec /pipes
    char new_pipe_path[PATH_MAX];
    snprintf(new_pipe_path, sizeof(new_pipe_path), "%s/pipes", base_path);
    
    // S'assurer que ce n'est pas trop long
    if (strlen(new_pipe_path) >= PATH_MAX) {
        dprintf(STDERR_FILENO, "Error: path too long\n");
        return -1;
    }
    
    dprintf(STDOUT_FILENO, "DEBUG: Setting pipe_path to '%s'\n", new_pipe_path);
    
    // Mettre à jour la variable globale
    strcpy(pipe_path, new_pipe_path);
    
    // Créer le répertoire
    if (mkdir_p(pipe_path) != 0) {
        dprintf(STDERR_FILENO, "Error: failed to create directory\n");
        return -1;
    }
    
    // Sauvegarder dans le fichier
    return pipe_file_write();
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
    if(pipe_file_read() < 0){
        dprintf(2, "Error : an error occured while reading the pipe_file\n");
        return -1;
    }
    
    if(daemon_setup_pipes() < 0){
        dprintf(2, "Error : an error occured while reading the pipe_file\n");
        return -1;
    }

    char *rep_path = make_path(pipe_path, REPLY_PIPE);
    if (!rep_path)return -1;

    dprintf(STDOUT_FILENO, "[daemon] The pipe reply path is %s\n", rep_path);

    int w = open(rep_path, O_WRONLY);

    if (w < 0){
        perror("write");
        return -1;
    }  

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
    if (w < 0){
        perror("open");
        return -1;
    }   

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
