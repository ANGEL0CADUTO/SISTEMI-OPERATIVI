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

#define SEM_KEY 6789
#define NUM_CHARS 5
#define SIZE 1024

char **files;
char buffer[NUM_CHARS];
//char other_buffer[NUM_CHARS];
int num_threads;
int sem_ds;

struct sembuf buf[1];
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo *__buf;
} sem_arg;

void wait_sem(int k)
{
	buf[0].sem_num = k;
	buf[0].sem_op = -1;
	buf[0].sem_flg = 0;
	
	if (semop(sem_ds, buf, 1) == -1)
	{
		fprintf(stderr, "Error semop\n");
		exit(EXIT_FAILURE);
	}
}

void signal_sem(int k)
{
	buf[0].sem_num = k;
	buf[0].sem_op = 1;
	buf[0].sem_flg = 0;
	
	if (semop(sem_ds, buf, 1) == -1)
	{
		fprintf(stderr, "Error semop\n");
		exit(EXIT_FAILURE);
	}
}

void *subroutine(void *i)
{
	long int me = (long int)i;
	int res;
	
	FILE *f = fopen(files[me], "w+");
	if (f == NULL)
	{
		fprintf(stderr, "Error fopen\n");
		exit(EXIT_FAILURE);
	}
	
	while (1)
	{
		wait_sem(me);
		printf("Thread %ld ready to read %d bytes\n", me, NUM_CHARS);
		fflush(stdout);
		if (fwrite(buffer, strlen(buffer), 1, f) < 0)
		{
			fprintf(stderr, "Error fwrite\n");
			fclose(f);
			exit(EXIT_FAILURE);
		}
		printf("Bytes has been written\n");
		fflush(stdout);
		signal_sem(me+1);
	}
	
	fclose(f);
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: <program_name> <file_1> <file_2> ... [<file_n>]\n");
		exit(EXIT_FAILURE);
	}
	
	num_threads = argc-1;
	pthread_t tid;
	int ret, i;
	
	files = argv+1;
	
	if ((sem_ds = semget(SEM_KEY, num_threads, IPC_CREAT|0666)) == -1)
	{
		fprintf(stderr, "Error semget\n");
		semctl(sem_ds, -1, IPC_RMID, sem_arg);
		exit(EXIT_FAILURE);
	}
	
	sem_arg.val = 0;
	for (i=0; i<num_threads; i++)
	{
		if (semctl(sem_ds, i, SETVAL, sem_arg) == -1)
		{
			fprintf(stderr, "Error semctl\n");
			semctl(sem_ds, -1, IPC_RMID, sem_arg);
			exit(EXIT_FAILURE);
		}
	}
	
	signal_sem(0);
	
	for (i=0; i<num_threads; i++)
	{
		ret = pthread_create(&tid, NULL, subroutine, (void *)i);
		if (ret != 0)
		{
			fprintf(stderr, "Error pthread_create\n");
			exit(EXIT_FAILURE);
		}
	}
	
	i = 0;
	while (1)
	{	
		printf("Type something :");
		fflush(stdout);
		
		fgets(buffer, 5, stdin);
	}
	
	exit(EXIT_SUCCESS);
}