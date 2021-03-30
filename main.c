#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <malloc.h>

#define MAX_LINE 80 /* The maximum length command */
#define	READ_END	0 /* define stdin */
#define	WRITE_END	1 /* define stdout */

void redirect(char *str,char **args,int redirect_pos);
void ex_pipe(char **args_pipe);

int main(void){
	char *line = (char*)malloc(sizeof(char) * MAX_LINE); /* first input command line(include spaces) */
	int should_run = 1; /* flag to determine when to exit program */
	pid_t pid; /* process id to distinguish parent and child */
	int pipe_pos = 0; /* flag to store index of "|" in args array. default is 0(not founded) */

	while(should_run){
		printf("osh>");
		fflush(stdout);

		fgets(line,MAX_LINE,stdin); /* read line from stdin and store in line */
		line[strlen(line) - 1] = '\0'; /* remove "\n" in last element of array */

		int i = 0;
		int params_num = 1;
		while(line[i] != '\0'){ /* find out the number of parameters by counting spaces */
			if(line[i] == ' '){
				params_num++;
			}
			i++;
		}

		/* make dynamic array to store command line arguments separated by space */
		char **args = (char **)malloc(sizeof(char *) * params_num);
		/*make dynamic array to store another command when using pipe */
		char **args_pipe = (char **)malloc(sizeof(char *) * params_num);

		i = 0;
		args[i] = strtok(line," ");
		while(args[i] != NULL){ /* insert parameters in array */
			i++;
			args[i] = strtok(NULL," "); /*arguments seperated by space */
		}

		i = 0;
		char *redirect_file;

		while(args[i] != NULL){
			if (!strcmp(args[i],">") | !strcmp(args[i],"<")){ /* when args include redirect operands */
				redirect_file = args[i+1];
				redirect(redirect_file,args,i); /* redirect */

				if(!strcmp(args[params_num -1],"&")){ /* after redirection, modify command line */
					args[params_num -1] = NULL;
					args[i] = "&";
				}
				else {
					args[i] = NULL;
				}
				args[i+1] = NULL;
				params_num = params_num - 2;
			}
			else if(!strcmp(args[i],"|")){ /* when args include pipe operands */
				pipe_pos = i;
				int t = 0;
				args[i] = NULL;
				while(t < params_num - i - 1){ /* args_pipe store pipe arguments */
					if(!strcmp(args[i + t + 1],"&")){
						args[i] = "&";
						args_pipe[t] = NULL;
						pipe_pos++;
					}
					else {
						args_pipe[t] = args[i + t + 1];
						args[i + t + 1] = NULL;
					}
					t++;
				}
				if (pipe_pos != i){
					params_num++;
				}
				params_num = params_num - t - 1;

			}
			i++;
		}

		pid = fork();

		if (pid < 0){ /* error occurred */
			fprintf(stderr, "Fork Failed");
			return -1;
		}
		else if (pid == 0){ /*child process */
			if(!strcmp(args[params_num -1],"&")){ /* if child process include "&", remove it */
				args[params_num -1] = NULL;
			}
			if (pipe_pos != 0){
				ex_pipe(args_pipe); /* pipe */
			}
			//sleep(5); /* give sleep command to check background operation */
			execvp(args[0],args); /* operate command */

		}
		else { /* parent process */
			if(!strcmp(args[params_num -1],"&")){ /* Background,the parent does not wait child process */
				waitpid(pid,NULL,WNOHANG);
			}
			else { /* Foreground,the parent waits until child processes are completed */
				waitpid(pid,NULL,0);
			}
		}
		should_run = 0; /* after parent and child process, change flag */
	}
	return 0;
}

void redirect(char *str,char **args,int redirect_pos){
	int fd; /* file descriptor number to redirect */
	mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH; /* file permission when opening file */

	if(!strcmp(args[redirect_pos],">")){ /* case 1 : redirect ">" */
		if ((fd = open(str,O_CREAT | O_WRONLY | O_TRUNC,mode)) == -1){ /* open file to write stdout */
			fprintf(stderr, "File Can Not Open");
		}
		dup2(fd, STDOUT_FILENO); /* redirect stdout to fd */
		close(fd);
	}
	else { /* case 2 : redirect "<" */
		if ((fd = open(str,O_RDONLY,mode)) == -1){ /* open existing file to give stdin*/
			fprintf(stderr, "File Not Found");
		}
		dup2(fd,STDIN_FILENO); /* redirect stdin to fd */
		close(fd);
	}
}

void ex_pipe(char **args_pipe){
	/* if args include pipe operand("|"),
	 * process creates another child process and communicate using pipe */
	int fd[2];
	pid_t pid_pipe;

	if (pipe(fd) == -1) { /* create pipe */
		fprintf(stderr, "Pipe failed");
	}

	pid_pipe = fork(); /* create another child process */

	if (pid_pipe < 0){ /* error occurred */
		fprintf(stderr, "Pipe Fork Failed");
	}
	else if (pid_pipe == 0){ /*pipe child process */
		close(fd[WRITE_END]); /* close unused write pipe */
		dup2(fd[READ_END],STDIN_FILENO); /* redirect stdin */
		close(fd[READ_END]); /* after duplicating file descriptor,close read pipe */
		execvp(args_pipe[0],args_pipe);
	}
	else { /* pipe parent process */
		close(fd[READ_END]); /* close unused read pipe */
		dup2(fd[WRITE_END], STDOUT_FILENO); /* redirect stdout */
		close(fd[WRITE_END]); /* after duplicating file descriptor,close write pipe */
	}

}
