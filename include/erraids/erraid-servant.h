extern pid_t father;

/**
 * @brief Start the reading, processing and answering of client
 * requests
 * @param father the pid of erraid his father
 */
void start_serve(pid_t proc_father);