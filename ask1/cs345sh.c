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


/* returns the number of pipes */
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

/* struct to create a list for the commands when we have piping */
typedef struct commands{

	struct commands* next;

	char *par;

} commands_t;

/* adds a command in the list */
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

void free_cmd_list(struct commands* head){

	struct commands* tmp;
	char* strtmp;

	while(head != NULL){

		tmp = head;
		strtmp = head->par;
		head = head->next;
		free(tmp);
		free(strtmp);
	}
}
/* initializes the list */
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
	commands_t* head = malloc(sizeof(commands_t));

	struct passwd *user = getpwuid(getuid());

	const char delim[2] = " ";
	char *parts;
	char *Gpath = getpath();

	rl_bind_key('\t', rl_complete);

	while(TRUE){
		sprintf(prompt, "<%s>@cs345sh%s/$ ", user->pw_name, Gpath);
		
				
		if((command = readline(prompt))){
			commandcp = malloc(sizeof(char) * (strlen(command)) +1);
			commandcp = strcpy(commandcp, command);
			parts = strtok(command, delim);
				
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

				CDstatus = chdir(path);

                        }else{
				CDstatus = chdir("/");
			}	
			
			/*printf("cd was %s, and current path is: %s \n", (CDstatus==0) ? "successful" : "unsuccessful", getpath());*/
			if(CDstatus != 0){
				printf("cd was unsuccessful\n");
			}
			free(path);
			

		}else if(strcmp("exit", parts) == 0){
			exit(0);
		}else{
			int ret_pipe = 0;
			int pfd[2];

			commands_t* current = head;
			head->next = NULL;
			make_list_OC(commandcp, current);
			current = current->next;
			ret_pipe = number_of_pipes(commandcp);

			if(ret_pipe > 1){
				pipe(pfd);
			}

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

 			parts = current->par;
                        parts = strtok(current->par, delim);

                        param = malloc(2*sizeof(char*));
			check_mem(param, par+1);

                        param[0] = malloc(2*strlen(parts));
			check_mem(param[0], 2*strlen(parts));

			param[0] = strcpy(param[0], parts);

			parts = strtok(NULL,delim);
                        param[1] = NULL;
		
	
                        while(parts != NULL){
 
				wordexp(parts, &wild, 0);
				temp = wild.we_wordv;
				
				/* redirection checks */
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

					fd = open(parts, O_CREAT | O_WRONLY | O_APPEND, 0600);
					if(fd < 0){
						fprintf(stderr, "Cannot open file. Try again later.\n");
						abort();
					}
					parts = strtok(NULL, delim);
				
					continue;
				}

				/* wild card management and allocation for exec parameters */
				if(temp[wild_c] == NULL){

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

			child_pid = fork();

			if(child_pid < 0){

				printf("fork failed\n");
				abort();

			}else if(child_pid != 0){
                                /*printf("I am the parent %d, my child is %d \n", getpid(), child_pid);*/

                                if(!ret_D){
					if(ret_pipe > 1){
						if(current->next == NULL){
							close(pfd[1]);
							printf(" father close pdf[1]\n");
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
					
					if(current->next != NULL){
						dup2(pfd[1], 1);
						close(pfd[1]);
						close(pfd[0]);
					}else{
						close(pfd[1]);
						dup2(pfd[0], 0);
						close(pfd[0]);
					}
				}

                                ret_exec = execvp(param[0], param);
                                printf("execvp failed, return: %d \n", ret_exec);
                        }

			current = current->next;
			} 
			/*TODO free */	
			}
		free(command);			
		Gpath = getpath();
		free_cmd_list(head->next);			 
	}

	free(Gpath);

	return 0;


}



