/*Read the cmd folder of the task specified by the path argument
and extract the information
return 0 if succes, 1 otherwise */
int cmd_reader(char* path);

/*Read the argv file of the task specified by the path argument
return 0 if succes, 1 otherwise*/
int argv_reader(char* path);

/*Read the type file of the task specified by the path argument
return 0 if succes, 1 otherwise*/
int type_reader(char* path);