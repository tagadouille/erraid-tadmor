#ifndef CMD_READER_H
#define CMD_READER_H

#include "types/task.h"

/**
* @brief Read the cmd folder of the task specified by the path argument
* and extract the information
* @param path the path to the cmd folder of the task
* @param cmd the command to be completed
* @return 0 if succes, -1 otherwise 
*/
int cmd_reader(const char* path, command_t* cmd);

/**
* @brief Read the argv file of the task specified by the path argument
* @param path the path to the cmd folder of the task
* @param og_command the command_t structure to fill, if it's a simple command it will be filled directly, if it's a complex command the simple command will be added to it
* @param type the type of the command being parsed
* @return 0 if succes, -1 otherwise
*/
int argv_reader(const char* path, command_t* og_command, command_type_t type);

/**
* @brief Read the type file of the task specified by the path argument
* @param path the path to the cmd folder of the task
* @return 0 if succes, -1 otherwise
*/
int type_reader(const char* path, command_t *cmd);

int type_interpreter(const char* path, char* buffer, command_t* cmd);

#endif