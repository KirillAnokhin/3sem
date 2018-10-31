#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <ctype.h>
#include <malloc.h>

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

		if (execvp(argv[0], argv) == -1) 
		{
			perror("Wrong comand");
			// This is child
			return -2;
		}
	}
	return pid;
}


int super_exec(char* argv[], int fd_in, int last_fd_out)
{
	//assert(*argv);
	if (argv == NULL || *argv == NULL)
       	{
		fprintf(stderr, "Wrong command here\n");
		return -1;
	}
	
	/// Find stick
	char** ptr = find_stick(argv);
	int tmp = 0;

	/// If there are no stick
	/// we can just exec this argv
	pid_t this_pid;
	if (*ptr == NULL)
       	{
		this_pid = fork_exec(argv, fd_in, last_fd_out);
		if (this_pid == -1)
	       	{
			perror("fork_exec");
			return -1;
		}
		if (this_pid == -2)
			return -2;
		while (wait(NULL) != -1);
		return 0;
	}
	
	/// We must disable stick to execute 
	/// current argv correctly
	free (*ptr);
	*ptr = NULL; // leak
	ptr++; // next argv
	if (*ptr == NULL)
	{
		fprintf(stderr, "Wrong pipe syntax\n");
		return -1;
	}
	int pipe_fd[2];
	if (pipe(pipe_fd) == -1)
		return -1;

	/// execute this argv
	if(*argv == NULL)
	{
		fprintf(stderr, "Wrong syntax\n");
		return -1;
	}
	this_pid = fork_exec(argv, fd_in, pipe_fd[1]);
	if (this_pid == -1)
       	{
		perror("fork_exec");
		close(pipe_fd[0]);
		close(pipe_fd[1]);
		return -1;
	}
	close(pipe_fd[1]);

	/// execute next argv
	if (super_exec(ptr, pipe_fd[0], last_fd_out) == -1)
	{
		close(pipe_fd[0]);
	       	return -1;
	}
	close(pipe_fd[0]);

	return 0;
}


int lexer(char* str, char*** arr_p)
{
	*arr_p = calloc(100, sizeof(char*));
	int cur_arr = 0;
	char* beg;
	char* cur;
	char* tmp;
	cur = str;
	beg = cur;
	while(1)
	{	
		while(isspace(*beg) && *beg != '\0')
			beg++;
		cur = beg;
		while(!isspace(*cur) && (*cur != '|') && (*cur != '\0'))
			cur++;
		if (beg != cur)
		{
			tmp = calloc((size_t)(cur - beg) + 1, sizeof(char));
			assert(tmp);
			memcpy(tmp, beg, (size_t)(cur - beg));
			(*arr_p)[cur_arr++] = tmp;
		
			beg = cur;
			cur++;
		}
		else if (*cur == '|')
		{
			tmp = calloc(2, sizeof(char));
			memcpy(tmp, beg, 1);
			(*arr_p)[cur_arr++] = tmp;		
			cur++;
			beg = cur;
		}
		else if(beg == cur)
			break;
		else if(*cur == '\0')
			break;
	}
	return cur_arr;
}

int lexer_clean(char** arr)
{
	size_t n_mal = malloc_usable_size(arr)/sizeof(char*);
	for(size_t i = 0; i < n_mal; i++)
	{
		if (arr[i] != NULL)
			free(arr[i]);
	}
	free(arr);
	return 0;
}

int main()
{
	char buf[BUF_SIZE];
	char** comands;
	int ncomands;
	while(1)
	{
		printf("user@linux:$ ");
		fgets(buf, BUF_SIZE, stdin);
		if(feof(stdin))
		{
			printf("EOF in stdin\n");
			return 0;
		}
		ncomands = lexer(buf, &comands);
		for(int i = 0; i < ncomands; i++)
		{
			if(strcmp(comands[i], "exit") == 0)
			{
				lexer_clean(comands);
				return 0;
			}		
			if(comands[i] == NULL)
			{
				printf("failure\n");
				return 0;
			}
		}	
		if(ncomands == 0)
			continue;	
		//if(*comands)
	       //	{
		int tmp = super_exec(comands, STDIN_FILENO, STDOUT_FILENO);
		if (tmp == -1)
		{
			//lexer_clean(comands);
			perror("Wrong cmd");
		}
		else if (tmp == -2)
		{
			lexer_clean(comands);
			return 0;
		}
					
	//	}
		lexer_clean(comands);	
	}	
	lexer_clean(comands);
	return 0;
}

