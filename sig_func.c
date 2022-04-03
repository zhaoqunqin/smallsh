#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

extern bool is_foreground_process;
extern bool is_background_process;
extern bool foreground_only_mode;
extern bool waiting_foreground_process;
bool need_change_mode = false;
bool foreground_only_mode = false;

void act_func(int sig)
{
	if(sig == SIGTSTP && !is_foreground_process && !is_background_process) {
		// if parent process interrupted by signal SIGTSTP while waiting foreground process
		// check if we need to change forground-only mode after the foreground process is done.
		if(waiting_foreground_process) {
			if(need_change_mode)
				need_change_mode = false;
			else
				need_change_mode = true;
			return;
		}
		// If parent process not waiting foreground process
		// we will chang mode immediately after receive SIGTSTP.
		if(foreground_only_mode) {
			foreground_only_mode = false;
			puts("\nExiting foreground-only mode");
		}
		else {
			foreground_only_mode = true;
			puts("\nEntering foreground-only mode (& now ignored)");
		}
		fflush(stdout);
	}
}

