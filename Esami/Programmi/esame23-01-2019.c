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

#define MAX_SIZE 4096
#define SEM_KEY1 4096
#define SEM_KEY2 4097

typedef struct __thread_args {
	int me;
	char *my_string;
} thread_args;

char buff[MAX_SIZE];
int num_thread;
FILE *f;
char *filename;
int sem_read, sem_write;
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo *__buf;
} sem_arg;
struct sembuf buf[1];

void sigint_handler()
{
	char cmd[MAX_SIZE];
	printf("\n");
	sprintf(cmd, "cat %s", filename);
	system(cmd);
}

void wait_sem(int s, int k, int n)
{
	buf[0].sem_num = k;
	buf[0].sem_op = -n;
	buf[0].sem_flg = 0;
redo_1:
	if (semop(s, buf, 1) == -1 )
	{
		if (errno == EINTR)
			goto redo_1;
		else
		{
			fprintf(stderr, "Error wait_sem\n");
			exit(EXIT_FAILURE);
		}
	}
}

void signal_sem(int s, int k, int n)
{
	buf[0].sem_num = k;
	buf[0].sem_op = n;
	buf[0].sem_flg = 0;
redo_2:
	if (semop(s, buf, 1) == -1)
	{
		if (errno == EINTR)
			goto redo_2;
		else
		{
			fprintf(stderr, "Error signal_sem\n");
			exit(EXIT_FAILURE);
		}
	}
}

void *subroutine(void *i)
{
	int ret;
	thread_args *my_args;
	my_args = (thread_args *)i;
	int me = my_args->me;
	
	printf("Thread %d in charge of string %s\n", me, my_args->my_string);
	
	while (1)
	{
		wait_sem(sem_write, me, 1);
		
		if (strcmp(buff, my_args->my_string) == 0)
		{
			strcpy(buff, "*");
			for (i=0; i<(strlen(my_args->my_string)-1); i++)
			{
				strcat(buff, "*");
			}
		}
		
		signal_sem(sem_read, me, 1);
	}
}

int main(int argc, char **argv)
{
	pthread_t tid;
	struct sigaction sa;
	int ret, i;
	
	if (argc < 3)
	{
		fprintf(stderr, "Usage: <prog> <filename> <str1> ... [<strN>]\n");
		exit(EXIT_FAILURE);
	}
	
	num_thread = argc - 2;
	filename = argv[1];
	
	f = fopen(filename, "w+");
	if (f == NULL)
	{
		fprintf(stderr, "Error fopen\n");
		exit(EXIT_FAILURE);
	}
	
	if ((sem_read = semget(SEM_KEY2, num_thread, IPC_CREAT|0666)) == -1)
	{
		fprintf(stderr, "Error semget\n");
		semctl(sem_read, -1, IPC_RMID, sem_arg);
		exit(EXIT_FAILURE);
	}
	
	if ((sem_write = semget(SEM_KEY1, num_thread, IPC_CREAT|0666)) == -1)
	{
		fprintf(stderr, "Error semget\n");
		semctl(sem_write, -1, IPC_RMID, sem_arg);
		exit(EXIT_FAILURE);
	}
	
	for (i=0; i<num_thread; i++)
	{
		sem_arg.val = 1;
		if (semctl(sem_read, i, SETVAL, sem_arg) == -1)
		{
			fprintf(stderr, "Error init\n");
			semctl(sem_read, -1, IPC_RMID, sem_arg);
			exit(EXIT_FAILURE);
		}
		
		sem_arg.val = 0;
		if (semctl(sem_write, i, SETVAL, sem_arg) == -1)
		{
			fprintf(stderr, "Error init\n");
			semctl(sem_write, -1, IPC_RMID, sem_arg);
			exit(EXIT_FAILURE);
		}
	}
	
	signal(SIGINT, sigint_handler);
	
	for (i=0; i<num_thread; i++)
	{
		thread_args *cur_args = (thread_args *)malloc(sizeof(thread_args));
		cur_args->me = i;
		cur_args->my_string = argv[i+2];
		
		ret = pthread_create(&tid, NULL, subroutine, (void *)cur_args);
		if (ret != 0)
		{
			fprintf(stderr, "Error pthread_create\n");
			exit(EXIT_FAILURE);
		}
	}
	
	strcpy(buff, "\0");
	
	
	while (1)
	{
		for (i=0; i<num_thread; i++)
		{
			wait_sem(sem_read, i, 1);
		}
		
		if (strcmp(buff, "\0") != 0)
		{
			fprintf(f, "%s\n", buff);
			fflush(f);
		}
		
		while (scanf("%s", buff) <= 0)
		{
			if (errno != EINTR)
			{
				fprintf(stderr, "Error scanf\n");
				exit(EXIT_FAILURE);
			}
		}
		
		for (i=0; i<num_thread; i++)
		{
			signal_sem(sem_write, i, 1);
		}
	}
	
}