/* Author: Stefanos Pleros
// A.M: csd3593
//
// File Name: cs345sh.c
// -------------------------------------------  
// 
// This is an implementation of a linux shell.
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <wordexp.h>
#include <sys/stat.h> 
#include <fcntl.h>

#define TRUE 1

/* returns the current path */
char *getpath(){

	long size;
	char *cwd;
	char *buf;

	size = pathconf(".", _PC_PATH_MAX);

        if((buf = (char *)malloc((size_t)size)) != NULL){
        	cwd = getcwd(buf, (size_t)size);
        }

	return cwd;

}

/* returns 1 for daemon process and 0 for non daemon */
int is_daemon(char **parameters, char *string, int length){

	/* checks if daemon */
	if(string[strlen(string)-1] == '&'){ 

		/* removing the daemon operator */
		string[strlen(string)-1] = '\0';
		
		/* removing the extra arguments if space was left for deamon operator */
		if(strlen(string) ==  0){
			parameters[length -1] = NULL;
		}
		
		return 1;
	}
	
	return 0;
	
}



int number_of_pipes(char *command){

	char *cmdcp = malloc(sizeof(char) * (strlen(command) +1));
	int loops = 1;
	char *token;
	char delim[2] = " ";
	cmdcp = strcpy(cmdcp, command);
	token = strtok(cmdcp, delim);

	while(token != NULL){
		if(strcmp(token, "|") == 0){			
			++loops;
		}
		token = strtok(NULL, delim);
	}

	return loops;
}

int **dynamic_pfd(char *commands){
	int c = 0;
	int pipes;
	int **d_pfd;

	pipes = number_of_pipes(commands);
	pipes -=1;
	d_pfd = malloc(sizeof(int *)*(pipes));
	
	while(c < pipes){
		
		d_pfd[c] = malloc(sizeof(int)*2);
		
		c++;
	}

	return d_pfd;

}

typedef struct commands{

	struct commands* next;

	char *par;

} commands_t;

void push_command(commands_t* head, char *par) {
    commands_t* current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    /* add a new command */
    current->next = malloc(sizeof(commands_t));
    current->next->par = malloc(sizeof(char) * (strlen(par)+1));
    current->next->par = strcpy(current->next->par, par);
    current->next->next = NULL;
}

void make_list_OC(char *command, commands_t* head){

	 char* cmdcp = malloc(sizeof(char) * (strlen(command) +1));
         char* token = NULL;
	 char delim[4] = "|";
	 cmdcp = strcpy(cmdcp, command);

         if(number_of_pipes(command) <= 1){
                 push_command(head, command);
		 return;
         }

	 token = strtok(cmdcp, delim);
         while(token != NULL){

                push_command(head, token);
		 
		token = strtok(NULL, delim);
         }
}
/* checks if dup2 is needed for each of the redirection tokens */
int needs_dup2(char* string){

	if((strcmp(string, ">") == 0)){
		return 1;

	}else if((strcmp(string, "<") == 0)){
		return 2;

	}else if((strcmp(string, ">>") == 0)){
		return 3;
	}

		return -1;	
}

/* checks if malloc/realloc successfuly returned an address */
void check_mem(void *pointer, size_t bytes){

	if(pointer == NULL){
        	fprintf(stderr, "Fatal: failed to allocate %u bytes.\n", bytes );  
		abort();                         	
	}
}


int main(int argc, char *argv[]){

	char *command;
	char *commandcp;
	char prompt[100];
	struct passwd *user = getpwuid(getuid());

	const char delim[2] = " ";
	char *parts;
	char *Gpath = getpath();
	int status;

	rl_bind_key('\t', rl_complete);

	while(TRUE){
		sprintf(prompt, "<%s>@cs345sh%s/$ ", user->pw_name, Gpath);
		
				
		if((command = readline(prompt))){
			commandcp = malloc(sizeof(char) * (strlen(command)) +1);
			commandcp = strcpy(commandcp, command);
			parts = strtok(command, delim);
			if(parts != NULL){
				printf("Command was: %s \n", parts);
				
			}
		}

		if(strcmp("cd", parts) == 0){
			int CDstatus = 1;
			char *path = NULL;	
	
			parts = strtok(NULL,delim);
			if(parts != NULL){
				path = malloc(strlen(parts));
				path = strcpy(path, parts);
			}

                        if(path != NULL){

                                printf("Path was: %s \n", path);
				CDstatus = chdir(path);

                        }else{
				CDstatus = chdir("/");
			}	
			
			printf("cd was %s, and current path is: %s \n", (CDstatus==0) ? "successful" : "unsuccessful", getpath());
			free(path);
			

		}else if(strcmp("exit", parts) == 0){
			exit(0);
		}else{
			int ret_pipe = 0;
			int **d_pfd = dynamic_pfd(commandcp);
			int c_c = 0;
			commands_t* head = malloc(sizeof(commands_t)); 
			
			commands_t* current = head;
			head->next = NULL;
			make_list_OC(commandcp, current);
			current = current->next;
			printf("at least I am in\n");
			ret_pipe = number_of_pipes(commandcp);

			if(ret_pipe > 1){
				int pipes = 0;

				while(pipes < (ret_pipe-1)){

					pipe(d_pfd[pipes]);
					pipes++;
				}
			}
			/*HERE CHANGE ---------------------------------- */
			while(current!=NULL){

			pid_t child_pid;
                        int ret_exec = 0;
			int ret_D = 0;
			int prev_dup2 = 0;
			int fd;
                        int par=1;
                        char **param;
			wordexp_t wild;
			char **temp;
			int wild_c = 0;

			printf("number of Pipes %d\n", ret_pipe);
 			parts = current->par;
                        parts = strtok(current->par, delim);
			printf("first is: %s\n", parts);

                        param = malloc(2*sizeof(char*));
			check_mem(param, par+1);

                        param[0] = malloc(2*strlen(parts));
			check_mem(param[0], 2*strlen(parts));

			param[0] = strcpy(param[0], parts);

			parts = strtok(NULL,delim);
			printf("second is: %s\n", parts);
                        param[1] = NULL;
		
	
                        while(parts != NULL){
 
				wordexp(parts, &wild, 0);
				temp = wild.we_wordv;
				
				if(needs_dup2(parts) > 0){
					prev_dup2 = needs_dup2(parts);
					parts = strtok(NULL, delim);
					continue;
				}

				if((prev_dup2 == 1) && (parts!=NULL)){
					
					fd = open(parts, O_WRONLY | O_CREAT | O_TRUNC, 0600);
					if(fd < 0){
						fprintf(stderr, "Cannot open file. Try again later.\n");
						abort();
					}
					parts = strtok(NULL, delim);
						
					continue;
				}

				if((prev_dup2 == 2) && (parts!=NULL)){
					
					fd = open(parts, O_RDONLY);
					if(fd < 0){
                                                 fprintf(stderr, "Cannot open file. Try again later.\n");
                                                 abort();
                                        }
					parts = strtok(NULL, delim);
			
					continue;

				}

				if((prev_dup2 == 3) && (parts!=NULL)){

					fd = open(parts, O_WRONLY | O_APPEND);
					if(fd < 0){
						fprintf(stderr, "Cannot open file. Try again later.\n");
						abort();
					}
					parts = strtok(NULL, delim);
				
					continue;
				}


				if(wild.we_wordc <= 1){

	                        	param = realloc(param, (par+2)*sizeof(char*));
                                	check_mem(param, (par+2)*sizeof(char*));

                                	param[par] = malloc(sizeof(char)*(strlen(parts)+1));
					check_mem(param[par], sizeof(strlen(parts)));

                                	param[par] = strcpy(param[par], parts);
                                	param[par+1] =  NULL;
					par++;
				}else{
					param = realloc(param, (par+2+wild.we_wordc)*sizeof(char*));
					check_mem(param, (par+2+wild.we_wordc)*sizeof(char*));
	
					while(wild_c < wild.we_wordc){
						param[par] = malloc(sizeof(char) * (strlen(temp[wild_c])) +1);
						param[par] = strcpy(param[par], temp[wild_c]);
						param[par+1] = NULL;

						wild_c++;
						par++;
					}				
				wordfree(&wild);
				}
                                parts = strtok(NULL, delim);
                        }
	
			ret_D = is_daemon(param, param[par-1], par);		
			printf("command was daemon ret: %d\n", ret_D);

			child_pid = fork();

			if(child_pid < 0){

				printf("fork failed\n");
				abort();

			}else if(child_pid != 0){
                                /*printf("I am the parent %d, my child is %d \n", getpid(), child_pid);*/

                                if(!ret_D){
					/* waitpid(child_pid, &status, 0);*/
					if(ret_pipe > 1){
						if(c_c > 0){
							close(d_pfd[c_c-1][0]);
						}
				
						if(c_c != (ret_pipe-1)){
							close(d_pfd[c_c][1]);
						}
					}
					wait(NULL);
					if(prev_dup2 >0){
						close(fd);
					}
				}else{
					printf("background process: %s | pid: %d\n",param[0], child_pid);
				}
                        }else{
                              /*  printf("I am the child %d, my parent is %d \n", getpid(), getppid()); */
				printf(" prev_dup2: %d\n", prev_dup2);				
				if(prev_dup2 == 1){
					close(1);
					dup2(fd, 1);
				}
				if(prev_dup2 == 2){
					close(0);
					dup2(fd, 0);
					
				}
				if(prev_dup2 == 3){
					close(1);
					dup2(fd, 1);
				}

				if(ret_pipe > 1){
					
					if(c_c == 0){
						printf(" process1 \n");
						dup2(d_pfd[c_c][1], 1);
						close(d_pfd[c_c][1]);
						close(d_pfd[c_c][0]);
						printf(" process1 \n");
					}else if(c_c == (ret_pipe-1)){
						if(ret_pipe > 2){
							c_c=c_c+1;
						}
						printf(" process2 \n");
						close(d_pfd[c_c-1][1]);
						dup2(d_pfd[c_c-1][0], 0);
						close(d_pfd[c_c-1][0]);
						printf(" process2 \n");
					}else{
						printf("process 3\n");
						dup2(d_pfd[c_c][0], 0);
						dup2(d_pfd[c_c +1][1], 1);
						close(d_pfd[c_c][0]);
						close(d_pfd[c_c][1]); 
						printf("process 3\n");
					}
				}

                                ret_exec = execvp(param[0], param);
                                printf("execvp failed, return: %d \n", ret_exec);
                        }
			c_c++;
			current = current->next;
			} 
			/*TODO free */	
			}
		free(command);			
		Gpath = getpath();
				 
	}

	free(Gpath);

	return 0;


}



