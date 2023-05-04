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

int fd, sd, i, ret, c;
struct sembuf oper;
void **mem;
char *segment;

void child_function()
{
	FILE *f;
	
	f = fdopen(fd, "r");
	if (f == NULL)
	{
		fprintf(stderr, "Error fdopen\n");
		exit(EXIT_FAILURE);
	}
	
	while (fscanf(f, "%s", segment) != EOF)
	{
		printf("read %s\n", segment);
		segment += strlen(segment)+1;
	}
	
	oper.sem_num = 0;
	oper.sem_op = 1;
	oper.sem_flg = 0;
	
	semop(sd, &oper, 1);
	
	while (1)
		pause();
}

void handler(int signo, siginfo_t *a, void *b)
{
	int i;
	segment;
	
	printf("Handler activated\n");
	
	for (i=1; i<c; i++)
	{
		segment = mem[i];
		while (strcmp(segment,"\0") != 0)
		{
			printf("%s\n", segment);
			segment += strlen(segment)+1;
		}
	}
}

int main(int argc, char **argv)
{
	struct sigaction act;
	sigset_t set;
	c = argc;
	
	if (argc < 2)
	{
		fprintf(stderr, "Usage: command filename1 [filename2] ...\n");
		exit(EXIT_FAILURE);
	}
	
	for (i=0; i<argc; i++)
	{
		fd = open(argv[i], O_RDONLY);
		if (fd == -1)
		{
			fprintf(stderr,"Error open\n");
			exit(EXIT_FAILURE);
		}
	}
	
	sd = semget(IPC_PRIVATE, 1, 0660);
	if (sd == -1)
	{
		fprintf(stderr, "Error semget\n");
		exit(EXIT_FAILURE);
	}
	
	semctl(sd, 0, SETVAL, 0);
	
	mem = malloc((argc)*sizeof(void *));
	if (mem == NULL)
	{
		fprintf(stderr, "Error malloc\n");
		exit(EXIT_FAILURE);
	}
	
	sigfillset(&set);
	act.sa_sigaction = handler;
	act.sa_mask = set;
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);
	
	for (i=1; i<argc; i++)
	{
		mem[i] = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);
		if (mem[i] == NULL)
		{
			fprintf(stderr, "Error mmap\n");
			exit(EXIT_FAILURE);
		}
		segment = mem[i];
		
		fd = open(argv[i], O_RDONLY);
		ret = fork();
		if (ret == -1)
		{
			fprintf(stderr, "Error fork\n");
			exit(EXIT_FAILURE);
		}
		
		if (ret == 0)
		{
			signal(SIGINT, SIG_IGN);
			child_function();
		}
	}
	
	oper.sem_num = 0;
	oper.sem_op = -(argc-1);
	oper.sem_flg = 0;
redo:
	ret = semop(sd, &oper, 1);
	if (ret == -1 && errno == EINTR)
		goto redo;
	if (ret == -1 && errno != EINTR)
	{
		fprintf(stderr, "Error semop\n");
		exit(EXIT_FAILURE);
	}
	
	printf("All childs unlocked their semaphores\n");
	
	while (1)
		pause();
	
	return 0;	
	
}