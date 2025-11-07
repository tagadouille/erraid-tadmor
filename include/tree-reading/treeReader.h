#define TASKPATH "Consignes/exemples-arborescences/" //The path to the task tree (provisional)

/*Name of all the tasks directories (provisional)*/
#define DIR1 "exemple-arborescence-1"
#define DIR2 "exemple-arborescence-2"
#define DIR3 "exemple-arborescence-3"
#define DIR4 "exemple-arborescence-4"

/*Read the given task tree according to the path and task_id
arguments
return 0 if success, 1 if failure*/
int task_reader(const char* path, uint16 task_id);