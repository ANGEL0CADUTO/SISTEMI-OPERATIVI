#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEM_KEY 5000

struct sembuf buf[1];
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo *__buf;
} sem_arg;

int sem_ds;

void wait_sem(int s, int n, int k)
{
	buf[0].sem_num = n;
	buf[0].sem_op = -k;
	buf[0].sem_flg = 0;

redo_w:
	if (semop(s, buf, 1) == -1)
 	{
 		if (errno == EINTR)
 			goto redo_w;
 		else
 		{
 			fprintf(stderr, "Error wait_sem\n");
 			semctl(s, -1, IPC_RMID, NULL);
 			exit(EXIT_FAILURE);
 		}
 	}
}

void signal_sem(int s, int n, int k)
{
	buf[0].sem_num = n;
	buf[0].sem_op = +k;
	buf[0].sem_flg = 0;
redo_s:
	if (semop(s, buf, 1) == -1)
	{
		if (errno == EINTR)
			goto redo_s;
		else
		{
			fprintf(stderr, "Error signal_sem\n");
			semctl(s, -1, IPC_RMID, NULL);
			exit(EXIT_FAILURE);
		}
	}
}

int main()
{
	if ((sem_ds = semget(SEM_KEY, 1, IPC_CREAT|0666)) == -1) 
	{
		fprintf(stderr, "Error semget\n");
		exit(EXIT_FAILURE);
	}
	
	sem_arg.val = 1;
	if (semctl(sem_ds, 0, SETVAL, sem_arg) == -1)
	{
		fprintf(stderr, "Error semctl\n");
		semctl(sem_ds, -1, IPC_RMID, NULL);
		exit(EXIT_FAILURE);
	}
	
	signal_sem(sem_ds, 0, 1); 
	
	wait_sem(sem_ds, 0, 1);
}