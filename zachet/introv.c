#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define BUF_SIZE 128


#define START_SOP()					\
	({						\
		int tmp = semop(semid, sops, n_sops);	\
		n_sops = 0;				\
		tmp;					\
	})

#define ADD_SOP(num, op, flg)			\
	do {					\
		sops[n_sops].sem_num = num;	\
		sops[n_sops].sem_op  = op;	\
		sops[n_sops].sem_flg = flg;	\
		n_sops++;			\
	} while (0)

int main()
{
	char str[BUF_SIZE];
	key_t key;
	int semid;
	char pathname[] = "introv.c";
	struct sembuf sops[1];
	size_t n_sops = 0;

	key = ftok(pathname, 0);
	semid = semget(key, 1, 0666 | IPC_CREAT);

	ADD_SOP(0, 1, SEM_UNDO);
	if (START_SOP() == -1) {
		perror("semop");
		return 0;
	}

	ADD_SOP(0, -2, IPC_NOWAIT);
	ADD_SOP(0,  2, 0);
	if (START_SOP() == -1) {
		if (errno != EAGAIN) {
			perror("semop");
			return 0;
		}
		strcpy(str, "Hello, world\n");
	} else {
		strcpy(str, "Goodbye, world\n");
	}

	for(char* p = str; *p != '\0'; p++) {	
		write(STDOUT_FILENO, p, 1);
		sleep(1);
	}

	ADD_SOP(0, 1, SEM_UNDO);
	if (START_SOP() == -1) {
		perror("semop");
		return 0;
	}
	return 0;
}
