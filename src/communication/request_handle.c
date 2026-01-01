#define _GNU_SOURCE

#include "communication/request_handle.h"
#include "communication/answer.h"
#include "communication/request.h"
#include "tadmor.h"
#include "tree-reading/tree_reader.h"
#include "erraids/erraid.h"
#include "types/time_exitcode.h"
#include "tree-writer/task_annihilator.h"
#include "tree-writer/task_creator.h"
#include "tree-writer/task_combinator.h"

a_list_t* handle_ls(char *rundir)
{
    all_task_t *list = all_task_listing(rundir); // get list of all tasks
    
    if (list == NULL) {
        return create_a_list(ERR, 0, NULL);
    }

    return create_a_list(OK, list->nbtask, list->all_task); // return list of tasks
}

a_timecode_t* handle_tx(char *rundir, uint64_t id)
{
    if (task_reader(rundir, id, TIME_EXIT) < 0) {
        return create_a_timecode_t(ERR, 0, NULL);
    }

    // return time-exitcode array
    return create_a_timecode_t(
        OK,
        curr_time->nbruns,
        curr_time->all_timecode
    );
}

a_output_t* handle_output(char *rundir, uint64_t id, bool is_stderr)
{
    if (task_reader(rundir, id, is_stderr ? STDERR : OUTPUT) < 0) {

        string_t* empty_string = string_create(NULL, 0);
        a_output_t* err_output = create_a_output_t(ERR, empty_string, NF);
        string_free(empty_string);
        return err_output;
    }

    return create_a_output_t(OK, curr_output, 0);
}

answer_t* handle_rm(char *rundir, uint64_t id)
{
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%lu", rundir, id); 

    int ret = delete_directory(path);

    if (ret < 0) {
        return create_answer(ERR, id, NF);
    }

    return create_answer(OK, id, 0);
}

answer_t* handle_tm(void)
{
    //return create_answer(ERR, 0, NR); Si le daemon n'est pas lancé je mettrai cette ligne en plus.
    
    //terminate_daemon();
    return create_answer(OK, 0, 0); // return success answer
}

void* simple_request_handle(simple_request_t *req, char *rundir)
{
    if (req == NULL || rundir == NULL) {
        dprintf(2, "Error : the request is NULL or the rundir is NULL");
        return NULL;
    }

    switch (req->opcode) { // handle request based on opcode

        case LS:
            return handle_ls(rundir);
        
        case RM:
            return handle_rm(rundir, req->task_id);

        case TX:
            return handle_tx(rundir, req->task_id);

        case SO:
            return handle_output(rundir, req->task_id, false);

        case SE:
            return handle_output(rundir, req->task_id, true);

        case TM:
            return handle_tm();

        default: // unknown opcode
            return create_answer(ERR, req->task_id, NR);
    }
}

/**
 * @brief Handle the create request (CR).
 * @param rundir Base directory of task folders
 * @param timing The timing structure
 * @param command The command to execute
 * @return answer_t* The answer structure
 */
static answer_t* handle_combine(char* rundir, timing_t timing, composed_t* composed){

    int64_t id = combine_and_destroy_tasks(&timing, composed);

    if(id == -1){
        return create_answer(ERR, 0, NF);
    }
    return create_answer(OK, id, 0);
}

/**
 * @brief Handle the create request (CR).
 * @param rundir Base directory of task folders
 * @param timing The timing structure
 * @param command The command to execute
 * @return answer_t* The answer structure
 */
static answer_t* handle_create(char* rundir, timing_t timing, command_t* command){

    int res = create_task_dir(&timing, command ->args.simple);
    
    return create_answer(OK, res, 0);
}

answer_t* complex_request_handle(complex_request_t *req, char *rundir) {

    if (req == NULL || rundir == NULL) {
        dprintf(2, "Error : the request is NULL or the rundir is NULL");
        return NULL;
    }

    // handle request based on opcode :
    switch (req->opcode) {

        case CR:
            return handle_create(rundir, req->timing, req->u.command);

        case CB:
            return handle_combine(rundir, req->timing, req->u.composed);

        default: // unknown opcode
            return create_answer(ERR, 0, NR);
    }
}