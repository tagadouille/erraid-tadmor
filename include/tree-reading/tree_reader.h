#ifndef TREE_READER_H
#define TREE_READER_H

#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include "types/task.h"

#define TASKPATH "Consignes/exemples-arborescences/" //The path to the task tree (provisional)

/*Name of all the tasks directories (provisional)*/
#define DIR1 "exemple-arborescence-1"
#define DIR2 "exemple-arborescence-2"
#define DIR3 "exemple-arborescence-3"
#define DIR4 "exemple-arborescence-4"

#define SUBDIR "/tmp-username-erraid"

#define MAX_PATH pathconf("/", _PC_PATH_MAX) //The maximum path length of the computer
#define BUFFER_SIZE 1024 //The size of the buffer for reading

// The current task being processed
extern task_t* curr_task; //! maybe move it to erraid

// The different type of action to do while reading the task tree
enum Action_type {
    LIST = 10, //To list the tasks
    OUTPUT, //To get the standard output of the tasks
    ERR, //To get the error outpout of the tasks
    TIME_EXIT //To get the times and exit codes of the tasks
};
typedef enum Action_type Action_type;

/**
* @brief Read the given task tree located at path argument for the task with the id task_id
* @param path the path to the task tree
* @param task_id the number of the task
* @param action specify the action to do while reading the task.
* @return 0 if success, -1 if failure
*/
int task_reader(const char* path, uint16_t task_id, Action_type action);

/**
* @brief Find the task according to the path and task_id arguments
* @param path the path to the directory containing the tasks
* @param task_id the number of the task
* @param action specify the action to do while reading the task
* @return 0 if the task is found, -1 if not or if a error occured (an error message will be display in
*that case)
*/
int task_finder(char* path, char* task_id, Action_type action);

/**
* @brief Extract the information of the specified task which is stored at path argument according to the action argument.
* @param path the path to the task
* @param action specify the action to do while reading the task
* @return 0 if success, -1 if failure
*/
int extract_task_information(const char* path, Action_type action);

/**
 * @brief Auxiliary function to extract the command information from the cmd folder of the task
 * @param path the path to the task
 * @return 0 if success, -1 if failure
 */
int aux_extract_cmd(const char* path);

/**
 * @brief Auxiliary function to extract information from a file located in folder_name inside path
 * @param path the path to the task
 * @param folder_name the folder name containing the file to read
 * @param func pointer to the output_reader function to use to read the file
 * @param is_stderr boolean to specify if we read stderr or stdout
 * @return 0 if success, -1 if failure
 */
int aux_extract_output(const char* path, char* folder_name, int (*func)(const char*, bool), bool is_stderr);

/**
 * @brief Auxiliary function to extract timing or times-exitcodes information from a file located in folder_name inside path
 * @param path the path to the task
 * @param folder_name the folder name containing the file to read
 * @return 0 if success, -1 if failure
 */
int aux_extract_time(const char* path, char* folder_name);

/**
* @brief  the buffer with a size of BUFFER_SIZE
* @param buffer the buffer to initialize
* @return 0 if succes, -1 if failure
*/
int buffer_init(char** buffer);

/**
* Make the path by concatenate og_path with "/folder_name" and test if the path created is correct
* @param og_path the original path
* @param folder_name the folder name to concatenate
* @return the concatenation, NULL if failure
*/
char* make_path(const char* og_path, const char* folder_name);

/**
* Make the path by concatenate og_path with "/folder_name" and don't test if the path created is correct
* @param og_path the original path
* @param folder_name the folder name to concatenate
* @return the concatenation, NULL if failure
*/
char* make_path_no_test(const char* og_path, const char* folder_name);

/**
 * @brief determine if the folder at the path exist or not
 * @param path the path to verify
 * @return 0 if it exists, 1 otherwise
 */
int folder_exist(const char* path);

#endif