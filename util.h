#ifndef UTILH
#define UTILH

#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

//All but comp_strings: Return true on success, print an error to stderr and return false on error
//On error, any output vars are not modified
//Most of these are just wrappers around the system calls

//assert clone, but accepts an optional error string
void die_if_false(bool condition, const char *error);
//Opens a file, sets fd_out to the file descriptor
bool open_file(const char *path, int oflag, int *fd_out);
//Closes a file
bool close_file(int fd);
//Seeks a file as per lseek
bool seek_file(int fd, off_t offset, int whence, off_t *actual_offset_out);
//Reads a file
bool read_file(int fd, void *buffer, size_t num_bytes, ssize_t *num_bytes_read);
//Writes raw bytes to a file
bool write_file(int fd, void *buffer, size_t size);
//Writes a string to a file, with no_longer_than being a safety incase the null terminator is missing
bool write_string(int fd, const char *s, size_t no_longer_than);
void convert_integer(char *str, long n, int base, int min_length);
//As above, but writes num_strings in one call
bool write_strings(int fd, size_t no_longer_than, int num_strings, ...);
bool write_int(int fd, long num, int base, int min_length);
//Opens pipes
bool open_pipes(int *in_fd, int *out_fd);
//dup2 wrapper
bool dup2_checked(int fildes, int fildes2);
//execvp wrapper
bool exec_checked(const char *file, char *const argv[]);



//Duplicates strncmp
int comp_strings(const char *str1, const char *str2, int no_longer_than);
char *strcpy(char *destination, const char *source);

void hexDump (void *addr, int len);

#endif