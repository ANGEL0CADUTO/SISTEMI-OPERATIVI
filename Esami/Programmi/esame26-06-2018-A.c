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

#define SEM_KEY 4098

typedef struct _data {
	int val;
	struct _data *next;
} data;

data *lists;
int num_threads;
int val;
struct sembuf buf[1];
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo *__buf;
} sem_arg;
int sem_ds;

void wait_sem(int k)
{
	buf[0].sem_num = k;
	buf[0].sem_op = -1;
	buf[0].sem_flg = 0;
redo_1:
	if (semop(sem_ds, buf, 1) == -1)
	{
		if (errno == EINTR)
			goto redo_1;
		else
		{
			fprintf(stderr, "Error wait_sem\n");
			semctl(sem_ds, -1, IPC_RMID, sem_arg);
			exit(EXIT_FAILURE);
		}
	}
}

void signal_sem(int k)
{
	buf[0].sem_num = k;
	buf[0].sem_op = 1;
	buf[0].sem_flg = 0;
redo_2:
	if (semop(sem_ds, buf, 1) == -1)
	{
		if (errno == EINTR)
			goto redo_2;
		else
		{
			fprintf(stderr, "Error signal_sem\n");
			semctl(sem_ds, -1, IPC_RMID, sem_arg);
			exit(EXIT_FAILURE);
		}
	}
}

void *subroutine(void *i)
{
	data *aux;
	long int me = (long int)i;
	printf("Thread %ld alive\n", me);
	fflush(stdout);
	
	while (1)
	{
		aux = (data *)malloc(sizeof(data));
		if (aux == NULL)
		{
			fprintf(stderr, "Error malloc\n");
			exit(EXIT_FAILURE);
		}
		
		wait_sem(me);
		
		printf("Thread %ld found value %d\n", me, val);
		fflush(stdout);
		
		aux->val = val;
		
		if (me+1 >= num_threads)
			signal_sem(0);
		else
			signal_sem(me+1);
		
		aux->next = lists[me].next;
		lists[me].next = aux;
	}
	
	exit(EXIT_SUCCESS);
}

void *sigint_handler(void)
{
	data aux;
	int i;
	
	for (i=0; i<num_threads; i++)
	{
		aux = lists[i];
		printf("Printing list %d\n", i);
		while (aux.next != NULL)
		{
			printf("(%d) -> ", aux.next->val);
			aux = *(aux.next);
		}
		printf("\n");
	}
	
}

int main(int argc, char **argv)
{
	long i;
	pthread_t tid;
	int ret;
	
	if (argc != 2)
	{
		fprintf(stderr, "Usage: <prog> <num_threads>\n");
		exit(EXIT_FAILURE);
	}
	
	num_threads = strtol(argv[1], NULL, 10);
	printf("Spawning %d threads\n", num_threads);
	fflush(stdout);
	
	lists = (data *)malloc(sizeof(data)*num_threads);
	if (lists == NULL)
	{
		fprintf(stderr, "Error malloc\n");
		exit(EXIT_FAILURE);
	}
	for (i=0; i<num_threads; i++)
	{
		lists[i].val = -1;
		lists[i].next = NULL;
	}
	
	if ((sem_ds = semget(SEM_KEY, num_threads, IPC_CREAT|0666)) == -1)
	{
		fprintf(stderr, "Error semget\n");
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
	
	signal(SIGINT, sigint_handler);
	
	for (i=0; i<num_threads; i++)
	{
		if (pthread_create(&tid, NULL, subroutine, (void *)i) != 0)
		{
			fprintf(stderr, "Error pthread_create\n");
			exit(EXIT_FAILURE);
		}
	}
	
	i = 0;
	signal_sem(0);
	
	while (1)
	{
		wait_sem(i);
		
		while (scanf("%d", &val) <= 0)
		{
			if (errno != EINTR)
			{
				fprintf(stderr, "Error scanf\n");
				exit(EXIT_FAILURE);
			}
		}
		
		i = (i+1) % num_threads;
		
		signal_sem(i);
	}
	
	exit(EXIT_SUCCESS);
}