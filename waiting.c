#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "link.h"

bool waiting_foreground_process;

extern int foreground_status;
extern bool is_terminated_by_signal;
extern bool need_change_mode;
extern bool foreground_only_mode;

extern void delet(struct background_process *, pid_t);

void waiting(pid_t pid)
{
	int status;

	// We will waiting foregrond process,
	// set variable waiting_foreground_process to true.
	waiting_foreground_process = true;
	// I set a maxnum 10 that the parent process can be interrupt by SIGTSTP while waiting foreground process.
	// If the number of SIGTSTP the parent process receive is bigger than 10,
	// the parent process will return from waiting foreground process, so 
	// the foreground process will be released as a background process.
	for(int i = 0; i < 10; i++) {
		if(waitpid(pid, &foreground_status, 0) == pid)
			break;
	}
	waiting_foreground_process = false;

	status = foreground_status;

	// Check if the foreground process is terminated by signal 
	// or exit exactly.
	// And record the exit value or signal respectively.
	if(WIFEXITED(status)) {
		foreground_status = WEXITSTATUS(status);
		is_terminated_by_signal = false;
	}

	if(WIFSIGNALED(status)){
		foreground_status = WTERMSIG(status);
		printf("terminated by signal %d\n", foreground_status);
		fflush(stdout);
		is_terminated_by_signal = true;
	}

	// In act_func we record wether we need change mode after
	// waiting foreground process.
	if (need_change_mode) {
		need_change_mode = false;
		if(foreground_only_mode) {
			foreground_only_mode = false;
			puts("Exiting foreground-only mode");
		}
		else {
			foreground_only_mode = true;
			puts("Entering foreground-only mode (& now ignored)");
		}
		fflush(stdout);
	}
}
/*
 * @check_background_process:
 *
 * Everytime before parent process return command line access and control to user,
 * the process should check if there is any zombine process.
 * If there is zombine process, report it's return status.
 *
 */
void check_background_process(struct background_process *head)
{
	pid_t wait_pid;
	int status;

	while( (wait_pid = waitpid(-1, &status, WNOHANG)) > 0) {
		if(WIFSIGNALED(status)){
        		    printf("background pid %d is done: terminated by signal %d\n",
					   wait_pid, WTERMSIG(status));
       		}
		else
			printf("background pid %d is done: exit value %d\n",
				wait_pid, WEXITSTATUS(status));
		fflush(stdout);
		delet(head, wait_pid);
	}
}
void insert_background_process(struct background_process *last, pid_t pid)
{
	struct background_process *background_process = (struct background_process *)malloc(
		sizeof(struct background_process));
	background_process->pid = pid;
	background_process->next = NULL;

	last->next = background_process;
	last = background_process;
}

