#define _GNU_SOURCE

#include "communication/request_handle.h"
#include "communication/answer.h"
#include "communication/request.h"
#include "tadmor.h"
#include "tree-reading/tree_reader.h"
#include "erraids/erraid.h"
#include "types/time_exitcode.h"

a_list_t* handle_ls(char *rundir)
{
    all_task_t *list = all_task_listing(rundir); // get list of all tasks
    if (list == NULL) { // if error during listing
        return create_a_list(ERR, 0, NULL); // return error answer
    }

    return create_a_list(OK, list->nbtask, list->all_task); // return list of tasks
}

a_timecode_t* handle_tx(char *rundir, uint64_t id)
{
    if (task_reader(rundir, id, TIME_EXIT) < 0) { // if error during reading
        return create_a_timecode_t(ERR, 0, NULL); // return error answer
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
    // if is_stderr true read stderr else read stdout
    if (task_reader(rundir, id, is_stderr ? STDERR : OUTPUT) < 0) { // if error during reading
        // return error answer
        return create_a_output_t(ERR, curr_output, NF);
    }

    return create_a_output_t(OK, curr_output, 0); // return output/error output
}

answer_t* handle_rm(char *rundir, uint64_t id)
{
    // construct path to task folder
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%lu", rundir, id); 

    if (access(path, F_OK) != 0) { // if task folder does not exist
        return create_answer(ERR, id, NF); // return not found error
    }

    if (remove(path) != 0) { // if error during removal
        return create_answer(ERR, id, NR); // return no 'runned' error
    }

    return create_answer(OK, id, 0); // return success answer
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
        return create_answer(ERR, 0, NR);
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