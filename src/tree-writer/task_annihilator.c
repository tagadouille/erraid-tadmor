#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

/**
 * @brief Delete reccursively all the files and directories
 * 
 * @param path path to the directory to delete
 * @return 0 if success, -1 if error
 */
static int remove_directory_contents(const char *path) {

    DIR *dir;
    struct dirent *entry;

    struct stat statbuf;

    char full_path[PATH_MAX];

    int result = 0;
    
    if (!path) {
        dprintf(2, "Error : the path can't be null\n");
        return -1;
    }
    
    dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return -1;
    }
    
    // Read all the entries :
    while ((entry = readdir(dir)) != NULL && result == 0) {

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Make the full path :
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (lstat(full_path, &statbuf) == -1) {
            dprintf(2, "Error : lstat fail for '%s': %s\n", full_path, strerror(errno));
            continue;
        }
        
        // Process according to the file type :

        // Directory
        if (S_ISDIR(statbuf.st_mode)) {
            
            // Delete the content :
            if (remove_directory_contents(full_path) == -1) {
                dprintf(2, "Error : deletion of the sub-directory failed '%s'\n", full_path);
                result = -1;
                break;
            }
            
            // Delete the directory :
            if (rmdir(full_path) == -1) {
                dprintf(2, "Error: failed to delete the directory '%s': %s\n", full_path, strerror(errno));
                result = -1;
                break;
            }
            
        } else {
            // If it's a file or a symbolic link :
            if (unlink(full_path) == -1) {
                result = -1;
                dprintf(2, "Error : failed to delete the file '%s' : %s\n", full_path, strerror(errno));
                break;
            }
        }
    }
    
    if (closedir(dir) == -1) {
        dprintf(2, "Error : Erreur when closing the directory '%s': %s\n", path, strerror(errno));
    }
    
    if (result == 0) {
        dprintf(1, "Success: Content of '%s' completely delete\n", path);
    } else {
        dprintf(2, "Failure : Error while delete the content of '%s'\n", path);
    }
    
    return result;
}

int delete_directory(const char *path) {

    struct stat statbuf;
    
    if (!path) {
        dprintf(2, "Error : the path can't be null\n");
        return -1;
    }
    
    dprintf(1, "=== Start : Deletion of '%s' ===\n", path);
    
    // Some verifications :
    if (access(path, F_OK) == -1) {
        dprintf(2, "Error : The path'%s' doesn't exist\n", path);
        return -1;
    }
    
    if (stat(path, &statbuf) == -1) {
        dprintf(2, "Error: lstat failed '%s': %s\n", path, strerror(errno));
        return -1;
    }
    
    if (!S_ISDIR(statbuf.st_mode)) {
        dprintf(2, "Errorr: '%s' is not a directory\n", path);
        return -1;
    }
    
    // delete
    int result = remove_directory_contents(path);

    if (rmdir(path) == -1) {
        dprintf(2, "Error: failed to delete the directory '%s': %s\n", path, strerror(errno));
        result = -1;
    }

    dprintf(1, "=== End: Deletion of '%s' - %s ===\n", path, result == 0 ? "Success" : "Failure");
    
    return result;
}