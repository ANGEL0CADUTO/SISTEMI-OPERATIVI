// utilizzo: 
// sudo apt install nmon ## Debain/ubuntu
// comando su shell: nmon 
// premete c e vedrete utilizzo do ogni cpu (non è il metodo del proff ma non lo trovavamo)
// poi aprite su un altra shell questo codice
// partirà subito il primo thread sul primo processore e vedrete da nomn la cpu1 salire
// mandate una lettera in input, inizierà il prossimo thread e vedrete cpu 2 salire, e ripetete.

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <execinfo.h>
#include <pthread.h>
#include <sched.h>

#define N 9 //Scegliete voi il numero di thread
#define numOfCpu 8 //Verificate quante sono le vostre e aggiornate
char buff[1]; 
int stack_size = 8388608; //boh perchè il tutor fa cosi


void thread_f(void* args);

void main (int argc,char* argv[]){
    void *stat;
    int arg;
    pthread_t tid;
    pthread_attr_t attr;
    cpu_set_t cpuset;

    if (pthread_attr_init(&attr)) {
        printf("Error. Unable to initialize thread attributes.\n");
        exit(1);
    }
    if (pthread_attr_setstacksize(&attr, stack_size)) {
        printf("Warning. Unable to set thread stack size.\n");
    }

    int i=0;
    while(i<N){
        printf("Azzero bit mask\n"); 
        CPU_ZERO(&cpuset);
        printf("Imposto bit mask per il thread %d\n",i);
        CPU_SET(i%numOfCpu, &cpuset);

        printf("Creo attributo con affinità di processore\n");

        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset); 

        printf("Creo Thread %d\n",i+1); //+1 per estetica
        arg = i;
        if (pthread_create(&tid,&attr,(void*) thread_f,(void*)&arg)!=0){ //CREATE
            perror("Error in pthread_create");
        }
        printf("Attendo thread %d\n",i);
        if (pthread_join(tid,&stat) !=0) { //JOIN
            perror("Error in pthread_join");
        }
        i++;
    }
    printf("MAIN thread: enter c to close\n");
    scanf("%s",buff); //unica cosa da migliorare, ma la fgets non mi funzionava
    exit(0);
}

void thread_f(void* arg){

    int* j = (int*)arg;
    printf("I'm the thread number: %d , enter c to close me\n",*j+1); //+1 per estetica
    int k=0;
    while (k<1000000000){ //while inutile per far girare la cpu giusto 2 secondi (con il mio pc)
        k++; 
    }
    scanf("%s",buff); //idem prima

    pthread_exit(0);
}