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

#define SIZE 5
#define SEM_KEY1 5001
#define SEM_KEY2 5002

pthread_t tid;
int fd1, fd2;
FILE *f1, *f2;
char  *str;
char buff[SIZE];
int sem_read, sem_write;
struct sembuf buf[1];
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo *__buf;
} sem_arg;

char *command1;
char *command2;

void wait_sem(int s, int k)
{
	buf[0].sem_num = 0;
	buf[0].sem_op = -k;
	buf[0].sem_flg = 0;
redo_1:
	if (semop(s, buf, 1) == -1)
	{
		if (errno == EINTR)
			goto redo_1;
		else
		{
			fprintf(stderr, "Error wait_sem\n");
			semctl(s, -1, IPC_RMID, NULL);
			exit(EXIT_FAILURE);
		}
	}
}

void signal_sem(int s, int k)
{
	buf[0].sem_num = 0;
	buf[0].sem_op = k;
	buf[0].sem_flg = 0;
redo_2:
	if (semop(s, buf, 1) == -1)
	{
		if (errno == EINTR)
			goto redo_2;
		else
		{
			fprintf(stderr, "Error signal_sem\n");
			semctl(s, -1, IPC_RMID, NULL);
			exit(EXIT_FAILURE);
		}	
		
	}
}

void sigint_handler()
{
	system(command1);
	printf("\n");
	system(command2);
	printf("\n");
}

void *thread_1(void *arg)
{
	long int me = (long int)arg;
	char *filename;
	int ret;
	
	signal(SIGINT, SIG_IGN);
	
	filename = (char *)malloc(sizeof(char)*(strlen(str) + 15));
	sprintf(filename, "%s_diretto.txt", str);
	
	command1 = (char *)malloc(sizeof(char)*(strlen(filename)));
	sprintf(command1, "%s\n", filename);
	
	fd1 = open(filename, O_CREAT|O_RDWR, 0666);
	if (fd1 == -1)
	{
		fprintf(stderr, "Error open\n");
		exit(EXIT_FAILURE);
	}
	f1 = fdopen(fd1, "a+");
	if (f1 == NULL)
	{
		fprintf(stderr, "Error fdopen\n");
		exit(EXIT_FAILURE);
	}
	
	while (1)
	{
	
		wait_sem(sem_write, 1);

rewrite:
		ret = fprintf(f1, "%s\n", buff);
		if (ret < 0)
		{
			if (errno == EINTR)
				goto rewrite;
			else
			{
				fprintf(stderr, "Error fprintf\n");
				exit(EXIT_FAILURE);
			}
		}
		fflush(f1);
		signal_sem(sem_read, 1);
		
		printf("Done! %s has been written\n", buff);
		fflush(stdout);
		
	}
}

void *thread_2(void *arg)
{
	long int me = (long int)arg;
	char *filename;
	int ret;
	
	signal(SIGINT, SIG_IGN);
	
	filename = (char *)malloc(sizeof(char)*(strlen(str) + 15));
	sprintf(filename, "%s_inverso.txt", str);
	
	command2 = (char *)malloc(sizeof(char)*(strlen(filename)));
	sprintf(command2, "%s\n", filename);
	
	fd2 = open(filename, O_CREAT|O_RDWR, 0666);
	if (fd2 == -1)
	{
		fprintf(stderr, "Error open\n");
		exit(EXIT_FAILURE);
	}
	f2 = fdopen(fd2, "a+");
	if (f2 == NULL)
	{
		fprintf(stderr, "Error fdopen\n");
		exit(EXIT_FAILURE);
	}
	
	while (1)
	{
	
		wait_sem(sem_write, 1);

rewrite:
		ret = fprintf(f2, "%s\n", buff);
		if (ret < 0)
		{
			if (errno == EINTR)
				goto rewrite;
			else
			{
				fprintf(stderr, "Error fprintf\n");
				exit(EXIT_FAILURE);
			}
		}
		fflush(f2);
		signal_sem(sem_read, 1);
		
	}
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: <prog_name> <string S>\n");
		exit(EXIT_FAILURE);
	}
	
	int num_threads = 2;
	int i, ret;
	
	str = (char *)malloc(sizeof(char)*strlen(argv[1]));
	strcpy(str, argv[1]);
	
	if ((sem_read = semget(SEM_KEY1, 1, IPC_CREAT|0666)) == -1)
	{
		fprintf(stderr, "Error semget\n");
		exit(EXIT_FAILURE);
	}
	
	if ((sem_write = semget(SEM_KEY2, 1, IPC_CREAT|0666)) == -1)
	{
		fprintf(stderr, "Error semget\n");
		exit(EXIT_FAILURE);
	}
	
	if (semctl(sem_read, 0, SETVAL, 2) == -1)
	{
		fprintf(stderr, "Error semctl\n");
		semctl(sem_read, -1, IPC_RMID, NULL);
		exit(EXIT_FAILURE);
	}
	
	if (semctl(sem_write, 0, SETVAL, 0) == -1)
	{
		fprintf(stderr, "Error semctl\n");
		semctl(sem_write, -1, IPC_RMID, NULL);
		exit(EXIT_FAILURE);
	}
	
	if (pthread_create(&tid, NULL, thread_1, (void *)0) == -1)
	{
		fprintf(stderr, "Error pthread_create\n");
		exit(EXIT_FAILURE);
	}
	
	if (pthread_create(&tid, NULL, thread_2, (void *)1) == -1)
	{
		fprintf(stderr, "Error pthread_create\n");
		exit(EXIT_FAILURE);
	}
	
	signal(SIGINT, sigint_handler);
	
	while (1)
	{
		wait_sem(sem_read, 2);
		printf("Type something: ");
		fflush(stdout);
			
		//fgets(buff, SIZE, stdin);
		while (scanf("%5s", buff) == -1)
		{
			if (errno != EINTR)
			{
				fprintf(stderr, "Error scanf\n");
				exit(EXIT_FAILURE);
			}
		}
		buff[SIZE] = '\0';
		
		signal_sem(sem_write, 2);
		
		printf("Files updating by threads\n");
		fflush(stdout);
	}
	
}