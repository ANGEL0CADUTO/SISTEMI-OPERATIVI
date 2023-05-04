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
FILE *output_file;
FILE **source_files;
char **files;

void printer()
{
	int i;
	char *p;
	int ret;
	
	for (i=0; i<num_threads; i++)
	{
		source_files[i] = fopen(files[i], "r");
		if (source_files[i] == NULL)
		{
			fprintf(stderr, "Error fopen\n");
			exit(EXIT_FAILURE);
		}
	}
	
	output_file = fopen("output_file", "w+");
	if (output_file == NULL)
	{
		fprintf(stderr, "Error fopen");
		exit(EXIT_FAILURE);
	}
	
	i = 0;
	while (1)
	{
		ret = fscanf(source_files[i], "%ms", &p);
		if (ret == EOF)
			break;
		printf("%s\n", p);
		fflush(stdout);
		fprintf(output_file, "%s", p);
		fflush(output_file);
		free(p);
		i = (i+1) % num_threads;
	}
}

void *the_thread(void *param)
{
	sigset_t set;
	long int me = (long int)param;
	int i, j;
	FILE *target_file;
	
	printf("Thread %d started up - in charge of %s\n", me, files[me]);
	fflush(stdout);
	
	target_file = fopen(files[me], "w+");
	if (target_file == NULL)
	{
		fprintf(stderr, "Error fopen\n");
		exit(EXIT_FAILURE);
	}
	
	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, NULL);
	
	while (1)
	{
		if (pthread_mutex_lock(ready+me))
		{
			fprintf(stderr,"Error mutex attempt\n");
			exit(EXIT_FAILURE);
		}
		
		printf("Thread %d - got string %s\n", me, buffers[me]);
		fprintf(target_file, "%s\n", buffers[me]);
		fflush(target_file);
		
		if (pthread_mutex_unlock(done+me))
		{
			fprintf(stderr,"Error mutex attemp\n");
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
		fprintf(stderr,"Incorrect number of arguments\n");
		exit(EXIT_FAILURE);
	}
	
	files = argv+1;
	num_threads = argc-1;
	
	buffers = (char **)malloc(sizeof(char *)*num_threads);
	if (buffers == NULL)
	{
		fprintf(stderr, "Error malloc\n");
		exit(EXIT_FAILURE);
	}
	
	source_files = (FILE **)malloc(sizeof(FILE *)*num_threads);
	if (source_files == NULL)
	{
		fprintf(stderr, "Error malloc\n");
		exit(EXIT_FAILURE);
	}
	
	ready = malloc(num_threads*sizeof(pthread_mutex_t));
	done = malloc(num_threads*sizeof(pthread_mutex_t));
	
	if (ready == NULL || done == NULL)
	{
		fprintf(stderr, "Error mutex allocation\n");
		exit(EXIT_FAILURE);
	}
	
	for (i=0; i<num_threads; i++)
	{
		if (pthread_mutex_init(ready+i, NULL) || pthread_mutex_init(done+i, NULL) || pthread_mutex_lock(ready+i))
		{
			fprintf(stderr, "Error mutex allocation\n");
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
	
	i = 0;
	while (1)
	{
read_again:		
		ret = scanf("%ms", &p);
		if (ret == EOF && errno == EINTR)
			goto read_again;
		printf("Read string (area is at address %p): %s\n", p, p);
redo1:
		if (pthread_mutex_lock(done+i))
		{
			if (errno == EINTR)
				goto redo1;
			fprintf(stderr, "Main thread - mutex lock attempt error\n");
			exit(EXIT_FAILURE);
		}
		buffers[i] = p;
redo2:
		if (pthread_mutex_unlock(ready+i))
		{
			if (errno == EINTR)
				goto redo2;
			fprintf(stderr, "Main thread - mutex unlock attemp error\n");
			exit(EXIT_FAILURE);
		}
		
		i = (i+1) % num_threads;
	}
	
	return 0;
}