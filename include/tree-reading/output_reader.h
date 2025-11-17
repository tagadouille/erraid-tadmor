#ifndef OUTPUT_READER_H
#define OUTPUT_READER_H

/**
* @brief Read the standard output file and write the content to STDOUT_FILENO
* @param path the path to the standard output file
* @param is_stderr specify if the file to read is stderr or stdout
* @return 0 if succes, -1 otherwise*/
int output_reader(const char* path, bool is_stderr);

#endif