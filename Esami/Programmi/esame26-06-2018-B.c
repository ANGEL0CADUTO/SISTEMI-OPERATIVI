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

#define PAGE_SIZE 4096
#define SEM_KEY 5000

int num_proc;
int *values;
int sem_ds;
char command[1024];
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
redo_1:
	if (semop(sem_ds, buf, 1) == -1)
	{
		if (errno != EINTR)
		{
			fprintf(stderr, "Error wait_sem\n");
			semctl(sem_ds, -1, IPC_RMID, NULL);
			exit(EXIT_FAILURE);
		}
		else
			goto redo_1;
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
		if (errno != EINTR)
		{
			fprintf(stderr, "Error signal_sem\n");
			semctl(sem_ds, -1, IPC_RMID, NULL);
			exit(EXIT_FAILURE);
		}
		else
			goto redo_2;
	}
}

void run(void)
{
	int val, ret;
	struct sembuf oper;
	
	signal(SIGINT, SIG_IGN);
	
	while (1)
	{
		while (scanf("%d", &val) == -1)
		{
			if (errno != EINTR)
			{
				fprintf(stderr, "Error scanf\n");
				exit(EXIT_FAILURE);
			}
		}
		
		printf("Process %d - read value %d\n", getpid(), val);
		fflush(stdout);
		
		wait_sem(1);
		
		printf("Process %d - writing value %d\n", getpid(), val);
		fflush(stdout);
		*values = val;
		
		signal_sem(0);
		
	}
}

void sigint_handler(void)
{
	system(command);
}

int main(int argc, char **argv)
{
	int i, ret, fd;
	FILE *file;
	
	if (argc != 3)
	{
		fprintf(stderr, "Usage: <prog> <output_file> <num_proc>\n");
		exit(EXIT_FAILURE);
	}
	if (argv[2] < 1)
	{
		fprintf(stderr, "num_proc must be >= 1!\n");
		exit(EXIT_FAILURE);
	}
	
	num_proc = strtol(argv[2], NULL, 10);
	values = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);
	if (values == NULL)
	{
		fprintf(stderr, "Error mmap\n");
		exit(EXIT_FAILURE);
	}
	
	if ((sem_ds = semget(SEM_KEY, 2, IPC_CREAT|0666)) == -1)
	{
		fprintf(stderr, "Error semget\n");
		semctl(sem_ds, -1, IPC_RMID, sem_arg);
		exit(EXIT_FAILURE);
	}
	
	if (semctl(sem_ds, 0, SETVAL, 1) == -1)
	{
		fprintf(stderr, "Error semctl\n");
		semctl(sem_ds, -1, IPC_RMID, sem_arg);
		exit(EXIT_FAILURE);
	}
	
	if (semctl(sem_ds, 1, SETVAL, 0) == -1)
	{
		fprintf(stderr, "Error semctl\n");
		semctl(sem_ds, -1, IPC_RMID, sem_arg);
		exit(EXIT_FAILURE);
	}
	
	sprintf(command, "cat %s", argv[1]);
	signal(SIGINT, sigint_handler);
	printf("Spawning %d processes\n", num_proc);
	fflush(stdout);
	
	for (i=0; i<num_proc; i++)
	{
		if ((ret =fork()) == -1)
		{
			fprintf(stderr, "Error fork\n");
			exit(EXIT_FAILURE);
		}
		else if (ret == 0)
		{
			run();
		}
		else
			continue;
	}
	
	fd = open(argv[1], O_CREAT|O_RDWR, 0666);
	if (fd == -1)
	{
		fprintf(stderr, "Error open\n");
		exit(EXIT_FAILURE);
	}
	file = fdopen(fd, "r+");
	if (file == NULL)
	{
		fprintf(stderr, "Error fdopen\n");
		exit(EXIT_FAILURE);
	}
	
	while (1)
	{
		wait_sem(0);
		
		printf("Found value is %d\n", values);
		fflush(stdout);
rewrite:
		ret = fprintf(file, "%d ", *values);
		if (ret == -1 && errno == EINTR)
			goto rewrite;
		fflush(file);
		
		signal_sem(1);
	}
	
	pause();
	
	exit(EXIT_SUCCESS);
}