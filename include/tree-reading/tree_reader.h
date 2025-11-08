#include <limits.h>
#include <stdint.h>

#define TASKPATH "Consignes/exemples-arborescences/" //The path to the task tree (provisional)

/*Name of all the tasks directories (provisional)*/
#define DIR1 "exemple-arborescence-1"
#define DIR2 "exemple-arborescence-2"
#define DIR3 "exemple-arborescence-3"
#define DIR4 "exemple-arborescence-4"

#define SUBDIR "/tmp-username-erraid"

#define MAX_PATH pathconf("/", _PC_PATH_MAX) //The maximum path length of the computer
#define BUFFER_SIZE 1024 //The size of the buffer for reading

/*Read the given task tree according to the path and task_id
arguments
return 0 if success, 1 if failure*/
int task_reader(const char* path, uint16_t task_id);

/*Find the task according to the path and task_id
arguments
return 0 if the task is found, 1 if not or if a error occured (an error message will be display in
that case)*/
int task_finder(char* path, char* task_id);

/*Extract the information of the specified task which is stored at path argument
return 0 if success, 1 if failure*/
int extract_task_information(const char* path);

/*Initialise the buffer with a size of BUFFER_SIZE
return 0 if succes, 1 if failure*/
int buffer_init(char** buffer);

/*Make the path by concat og_path with /folder_name
Return the concatenation, NULL if failure */
char* make_path(const char* og_path, const char* folder_name);