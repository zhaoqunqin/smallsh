#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "link.h"

char *argv[512];

char command[2050];
bool is_background_process = false;
bool is_foreground_process = false;
bool is_terminated_by_signal = false;

int foreground_status = 0;
int output_fd;
int input_fd;

extern bool need_change_mode;
extern bool foreground_only_mode;
extern bool need_create_foreground_process;

extern void waiting(pid_t);
extern void kill_all_pid(struct background_process *);
extern void check_background_process(struct background_process *);
extern bool traverse_command_line(char *, int *, int *);
extern void act_func(int);
extern void insert_background_process(struct background_process *, pid_t);
extern int check_badfile(char *);

int main()
{
	struct background_process *background_process, *head, *last;
	struct sigaction act;
	int count = 0;
	pid_t pid;

	head = malloc(sizeof(*head));
	head->next = NULL;
	last = head;

	//Register a func for SIGTSTP for all process including smallsh, 
	//forground and background process.
	//Foreground and background process can ignor SIGTSTP by call signal.
	act.sa_handler = act_func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGTSTP, &act, 0);

	// Ignore SIGINT,  the child process running foreground
	// can call signal(SIGINT, DFL) to receive SIGINT
	signal(SIGINT, SIG_IGN);

	while(true) {
		// puts the command prompt
		putchar(':');
		putchar(' ');
		fflush(stdout);
		//If parent process interrupt by SIGTSTP while waiting for input.
		//Set *command = 0 in the first can check if we get a command
		//and print exactly.
		*command = 0;
		//get command line and store it to command
		fgets(command, 2050, stdin);

		if(*command == 0)
			continue;

		/*
		 * @traverse_command_line:
		 *
		 * Get input fd, output fd, and check if we should create a foreground process.
		 *
		 * If any wrong, we call waitpid in check_background_process to release the zombine process,
		 * and report the user the status of background process BEFORE return command line access
		 * and control to user.
		 *
		 */
		if(!traverse_command_line(command, &input_fd, &output_fd)) {
			check_background_process(head);
			continue;
		}
		/*
		 * here is the three built-in commands
		 *
		 * First is user input exit to the command line
		 * we need to kill all of the child process.
		 *
		 */ 
		if(strcmp(argv[0], "exit") == 0) {
			kill_all_pid(head);
			return 0;
		}
		/*
		 * if user input status to the command line
		 * printf satus
		 * The exit status is recorded in the variable foreground_status
		 * 
		 */
		else if(strcmp(argv[0], "status") == 0) {
			if(is_terminated_by_signal)
				printf("terminated by signal %d\n", foreground_status);
			else
				printf("exit value %d\n", foreground_status);
			fflush(stdout);
		}
		else if(strcmp(argv[0], "cd") == 0) {
			//if there has a path after cd
			if(argv[1])
				chdir(argv[1]);
			// else change dir to HOME
			else
				chdir(getenv("HOME"));
		}

		// if we find a command that was not built in smallsh
		// use fork-execvp module to run the command
		else {
			pid = fork();
			if(pid == 0) {
				is_foreground_process = need_create_foreground_process;
				is_background_process = !is_foreground_process;

				// redirection of input and output
				dup2(input_fd, fileno(stdin));
				dup2(output_fd, fileno(stdout));

				// foreground and background shoud ignore SIGTSTP 
				signal(SIGTSTP, SIG_IGN);

				// foreground process shoud receive SIGINT 
				if(is_foreground_process) {
					signal(SIGINT, SIG_DFL);
				}

				//check if the command exist in PATH
				if(check_badfile(argv[0]) == -1) {
					int er = errno;
					puts("badfile: no such file or directory");
					fflush(stdout);
					return er;
				}

				//excute command
				execvp(argv[0], argv);

				exit(errno);
			} // end child process's code
			else {
				//If need create foreground process,
				//the parent process should waiting until the child process is done.
				if(need_create_foreground_process)
					waiting(pid);
				//Else inset this child process to link list used by kill_all_pid.
				//And the parent process should return command line access and control to user
				//immediately.
				else {
					insert_background_process(last, pid);
					printf("background pid is %d\n", pid);
					fflush(stdout);
				}
			}//end parent process's code
		}
		// Call waitpid in check_background_process to release zombine process, 
		// and report it's status.
		check_background_process(head);
		//close the redirection input fd and output fd
		if(input_fd != 0)
			close(input_fd);
		if(output_fd != 1)
			close(output_fd);
	} // end while(true)

	return 0;
}
			
