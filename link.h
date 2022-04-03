#include <sys/types.h>

struct background_process {
	pid_t pid;
	struct background_process *next;
};
