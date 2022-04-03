#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "link.h"
/*@delet:
 *
 * Remove the process identified by PID from the linked list
 *
 */
void delet(struct background_process *head, pid_t pid)
{       
        struct background_process *tmp;
                
        while(head->next && head->next->pid != pid)
                head = head->next;
             
        if(head->next) {
                tmp = head->next;
                head->next = tmp->next;
                free(tmp);
        }
}
/*@kill_all_pid:
 *
 * kill and remove all of the process int the linked list
 *
 */
void kill_all_pid(struct background_process *head)
{
	struct background_process *curr;
	pid_t pid;

	while(head->next) {
		curr = head->next;
		head->next = curr->next;
		
		if(kill(curr->pid, SIGKILL) == 0) {
			free(curr);
			continue;
		}
		/*
		 * if we can't kill the process indentifid by curr->pid even by sending SIGKILL
		 * maybe it is a zombine process
		 * so we call waitpid to try to release it if we can
		 *
		 */
		else {
			if(waitpid(curr->pid, NULL, WNOHANG) == 0) {
				/*
				 * if the process can't be killed by SIGKILL or released by waitpid
				 *
				 * try kill curr->pid again by sending signal 9
				 * if success, we have done the work,
				 * if fail , leave this work to operating system
				 * we want to return as fast as we can, and free link list
				 *
				 */
				kill(curr->pid, SIGKILL);
			}
			free(curr);
		}

	}
}
