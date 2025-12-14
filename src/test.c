#define _GNU_SOURCE

#include "types/time_exitcode.h"
#include "tree-reading/tree_reader.h"
#include "erraids/erraid.h"
#include "test.h"

static void test_time_exit_read(char* task_dir, uint64_t id){

    if(task_reader(task_dir, id, TIME_EXIT) < 0){
        dprintf(STDOUT_FILENO, "task_reader failed for %lu", id);
        return;
    }
    if(curr_time == NULL){
        dprintf(STDOUT_FILENO, "curr_time is null");
        return;
    }
    all_time_show(curr_time);
}

static void test_output_read(char* task_dir, uint64_t id){
    
    if(task_reader(task_dir, id, OUTPUT) < 0){
        dprintf(STDOUT_FILENO, "task_reader failed for %lu", id);
        return;
    }
    dprintf(STDOUT_FILENO, "stdout : %s\n", curr_output.data);
    string_free(&curr_output);

    if(task_reader(task_dir, id, STDERR) < 0){
        dprintf(STDOUT_FILENO, "task_reader failed for %lu", id);
        return;
    }
    dprintf(STDOUT_FILENO, "stderr : %s\n", curr_output.data);
    string_free(&curr_output);
}

void test_all(char* task_dir, uint64_t id){
    test_time_exit_read(task_dir, id);
    test_output_read(task_dir, id);
}

void test_task_read(char* task_dir){
    all_task_t* tasks = all_task_listing(task_dir);
    dprintf(STDOUT_FILENO, "nbtask : %i\n", tasks -> nbtask);
    
    for (size_t i = 0; i < tasks -> nbtask; i++)
    {
        task_display(&(tasks -> all_task)[i]);
    }
}