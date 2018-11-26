#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

/////======================

typedef struct mymsg mymsg;
struct mymsg{
	long type;
};

int runs_action(int msqid, int i)
{
	mymsg stop;
	stop.type = i + 1;
	mymsg start;	
	if(msgrcv(msqid, (mymsg *) &start, 0, i, 0) == -1) {
		printf("can't receive message\n");
		msgctl(msqid, IPC_RMID, NULL);
		return -1;
	}
	printf("%d runner start\n", i);
	printf("%d runner end\n", i);
	if(msgsnd(msqid, (mymsg *) &stop, 0, 0) < 0) {
		printf("can't send message\n");
		msgctl(msqid, IPC_RMID, NULL);
		return -1;
	}
	return 0;
}

int judge_action(int msqid, int n)
{
	mymsg begin;
	begin.type = 1;
	struct timeval t1;
	struct timeval t2;
	gettimeofday(&t1, NULL);
	printf("begin\n");	
	if(msgsnd(msqid, (mymsg *) &begin, 0, 0) < 0) {
		printf("can't send message\n");
		msgctl(msqid, IPC_RMID, NULL);
		return -1;
	}
	if(msgrcv(msqid, (mymsg *) &begin, 0, n + 1, 0) == -1) {
		printf("can't receive message\n");
		msgctl(msqid, IPC_RMID, NULL);
		return -1;
	}
	gettimeofday(&t2, NULL);
	printf("total time %ld\n", t2.tv_usec - t1.tv_usec);
	return 0;
}

int main(int argc, char* argv[]) {
	if(argc != 2) {
		printf("Expected one argument\n");
		return -1;
	}

	int n_runners = atoi(argv[1]);
	char pathname[] = "run.c";
	key_t key;
	if((key = ftok(pathname, 0)) < 0) {
		printf("can't generate key\n");
		return -1;
	}
	int msqid = 0;

	if((msqid = msgget(key, IPC_CREAT | 0666)) == -1) {
		printf("Can't get msqid\n");
		return -1;
	}
	
	int pid = 1;
	int i = 0;
	for(i; i < n_runners && (pid != 0); i++) 
		pid = fork();

	if(pid) {
		if(judge_action(msqid, n_runners) == -1) {
			printf("judge_action error\n");
			return -1;
		}
	}
		
	if(!pid) {
		if(runs_action(msqid, i) == -1) {
			printf("runs_action error\n");
			return -1;
		}
	}
	if(pid) {
		while(wait(0) != -1) {}	
		if(msgctl(msqid, IPC_RMID, NULL) == -1) {
			printf("msgctl error\n");
			return -1;
		}
	}
	return 0;
}


