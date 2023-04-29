#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>


#define N 4
int i,j;
pthread_t threadId;/*inizializziamo la variabile thread che ci servirà poi per creare il thread vero e proprio*/
cpu_set_t cpuset; /*maschera di 64 bit */
pthread_attr_t threadAttr; /*inizializziamo valori di default per il thread*/

void *thread_function(void* arg); /*inizializziamo la funzione che daremo in pasto alla funzione di creazione del thread*/

int main(int argc,char*argv[]){
    printf("numero CPU : %lu\n ",sizeof(CPU_COUNT(&cpuset)));
    printf("INIZIO PROGRAMMA: creazione threads a brevissimo \n");
    CPU_ZERO(&cpuset);
    for(i=0;i<sizeof(CPU_COUNT(&cpuset));i++){
        printf("AOOAAOAOAOAOAOAOAOAOA\n");
        CPU_SET(i,&cpuset);
        pthread_create(&threadId,NULL,thread_function,NULL); /*indirizzo variabile pthread_t,attributi di default con NULL, istruzioni che dovrà svolgere, parametri da passare in input alla funzione*/
        wait(0);
        if(pthread_setaffinity_np(threadId,sizeof(&cpuset),&cpuset) == 0){
            printf("SUCCESSONE\n");
        };
        
    


    }

    printf("threads aspettato a dovere");

}

void *thread_function(void*arg){
    printf("Ciao sono un nuovo thread e sono appena stato creato,adesso conterò fino a 4\n");
    for(i=0;i<5;i++){
        printf("%d\n",i);
        sleep(1);
    }
}