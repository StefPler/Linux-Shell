#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> // open
#include <sys/wait.h>

int main()
{
	char *ls[] = {"ls", "-l", NULL};
	char *grep[] = {"grep", "test", NULL};
	int p[2];
	int i;
	pipe(p);
// p[1] write end || p[0] read end
	for (i = 0; i < 2; ++i)
	{	
		if (fork() == 0)
		{

			if (i == 0)
			{
				dup2(p[1],1); // rewrite write end of pipe with stdout
				close(p[1]); //closing write end of pipe
				close(p[0]); //closing reading end of pipe
				execvp(ls[0],ls); 
			}else if (i == 1)
			{
				close(p[1]); // closing write end of pipe
				dup2(p[0],0); // rewrite to read end of pipe with stdin
				close(p[0]);  // closing reading end of pipe
				execvp(grep[0],grep);
			}
		}else
		{
			// the second proccess needs only the input end of the pipe
			// and in order for the input to work, all output ends of
			// the pipe must be closed
			if (i == 1)
			{
				close(p[1]);
			}
			wait(NULL);
		}
	}
	return 0;
}
