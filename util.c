#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include "util.h"

bool open_file(const char *path, int oflag, int *fd_out)
{
	int ret_value = open(path, oflag, 0666);

	if(ret_value == -1)
	{
		write_strings(STDERR_FILENO, 1024, 5, "Failed to open file ", path, "\n", strerror(errno), "\n");
		return false;
	}
	*fd_out = ret_value;
	return true;
}

bool close_file(int fd)
{
	int ret_value = close(fd);
	if(ret_value == 0) return true;
	
	write_strings(STDERR_FILENO, 1024, 3, "Failed to close file: ", strerror(errno), "\n");
	return false;
}

bool seek_file(int fd, off_t offset, int whence, off_t *actual_offset_out)
{
	off_t ret_value = lseek(fd, offset, whence);
	if(ret_value != (off_t) -1)
	{
		if(actual_offset_out) *actual_offset_out = ret_value;
		return true;
	}
	
	write_strings(STDERR_FILENO, 1024, 3, "Failed to seek file: ", strerror(errno), "\n");
	return false;
}

bool read_file(int fd, void *buffer, size_t num_bytes, ssize_t *num_bytes_read)
{
	ssize_t ret_value = read(fd, buffer, num_bytes);

	if(ret_value != -1)
	{
		if(num_bytes_read) *num_bytes_read = ret_value;
		return true;
	}

	write_strings(STDERR_FILENO, 1024, 3, "Failed to read file: ", strerror(errno), "\n");
	return false;
}

bool write_file(int fd, void *buffer, size_t size)
{
	size_t bytes_written;
       
	if(size <= 0) return true;
	if(!buffer) return false;
	bytes_written = write(fd, buffer, size);

	if(bytes_written == -1)
	{
	        if(fd != STDERR_FILENO) //Infinite loops are not fun, I am told
			write_strings(STDERR_FILENO, 1024, 3, "Failed to invoke write: ", strerror(errno), "\n");
		else
			_exit(1);  //if things are this broken, this is the only sensible thing to do
		return false;
    	}
	else if(bytes_written == size) return true;
	else return write_file(fd, buffer + bytes_written, size - bytes_written);
}

bool write_string(int fd, const char *s, size_t no_longer_than)
{
	size_t str_len;
	if(!s) return false;
	str_len = strnlen(s, no_longer_than);

	if(str_len == 0) return true;
	return write_file(fd, (void*) s, str_len);
}

bool write_strings(int fd, size_t no_longer_than, int num_strings, ...)
{
	va_list args;
	int n;
	bool return_value = true;

	va_start(args, num_strings);
	for(n = 0; n < num_strings; n++)
	{
		const char *string = va_arg(args, const char*);
		return_value = write_string(fd, string, no_longer_than);
		if(return_value == false) break;
	}

	va_end(args);
	return return_value;
}

//really only supports base 10 and 16
void convert_integer(char *str, long n, int base, int min_length) {

    char tmp[100];

    int i, r, k;
    if (n == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    if (n < 0) {
        str[0] = '-';
        convert_integer(str + 1, -n, base, min_length);
        return;
    }

    r = n;
    i = 0;

    while (r != 0) {
        tmp[i] = '0' + (r % base);
		if(tmp[i] > '9') tmp[i] += 7;
        r /= base;
        i++;
    }

    for (k=0;k<i;k++) {
        str[k] = tmp[i-1-k];
    }

    str[k] = '\0';

	size_t str_len = strlen(str);
	if(str_len < min_length)
	{
		char buffer[100];
		size_t diff = min_length - str_len;
		strcpy(buffer, str);
		strcpy(str+diff, buffer);
		for(size_t n = 0; n < diff; n++)
			str[n] = '0';
	}
}

char *strcpy(char *destination, const char *source)
{
	char *original = destination;
	while(*source)
	{
		*destination = *source;
		destination++;
		source++;
	}
	*destination = *source;
	return original;
}

bool write_int(int fd, long num, int base, int min_length)
{
	char buffer[100];
	convert_integer(buffer, num, base, min_length);
	return write_string(fd, buffer, 100);
}

int comp_strings(const char *str1, const char *str2, int no_longer_than)
{
	if(str1 == NULL || str2 == NULL) return 0;

	while(no_longer_than != 0)
	{
		if(*str1 < *str2) return -1;
		if(*str1 > *str2) return 1;
		str1++;
		str2++;
		no_longer_than--;

		if(*str1 == 0 && *str2 == 0) return 0;
		if(*str1 == 0) return -1;
		if(*str2 == 0) return 1;
	}

	return 0;
}

bool open_pipes(int *in_fd, int *out_fd)
{
	int pipes[2], status, errno;

	status = pipe(pipes);
	if(status == -1)
	{
		write_strings(STDERR_FILENO, 1024, 3, "Failed to open pipe: ", strerror(errno), "\n");
		return false;
	}
	*in_fd = pipes[0];
	*out_fd = pipes[1];
	return true;
}

bool dup2_checked(int fildes, int fildes2)
{
	int ret_value = dup2(fildes, fildes2);

	if(ret_value == -1)
	{
		write_strings(STDERR_FILENO, 1024, 3, "Failed to invoke dup2: ", strerror(errno), "\n");
		return false;
	}
	if(ret_value != fildes2)
	{
		write_strings(STDERR_FILENO, 1024, 1, "ret_value does not equal fildes2 (and it should)\n");
		return false;
	}
	return true;
}

bool exec_checked(const char *file, char *const argv[])
{
	execvp(file, argv);
	write_strings(STDERR_FILENO, 1024, 3, "Failed to invoke execvp: ", strerror(errno), "\n");
	return false;
}

void hexDump (void *addr, int len) {
    int i;
    unsigned char buff[17];       // stores the ASCII data
    unsigned char *pc = addr;     // cast to make the code cleaner.

    for (i = 0; i < len; i++) {
        if ((i % 16) == 0) {
            if (i != 0)
                write_strings(STDERR_FILENO, 17, 3, "  ", buff, "\n");
                //printf ("  %s\n", buff);
            write_strings(STDERR_FILENO, 20, 1, "  ");
            write_int(STDERR_FILENO, i, 16, 4);
            write_strings(STDERR_FILENO, 20, 1, " ");
            //printf ("  %04x ", i);
        }

        //printf (" %02x", pc[i]);
        write_strings(STDERR_FILENO, 1, 1, " ");
        write_int(STDERR_FILENO, pc[i], 16, 2);

        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    while ((i % 16) != 0) {
        write_string(STDERR_FILENO, "   ", 5);
        //printf ("   ");
        i++;
    }

    //printf ("  %s\n", buff);
    write_strings(STDERR_FILENO, 17, 3, "  ", buff, "\n");
}
