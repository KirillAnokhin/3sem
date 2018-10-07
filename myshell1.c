#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUF_SIZE 512


char **find_stick(char **argv)
{
	for (; *argv != NULL && strcmp(*argv, "|") != 0; argv++);
	return argv;
}

/// returns pid of process
int fork_exec(char *argv[], int fd_in, int fd_out)
{
	int pid = fork();
	if (pid == 0)
       	{
		if (fd_in != STDIN_FILENO) 
		{
			close(STDIN_FILENO);
			dup(fd_in);
			close(fd_in);
		}

		if (fd_out != STDOUT_FILENO)
	       	{
			close(STDOUT_FILENO);
			dup(fd_out);
			close(fd_out);
		}

		execvp(argv[0], argv);
	}
	return pid;
}

int super_exec(char* argv[], int fd_in, int last_fd_out)
{
	if (argv == NULL || *argv == NULL)
		return -1;

	/// Find stick
	char** ptr = find_stick(argv);
	/// If there are no stick
	/// we can just exec this argv
	pid_t this_pid;
	if (*ptr == NULL)
       	{
		this_pid = fork_exec(argv, fd_in, last_fd_out);
		if (this_pid == -1)
			return -1;
		/// Don't quit before this proc exit
		waitpid(this_pid, NULL, 1);
		usleep(10000);
		return 0;
	}
	
	/// We must disable stick to execute 
	/// current argv correctly
	*ptr = NULL;
	ptr++; // next argv
	int pipe_fd[2];
	if (pipe(pipe_fd) == -1)
		return -1;

	/// execute this argv
	this_pid = fork_exec(argv, fd_in, pipe_fd[1]);
	if (this_pid == -1)
		return -1;

	/// execute next argv
	if (super_exec(ptr, pipe_fd[0], last_fd_out) == -1)
	       	return -1;

	/// Don't quit before this proc exit
	waitpid(this_pid, NULL, 1);
	usleep(10000);
	return 0;
}


int main()
{
	while(1)
	{
		printf("user@linux:$ ");
		///all information in buf
		char buf[BUF_SIZE] = {};
		char* comands[BUF_SIZE] = {};

		fgets(buf, BUF_SIZE, stdin);

		char* token;
		char* saveptr;
		char* str = buf;
		int i = 0;

		for(str = buf, i = 0; ; i++, str = NULL)
		{
			token = strtok_r(str, " \n\v\t", &saveptr);
			comands[i] = token;

			if(token == NULL)
				break;
		
			if(strcmp(comands[i], "exit") == 0)
				return 0;

			//printf("%d: %s\n", i, comands[i]);	
		}


		if(super_exec(comands, STDIN_FILENO, STDOUT_FILENO) == -1)
	       	{
			perror("super_exec");
			return 0;
		}
	}	
	
	return 0;
}

