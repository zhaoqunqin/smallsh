#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern bool foreground_only_mode;
extern char *argv[512];
char buffer[512][255];
static char std_command[2050];
static char file_name[64];
bool need_create_foreground_process;

//return the figures of pid in decimal  ****
static int expansion_pid_to_str(char *str, pid_t pid)
{
	int figure = 0;
	int tmp = (int)pid;
	int i;

	// get figure of pid
	while(tmp != 0) {
		figure++;
		tmp = tmp/10;
	}

	tmp = (int)pid;

	// paste the pid in to the site str point
	for(i = figure; i != 0; i--) {
		str[i-1] = (tmp % 10) + '0';
		tmp = tmp/10;
	}

	return figure;
}

// expansion of variable ($$ to pid);
static void expansion_of_variable(char *str, char *new)
{
	char *tmp = str;
	pid_t pid = getpid();
	bool pre = false;
	int figure;
	int i = 0;
	int j = 0;

	if(tmp[i] == '$')
		pre = true;
	new[j++] = tmp[i++];

	for(; tmp[i] != 0; ) {
		if(tmp[i] == '$') {
			// If current char is '$' and previous char is '$', 
			// then we find two continuous '$'.
			// Now we shoud a expansion.
			if(pre) {
			//find $$ 
				i++;
				j--;
				figure = expansion_pid_to_str(&new[j], pid);
				// the index of the new string shoudl add figure
				j += figure;
				pre = false;
			}
			else {
				pre = true;
				new[j++] = tmp[i++];
			}
		}
		else {
			new[j++] = tmp[i++];
			pre = false;
		}
	}
}

/*
 * @traverse_command_line:
 *
 * Traverse command line to get input fd, output fd, arguments,
 * and check if we need create a foreground process.
 *
 */
bool traverse_command_line(char *str, int *input_fd, int *output_fd)
{
	char *tmp = std_command;
	int fd;
	int j;
	int i;
	int num = 0;

	*input_fd = 0;
	*output_fd = 1;
	need_create_foreground_process = true;

	memset(std_command, 0, 2050);

	// The command line that user input will be standard command line after the $$ expansion.
	expansion_of_variable(str, std_command);

	// Remove the leading space.
	while(*tmp == ' ')
		tmp = tmp + 1;

	// If the first char in the command after remove the leading space is
	// '\n' , '#' or '&'.
	// This command line will treat as a comment.
	if(*tmp == '\n' || *tmp == '#' || *tmp == '&') {
		argv[0] = NULL;
		return false;
	}

	/* Traverse command line to get input fd, output fd, arguments,
 	* and check if we need create a foreground process.
	*/
	for(; *tmp != 0; ) {
		// Skip line breaks and spaces.
		if(*tmp == '\n' || *tmp == ' ')
			tmp = tmp + 1;

		// If we find input or output redirection.
		else if(*tmp == '>' || *tmp == '<') {
			char ch = *tmp;
		        need_create_foreground_process = true;	
			j = 0;
			tmp  = tmp + 2;
			// Here paste the file name in command line to file_name array.
			while(*tmp != ' ' && *tmp != '\n') {
				file_name[j++] = *tmp;
				tmp = tmp + 1;
			} file_name[j] = 0;

			// If this is output redirection, we shoud open the file or create it for write.
			if(ch == '>') {
				*output_fd = open(file_name, O_WRONLY|O_CREAT, 0600);
				if(*output_fd == -1) { 
					puts("badfile: no such file for output");
					fflush(stdout);
					return false;
				}
				
			}
			// If this is input redirection, we shoud open the file for read.
			else {
				*input_fd = open(file_name, O_RDONLY);
				if(*input_fd == -1) {
					printf("can't open %s for input\n", file_name);
					fflush(stdout);
					return false;
				}
			}
				
		}
		// If find &, we set need_create_foreground_process to true temporayly.
		else if(*tmp == '&') {
		       need_create_foreground_process = false;	
		       tmp = tmp + 1;
		}
		// If this char is not a  space, redirection or &.
		// Now get the command and arguments that we may send to function execvp().
		else {
		        need_create_foreground_process = true;	
			argv[num] = buffer[num];
			for(i = 0; *tmp != ' ' && *tmp != '\n' && *tmp != 0;) {
				argv[num][i++] = *tmp;
				tmp = tmp + 1;
			}
			argv[num][i] = 0;
			num++;
		}
	}

	// If we entering foreground-only mode,
	// set need_create_foreground_process to true.
	if(foreground_only_mode)
		need_create_foreground_process = true;

	// If the user don't redirect the standard input or putput , 
	// redirect the input or output to /dev/null.
	if(!need_create_foreground_process) {
		fd = open("/dev/null", O_RDWR);
		if(*input_fd == 0)
			*input_fd = fd;
		if(*output_fd == 1)
			*output_fd = fd;
	}

	argv[num] = NULL;

	return true;
}

