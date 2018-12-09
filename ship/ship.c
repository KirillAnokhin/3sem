#include<sys/sem.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<stdarg.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>

#define CNG(SEMNAME, N)						\
{								\
	if(change_sem(semid, SEMNAME, (N)) == -1)		\
		return -1;					\
}	

enum {
	SEM_BOAT_IN,
	SEM_STAIR,
	SEM_BOAT_WAIT,
	SEM_START,
	SEM_FINISH,
	SEM_BOAT_OUT,
	SEM_COAST,
};

int n_pass  = 0;
int n_boat  = 0;
int n_stair = 0;
int n_trip  = 0;

int change_sem(int semid, int sem_name, int n)
{
	struct sembuf buf = {sem_name, n, 0}; 
	if (semop(semid, &buf, 1) == -1) {
		perror("SEM_ERROR\n");    
		return -1;
	}
	return 0;	
}

int ship_action(int semid)
{
	CNG(SEM_COAST, (n_pass));
	printf("boat sailed\n");
	for(int i = 0; i < n_trip; i++) {
		printf("stair is lowered\n");
		CNG(SEM_BOAT_IN, n_boat);
		CNG(SEM_STAIR, n_stair);
		CNG(SEM_BOAT_WAIT, (-n_boat));
		CNG(SEM_STAIR, (-n_stair));
		printf("stair is raised\n");
		CNG(SEM_START, n_boat);
		CNG(SEM_FINISH, (-n_boat));
		printf("boat is back.\n");
		CNG(SEM_BOAT_OUT, n_boat);
	}
	CNG(SEM_STAIR, (n_stair));
	printf("it was last travelling.\n");
	CNG(SEM_COAST, (-n_pass));
	return 0;
}

int pas_action(int semid, int id)
{
	printf("%d'th is hear.\n", id);
	while(1) {
		CNG(SEM_BOAT_IN, (-1));
		CNG(SEM_COAST, (-1));
		CNG(SEM_STAIR, (-1));
		printf("%d'th go on ship\n", id);
		CNG(SEM_STAIR, 1);
		CNG(SEM_BOAT_WAIT, 1);
		CNG(SEM_START, (-1));
		printf("%d'th are ready to back\n", id);
		CNG(SEM_FINISH, 1);
		CNG(SEM_STAIR, (-1));
		printf("%d'th go out ship\n", id);
		CNG(SEM_BOAT_OUT, (-1));
		CNG(SEM_STAIR, 1);
		CNG(SEM_COAST, 1);
	}
	return 0;
}

int main(int argc, char * argv[])
{
	if(argc != 5) {
		printf("expected 4 args\n");
		return 0;
	}
	n_pass = atoi(argv[1]);
	n_boat = atoi(argv[2]);
	n_stair = atoi(argv[3]);
	n_trip = atoi(argv[4]);

	if(n_boat > n_pass)
		n_boat = n_pass;
	int semid;
	if((semid = semget(IPC_PRIVATE, 8, IPC_CREAT|IPC_EXCL|0666)) == -1 ) {
		perror(strerror(errno));
		return -1;
	}
	pid_t name_proc[n_pass + 1];
	for(int i = 0; i < n_pass + 1; i++) {
		int pid = fork();
		if(pid == -1) {
			perror(strerror(errno));
			return -1;
		}
		if(pid == 0) {
			if(i == 0) {
				if(ship_action(semid) == -1)
					printf("ship_action error\n");
			} else 
				if(pas_action(semid, i) == -1) {
					printf("pass_ction error\n");
					for(int j = 0; j < n_pass + 1; j++) 
						if(j != i)
							kill(name_proc[j],SIGKILL);
					kill(getppid(), SIGKILL);
				}	
			return 0;
		}
		else
			name_proc[i] = pid;
	}
	
	wait(NULL);
	for(int i = 1; i < n_pass + 1; i++)
		kill(name_proc[i],SIGKILL);
	semctl(semid, IPC_RMID, 0);
	return 0;	
}
