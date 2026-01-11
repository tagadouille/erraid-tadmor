#include "tree-writer/task_creator.h"
#include "communication/request.h"
#include "types/timing.h"

/**
* Combines multiple tasks into a single task and destroys the original tasks.
* @param timing Pointer to the timing structure containing task information.
* @param composed Pointer to the composed structure containing task details.    
* @return The ID of the newly created combined task.
*/
int64_t combine_and_destroy_tasks(const timing_t *timing, const composed_t *composed);