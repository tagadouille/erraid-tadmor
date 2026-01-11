#include <stdint.h>

/**
 * @brief Set the run directory of the daemon.
 * Must be called before daemon_init() or erraid_init_foreground().
 * 
 * Example: erraid_set_rundir("/tmp/marc/erraid");
 *
 * @param rundir the path of the run directory
 * @param pipedir the path of the pipe directory
 * @return 0 on success, -1 on failure (errno is set).
 */
int erraid_set_rundir(const char *rundir, const char* pipedir);

/**
 * @brief Get the current run directory into `out` (size: outlen).
 * @param out buffer to store the run directory path
 * @param outlen size of the output buffer
 * @return 0 on success, -1 on failure.
 */
int erraid_get_rundir(char *out, size_t outlen);

/**
 * @brief Cleanup at daemon termination:
 *   - close logs
 */
void daemon_cleanup(void);

/**
 * @brief ensure the run directory
 */
int ensure_rundir(void);

/**
 * @brief create of the folder of the specified path
 * @param the path to create
 * @return 0 on success, -1 on failure
 */
int mkdir_p(const char *path);

/**
 * @brief helper hton64/ntoh64 (portable) 
 */
uint64_t hton64(uint64_t x);