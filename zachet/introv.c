#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define BUF_SIZE 128

int main()
{
	char str[BUF_SIZE];
	strcpy(str, "Hello, world\n");
	key_t key;
	char pathname[] = "introv.c";
	key = ftok(pathname, 0);
	int semid;
	semid = semget(key, 1, 0666 | IPC_CREAT);

	struct sembuf sops[2];
	
	sops[0].sem_num = 0;
	sops[0].sem_op = 0;
	sops[0].sem_flg = IPC_NOWAIT;
	
	sops[1].sem_num = 0;
	sops[1].sem_op = 1;
	sops[1].sem_flg = SEM_UNDO;
	
	if (semop(semid, sops, 2) == -1) {
		strcmp(str, "Goodbye, world\n");
	}
	int i = 0;		
	while (str[i] != '\0') {
		write(STDOUT_FILENO, str + i, 1);
		i++;
		sleep(1);
	}
	
	sops[0].sem_num = 0;
	sops[0].sem_op = -1;
	sops[0].sem_flg = SEM_UNDO;
	
	semop(semid, sops, 1);
	return 0;
}
