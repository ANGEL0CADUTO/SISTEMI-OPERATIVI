#define MAX_SIZE 1024

union semun sem_arg;
struct sembuf buf[1];
int sem_child;

void wait_sem(int sem, int num)
{
	buf[0].sem_num = num;
	buf[0].sem_op = -1;
	buf[0].sem_flg = 0;
	
	while (semop(sem, buf, 1) == -1) 
	{
		if (errno != EINTR)
		{
			perror("Error semop");
			exit(EXIT_FAILURE);
		}
	}
}

void signal_sem(int sem, int num)
{
	buf[0].sem_num = num;
	buf[0].sem_op = 1;
	buf[0].sem_flg = 0;
	
	while (semop(sem, buf, 1) == -1) 
	{
		if (errno != EINTR)
		{
			perror("Error semop");
			exit(EXIT_FAILURE);
		}
	}
}

void handler_sigint(int signum)
{
	fputs("Signal SIGINT captured\n", stdout);
	fflush(stdout);
}

void handler(char *file)
{
	int fsize;
	char *new_filename;
	FILE *old, *new;
	
	old = fopen(file, "r");
	if (old == NULL)
	{
		perror("Error fopen");
		exit(EXIT_FAILURE);
	}
	
	fseek(old, 0, SEEK_END);
	fsize = ftell(old);
	rewind(old);
	
	all_file = (char *)malloc(sizeof(char) * fsize);
	if (fread(all_file, 1, fsize, old) < 0)
	{
		perror("Error fread");
		exit(EXIT_FAILURE);
	}
	
	fclose(old);
	
	new_filename = (char *)malloc(sizeof(char) * (strlen(file) + 7));
	
	sprintf(new_filename, "backup_%s", file);
	
	new = fopen(new_filename, "w+");
	if (new == NULL)
	{
		perror("Error fopen");
		exit(EXIT_FAILURE);
	}
	
	if (fwrite(all_file, 1, fsize, new) < 0)
	{
		perror("Error fwrite");
		exit(EXIT_FAILURE);
	}
	
	fclose(new);
}

void work_child(int me, char *my_file, char *my_string)
{
	FILE *f;
	
	f = fopen(my_file, "a")
	if (f == NULL)
	{
		perror("Error fopen");
		exit(EXIT_FAILURE);
	}
	
	signal(SIGINT, handler_sigint);
	
	while (1)
	{
		buf[0].sem_num = me;
		buf[0].sem_op = -1;
		buf[0].sem_flg = 0;
		
		while (semop(sem_child, buf, 1) == -1) 
		{
			if (errno != EINTR)
			{
				perror("Error semop");
				exit(EXIT_FAILURE);
			}
			else
				handler(my_file);
		}
		
		// prendi nuova stringa
		
		if (fwrite(new_string, 1, strlen(new_string), f))
		{
			perror("Error fwrite");
			exit(EXIT_FAILURE);
		}
	}
}

void main(int argc, char *argv[])
{
	int i = 1, num1, num2, j, k, mode;
	
	//./a.out -f file1 ... filen -s str1 ... strn
	if ((argc % 2) == 0)
	{
		printf("Usage: <program name> <...>\n");
		exit(EXIT_FAILURE);
	}
	
	num1 = (argc-1)/2;
	num2 = num1-2;
	
	// creazione semaforo
	if (sem_child = semget(0, num2, IPC_CREAT|0666) < 0)
	{
		perror("Error semget");
		semctl(sem_child, -1, IPC_RMID);
		exit(EXIT_FAILURE);
	}
	
	// inizializzazine semaforo
	for (j = 0; j<num2; j++)
	{
		sem_arg.val = 0;
		if (semctl(sem_child, j, SETVAL, sem_arg) == -1)
		{
			perror("Error semctl");
			semctl(sem_write, -1, IPC_RMID);
			exit(EXIT_FAILURE);
		}
	}
	
	pid_t pids[num2];
	thread_t tids[num2];
	
	char **files = (char **)malloc(sizeof(char *)*num);
	char **strings = (char **)malloc(sizeof(char *)*num);
	
	if (files == NULL)
	{
		perror("Error malloc");
		exit(EXIT_FAILURE);
	}
	
	for (j = 0; j<num1; j++)
	{
		files[j] = (char *)malloc(sizeof(char)*MAX_SIZE);
		strings[j] = (char *)malloc(sizeof(char)*MAX_SIZE);
		
		if (files[j] == NULL)
		{
			perror("Error malloc");
			exit(EXIT_FAILURE);
		}
		
		if (strings[j] == NULL)
		{
			perror("Error malloc");
			exit(EXIT_FAILURE);
		}
	}
	
	while (i < argc)
	{
		if (strncmp(argv[i], "-f", 2) == 0)
		{
			mode = 0;
		}
		
		if (strncmp(argv[i], "-s", 2) == 0)
		{
			mode = 1;
		}
		
		if (mode == 0)
		{
			strncpy(files[i], argv[i], strlen(argv[i]);
		}
		else
		{
			strncpy(strings[i], argv[i], strlen(argv[i]);
		}
		
		i++;
	}
	
	for (i = 0; i<num2; i++)
	{
		if ((pids[i] = fork()) < 0)
		{
			perror("Error fork");
			exit(EXIT_FAILURE);
		}
		
		else if (pids[i] == 0)
		{
			// figlio
			work_child(i, files[i], strings[i]);
		}
		
		else
		{	
			// padre
			while (scanf("%s", new_string) < 0)
			{
				if (errno != EINTR)
				{
					perror("Error scanf");
					exit(EXIT_FAILURE);
				}
			}
			
			passed = 0;
			
			for (j = 0; j<num2; j++)
			{
				if (strncmp(argv[num2+3+i], new_string) == 0)
				{
					// passare tramite memoria condivisa
					signal_sem(sem_child, j);
					passed = 1;
				}
			}
			
			if (passed = 0)
			{
				for (j = 0; j<num2; j++)
				{
					// passare tramite memoria condivisa
					signal_sem(sem_child, j);
				}
			}
		}
	}
}