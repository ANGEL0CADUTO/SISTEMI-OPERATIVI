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

#define SIZE 4096

FILE *file;
pid_t pid;
int fd;
char *filename;
long num_threads;
char **memory_segments;
int sd1, sd2;

void parent_handler(int signo)
{
	printf("Parent received signal %d - Forwarding to child (pid %d)\n", signo, pid);
	kill(pid, signo);
}

void child_handler()
{
	char buff[1024];
	
	sprintf(buff, "cat %s\n", filename);
	system(buff);
}

void *parent_worker(void *arg)
{
	long me;
	struct sembuf oper;
	int ret;
	
	me = (long)arg;
	printf("Parent worker %d started up\n", me);
	
	oper.sem_num = me;
	oper.sem_flg = 0;
	
	while (1)
	{
		oper.sem_op = -1;
redo1:
		ret == semop(sd1, &oper, 1);
		if (ret == -1 && errno != EINTR)
		{
			fprintf(stderr,"Error semop\n");
			exit(EXIT_FAILURE);
		}
		if (ret == -1)
			goto redo1;	
redos:
		ret = scanf("%s", memory_segments[me]);
		if (ret == EOF && errno != EINTR)
		{
			fprintf(stderr,"Error scanf");
			exit(EXIT_FAILURE);
		}
		if (ret == -1)
			goto redos;
		
		printf("Parent worker - thread %d wrote string %s\n", me, memory_segments[me]);
		
		oper.sem_op = 1;
redo2:
		ret = semop(sd2, &oper, 1);
		if (ret == -1 && errno != EINTR)
		{
			fprintf(stderr,"Error semop\n");
			exit(EXIT_FAILURE);
		}
		if (ret == -1) goto redo2;
			
	}
	
	return NULL;
}

void *child_worker(void *arg)
{
	long me;
	struct sembuf oper;
	int ret;
	
	me = (long)arg;
	
	printf("Child worker %d started up\n", me);
	
	oper.sem_num = me;
	oper.sem_flg = 0;
	
	while (1)
	{
redo1:
		oper.sem_op == -1;
		ret = semop(sd2, &oper, 1);
		if (ret == -1 && errno != EINTR)
		{
			fprintf(stderr,"Error semop\n");
			exit(EXIT_FAILURE);
		}
		if (ret == -1)
			goto redo1;
		
		printf("Child worker - thread %d found string %s\n", me, memory_segments[me]);
		fprintf(file, "%s", memory_segments[me]);
		fflush(file);
redo2:
		oper.sem_op = 1;
		ret = semop(sd1, &oper, 1);
		if (ret == -1 && errno != EINTR)
		{
			fprintf(stderr,"Error semop\n");
			exit(EXIT_FAILURE);
		}
		if (ret == -1) 
			goto redo2;
	}
	
	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t tid;
	int exit_code;
	long i;
	int ret;
	
	if (argc < 3)
	{
		fprintf(stderr,"Usage: %s filename num_threads\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	if ((fd = open(argv[1], O_CREAT|O_RDWR|O_TRUNC, 0666)) == -1)
	{
		fprintf(stderr,"Error opening file\n");
		exit(EXIT_FAILURE);
	}
	
	if ((file = fdopen(fd, "w+")) == NULL)
	{
		fprintf(stderr,"Error opening file\n");
		exit(EXIT_FAILURE);
	}
	
	filename = argv[1];
	num_threads = strtol(argv[2], NULL, 10);
	
	if (num_threads < 1)
	{
		fprintf(stderr,"Num_threads must be greather than 0\n");
		exit(EXIT_FAILURE);
	}
	
	memory_segments = malloc(sizeof(char *)*num_threads);
	if (memory_segments == NULL)
	{
		fprintf(stderr,"Error malloc\n");
		exit(EXIT_FAILURE);
	}
	
	for (i=0; i<num_threads; i++)
	{
		memory_segments[i] = (char *)mmap(NULL, SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
		if (memory_segments[i] == NULL)
		{
			fprintf(stderr,"Error mmap\n");
			exit(EXIT_FAILURE);
		}
	}
	
	sd1 = semget(IPC_PRIVATE, num_threads, IPC_CREAT|0666);
	if (sd1 == -1)
	{
		fprintf(stderr,"Error semget\n");
		exit(EXIT_FAILURE);
	}
	
	for (i=0; i<num_threads; i++)
	{
		ret = semctl(sd1, i, SETVAL, 1);
		if (ret == -1)
		{
			fprintf(stderr,"Error semctl\n");
			exit(EXIT_FAILURE);
		}
	}
	
	sd2 = semget(IPC_PRIVATE, num_threads, IPC_CREAT|0666);
	if (sd2 == -1)
	{
		fprintf(stderr,"Error semget\n");
		exit(EXIT_FAILURE);
	}
	
	for (i=0; i<num_threads; i++)
	{
		ret = semctl(sd2, i, SETVAL, 0);
		if (ret == -1)
		{
			fprintf(stderr,"Error semctl\n");
			exit(EXIT_FAILURE);
		}
	}
	
	if ((pid = fork()) == -1)
	{
		fprintf(stderr,"Error fork\n");
		exit(EXIT_FAILURE);
	}
	
	if (pid == 0)
	{
		signal(SIGINT, child_handler);
		
		for (i=0; i<num_threads; i++)
		{
			if (pthread_create(&tid, NULL, child_worker, (void *)i) == -1)
			{
				fprintf(stderr,"Error pthread_create\n");
				exit(EXIT_FAILURE);
			}
		}
	}
	else
	{
		signal(SIGINT, parent_handler);
		
		for (i=0; i<num_threads; i++)
		{
			if (pthread_create(&tid, NULL, parent_worker, (void *)i) == -1)
			{
				fprintf(stderr,"Error pthread_create\n");
				exit(EXIT_FAILURE);
			}
		}
	}
	
	while (1) pause();
}