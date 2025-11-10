#ifndef CMD_READER_H
#define CMD_READER_H

/*The different type of task*/
#define SI 'SI' //individual task
#define SQ 'SQ' //sequence of task

/**
* @brief Read the cmd folder of the task specified by the path argument
* and extract the information
* @param path the path to the cmd folder of the task
* @return 0 if succes, -1 otherwise 
*/
int cmd_reader(const char* path);

/**
* @brief Read the argv file of the task specified by the path argument
* @param path the path to the cmd folder of the task
* @return 0 if succes, -1 otherwise
*/
int argv_reader(const char* path);

/**
* @brief Read the type file of the task specified by the path argument
* @param path the path to the cmd folder of the task
* @return 0 if succes, -1 otherwise
*/
int type_reader(const char* path);

#endif