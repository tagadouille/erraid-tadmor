#ifndef CMD_READER_H
#define CMD_READER_H

#include "types/task.h"

/**
* @brief Read the cmd folder of the task specified by the path argument
* and extract the information
* @param path the path to the cmd folder of the task
* @return 0 if succes, -1 otherwise 
*/
int cmd_reader(const char* path);

/**
 * @brief construct the command of the task by reading the task tree command structure
 * @param path the path to the cmd folder of the task
 * @param cmd a pointer to the command that'll be created
 * @return a pointer to the command that has been created if success, NULL pointer otherwise
 */
command_t* command_parser(const char* path, command_t* cmd);

/**
* @brief Read the argv file of the SI command specified by the path argument and construct it
* @param path the path to folder that contains the argv file
* @param og_command the command_t structure to fill
* @return a pointer to the command that has been filled if success, NULL otherwise
*/
command_t* argv_reader(const char* path, command_t* og_command);

/**
* @brief Read the type file of the task specified by the path argument
* @param path the path to the cmd folder of the task
* @return the type of the command if success, INVALID type otherwise
*/
command_type_t type_reader(const char* path);

/**
 * @brief associate the content of the buffer with the different command_type_t of @file types/task.h
 * @param buffer the buffer which is the content of the type file that has been read
 * @return the type of the command if success, INVALIDE otherwises
 */
command_type_t type_interpreter(char* buffer);

/**
 * @brief in function of the type given in parameter, it'll construct a new command from
 * og_command and cmd.
 * @param path the path the the folder where the argv file is
 * @param type the type of og_command
 * @param og_command a complex command
 * @param cmd a command which will be add the og_command if og_command is not a SI command,
 * can be NULL if the type is SI
 * @return the new command that has been created.
 */
command_t* type_processor(const char* path, command_type_t type, command_t* og_command, command_t* cmd);

#endif