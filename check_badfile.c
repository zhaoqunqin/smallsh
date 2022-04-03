#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static char path[1024];
/*
 * @check_back_badfile:
 *
 * check if the file or dir name pointed by name can be fond in the
 * PATH
 *
 */
int check_badfile(char *name)
{
	char *p = getenv("PATH");
	int i = 0;
	int j = 0;
	struct stat buff;

	path[i] = 0;
	for(;;p = p + 1) {
		/* here we are the end of the string of PATH
		 */
		if(*p == 0){
			// here we get a absolute path of the filename 
            		path[i++] = '/';
			while(name[j])
				path[i++] = name[j++];
			path[i] = 0;
			j = 0;
			if(stat(path, &buff) == 0)
				return 0;
			else
				return -1;
		}
		/*here we are the next path
		 */
		if(*p == ':') {
			//here we get a absolute path of the filename
            		p = p + 1;
			// insert / to the path
            		path[i++] = '/';
			//Paste the name at the end of the path
			while(name[j])
				path[i++] = name[j++];

			// restart set the value of the related variable
			path[i] = 0;
			i = 0;
			j = 0;
			if(stat(path, &buff) == 0)
				return 0;
		}
		path[i++] = *p;
	}

	return -1;
}


