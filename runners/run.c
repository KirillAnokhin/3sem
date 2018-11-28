#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <ctype.h>

/////======================

typedef struct mymsg mymsg;
struct mymsg{
	long type;
};

#define macro(do_smth) \
do { \
printf("can't " #do_smth  " message\n"); \
msgctl(msqid, IPC_RMID, NULL); \
return -1;\
} while(0)

int runs_action(int msqid, int i, int n)
{
	mymsg stop;
	stop.type = i + 1;
	mymsg isready;
	isready.type = n + 2;
	mymsg start;

	printf("%d there\n", i);
	if(msgsnd(msqid, (mymsg *) &isready, 0 ,0) < 0) 
		macro(send);
		
	if(msgrcv(msqid, (mymsg *) &start, 0, i, 0) == -1) 
		macro(recieve);

	printf("%d runner start\n", i);
	printf("%d runner end\n", i);
	if(msgsnd(msqid, (mymsg *) &stop, 0, 0) < 0) 
		macro(send);
	
	return 0;
}

int judge_action(int msqid, int n)
{
	mymsg begin;
	mymsg ready;
	begin.type = 1;
	struct timeval t1;
	struct timeval t2;
	for(int i = 0; i < n; i++) {
		if(msgrcv(msqid, (mymsg *) &ready, 0, n + 2, 0) == -1) 
			macro(recieve);
	}

	printf("begin\n");	
	if(msgsnd(msqid, (mymsg *) &begin, 0, 0) < 0) 
		macro(send);

	gettimeofday(&t1, NULL);
	if(msgrcv(msqid, (mymsg *) &begin, 0, n + 1, 0) == -1) 
		macro(receive);

	gettimeofday(&t2, NULL);
	printf("total time %ld\n", t2.tv_usec - t1.tv_usec);
	return 0;
}

int check_arg(char* arg)
{
	int n_runs;
	if(!isdigit(*arg))
		return -1;
	n_runs = atoi(arg);
	return n_runs;
}

int main(int argc, char* argv[]) {
	if(argc != 2) {
		printf("Expected one argument\n");
		return -1;
	}	
	int n_runners = 0;
	if((n_runners = check_arg(argv[1])) == -1) {
		printf("invalid argument\n");
		return -1;
	}
	printf("%d\n", n_runners);
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
		if(runs_action(msqid, i, n_runners) == -1) {
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


