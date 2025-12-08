#ifndef CODE_H
#define CODE_H

#define PIPE_BUF 4096

/**
 * The enum of the OPCODE
 */
enum opcode{
    LS = 0x4c53, //LIST -- list all the task
    CR = 0x4352, //CREATE -- create a new simple task
    CB = 0x4342, //COMBINE -- create a new task per combination of already existing task
    RM = 0x524d, // REMOVE -- delete a task
    TX = 0x5458, // TIMES_EXITCODES -- display times_exitcodes file
    SO = 0x534f, // STDOUT -- display stdout of the last complete execution of a task
    SE = 0x5345, // STDERR -- display stderr of the last complete execution of a task
    TM = 0x4b49 //TERMINATE -- finish the daemon
};

/**
 * The enum of the ANSTYPE
 */
enum anstype{
    OK = 0x4f4b,
    ERR = 0x4552,
};

/**
 * The enum of the ERRCODE
 */
enum errcode{
    NF = 0x4e46,
    NR = 0x4e52
};

#endif