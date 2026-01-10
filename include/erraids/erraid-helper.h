#include <stdint.h>

/**
 * Set the run directory of the daemon.
 * Must be called before daemon_init() or erraid_init_foreground().
 * 
 * Example: erraid_set_rundir("/tmp/marc/erraid");
 *
 * Returns 0 on success, -1 on failure (errno is set).
 */
int erraid_set_rundir(const char *rundir, const char* pipedir);

/**
 * Get the current run directory into `out` (size: outlen).
 * Returns 0 on success, -1 on failure.
 */
int erraid_get_rundir(char *out, size_t outlen);

/**
 * Cleanup at daemon termination:
 *   - close logs
 */
void daemon_cleanup(void);

/**
 * @brief ensure the run directory
 */
int ensure_rundir(void);

/**
 * @brief it's the equivalent of the function realpath
 */
char *my_realpath(const char *path, char *resolved_path);

/**
 * @brief create of the folder of the specified path
 * @param the path to create
 */
int mkdir_p(const char *path);

/**
 * @brief helper hton64/ntoh64 (portable) 
 */
uint64_t hton64(uint64_t x);

/**
 * @brief Write the current process PID into a pidfile in /tmp/$user/erraid/
 * @return 0 on success, -1 on failure
 */
int write_pid_file();

/**
 * @brief Read the PID of erraid from the pidfile
 * @return the PID on success, -1 on failure
 */
pid_t read_pid_file();