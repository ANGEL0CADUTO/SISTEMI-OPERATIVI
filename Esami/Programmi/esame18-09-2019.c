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

int num_threads;
char **buffers;
pthread_mutex_t *ready;
pthread_mutex_t *done;
FILE *file;
char **strings;

void printer()
{
	system("cat output.txt");
}

void *the_thread(void *param)
{
	sigset_t set;
	long int me = (long int)param;
	int i, j;
	
	if (me < num_threads-1)
	{
		printf("Thread %d started up - in charge of string %s\n", me, strings[me]);
	}
	else
	{
		printf("Thread %d started up - in charge of output file\n", me);
	}
	fflush(stdout);
	
	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, NULL);
	
	while (1)
	{
		if (pthread_mutex_lock(ready+me))
		{
			fprintf(stderr, "Error lock\n");
			exit(EXIT_FAILURE);
		}
		
		printf("%d - got string %s\n", me, buffers[me]);
		
		if (me == num_threads-1)
		{
			printf("Writing string %s to output file\n", buffers[me]);
			fprintf(file, "%s\n", buffers[me]);
			fflush(file);
		}
		else
		{
			for (i=0; i<strlen(strings[me]); i++)
			{
				for (j=0; j<strlen(buffers[me]); j++)
				{
					if (*(buffers[me]+j) == *(strings[me]+i))
						*(buffers[me]+j) = ' ';
				}
			}
			
			if (pthread_mutex_lock(done+me+1))
			{
				fprintf(stderr, "Error lock\n");
				exit(EXIT_FAILURE);
			}
			
			buffers[me+1] = buffers[me];
			if (pthread_mutex_unlock(ready+me+1))
			{
				fprintf(stderr, "Error unlock\n");
				exit(EXIT_FAILURE);
			}
		}
		
		if (pthread_mutex_unlock(done+me))
		{
			fprintf(stderr, "Error unlock\n");
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char **argv)
{
	int ret;
	long int i;
	pthread_t tid;
	char *p;
	
	if (argc < 2)
	{
		fprintf(stderr, "Incorrect number of arguments\n");
		exit(EXIT_FAILURE);
	}
	
	file = fopen("output.txt", "w+");
	if (file == NULL)
	{
		fprintf(stderr, "Error fopen\n");
		exit(EXIT_FAILURE);
	}
	
	strings = argv+1;
	num_threads = argc;
	
	buffers == (char **)malloc(sizeof(char *)*num_threads);
	if (buffers == NULL)
	{
		fprintf(stderr, "Error malloc\n");
		exit(EXIT_FAILURE);
	}
	
	ready = malloc(num_threads * sizeof(pthread_mutex_t));
	done = malloc(num_threads * sizeof(pthread_mutex_t));
	if (ready == NULL || done == NULL)
	{
		fprintf(stderr, "Error malloc\n");
		exit(EXIT_FAILURE);
	}
	
	for (i=0; i<num_threads; i++)
	{
		if (pthread_mutex_init(ready+i,NULL) || pthread_mutex_init(done+i,NULL) || pthread_mutex_lock(ready+i))
		{
			fprintf(stderr, "Error init\n");
			exit(EXIT_FAILURE);
		}
	}
	
	for (i=0; i<num_threads; i++)
	{
		ret = pthread_create(&tid, NULL, the_thread, (void *)i);
		if (ret != 0)
		{
			fprintf(stderr, "Error pthread_create\n");
			exit(EXIT_FAILURE);
		}
	}
	
	signal(SIGINT, printer);
	
	while (1)
	{
read_again:
		ret = scanf("%ms", &p);
		if (ret == EOF && errno == EINTR)
			goto read_again;
		printf("Read string (area is at address %p): %s\n", p, p);

redo_1:
		if (pthread_mutex_lock(done))
		{
			if (errno == EINTR)
				goto redo_1;
			fprintf(stderr, "Main thread - mutex lock attempt error\n");
			exit(EXIT_FAILURE);
		}
		buffers[0] = p;
redo_2:
		if (pthread_mutex_unlock(ready))
		{
			if (errno == EINTR)
				goto redo_2;
			fprintf(stderr, "Main thread - mutex unlock attempt error\n");
			exit(EXIT_FAILURE);
		}
	}
	
	return 0;
}