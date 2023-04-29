#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>



int i;
pthread_t threadId1;/*inizializziamo la variabile thread che ci servirà poi per creare il thread vero e proprio*/
pthread_t threadId2;
pthread_t threadId3;
pthread_t threadId4;
pthread_t * array[] = {&threadId1,&threadId2,&threadId3,&threadId4};
cpu_set_t cpuset; /*maschera di 64 bit */
pthread_attr_t threadAttr; /* piu tardi voglio capire come usare USARE PTHREAD_ATTR_SETAFFINITY*/

void *thread_function(void* arg); /*inizializziamo la funzione che daremo in pasto alla funzione di creazione del thread*/

int main(int argc,char*argv[]){
    printf("numero CPU : %lu\n ",sizeof(CPU_COUNT(&cpuset)));
    printf("INIZIO PROGRAMMA: creazione threads a brevissimo \n");
    CPU_ZERO(&cpuset); /*azzero la bitmask*/
    for(i=0;i<4;i++){

        printf("%lu\n",*array[i]); /*Si lo so è sbagliato, ho fatto così per pescarmi la variabile pthread threadIdN dove N=1,2,3,4 per poter settare
        la sua affinità e fare la threadjoin*/
        
        CPU_SET(i,&cpuset); /*setto il bit della cpu con cui fare affinità*/
        pthread_create(array[i],NULL,thread_function,NULL); /*indirizzo variabile pthread_t,attributi di default con NULL, istruzioni che dovrà svolgere, parametri da passare in input alla funzione*/
        
        
        pthread_join(threadId1,NULL);/*il ciclo for finisce e lui è il primo sospettato*/

        /*Un tempo qui si faceva affinità di cpu, poi però arrivò più di 1 thread :c
        
        if(pthread_setaffinity_np(threadId,sizeof(&cpuset),&cpuset) == 0){
            printf("SUCCESSONE\n");
        };
        
        */
        

    }


    printf("CICLO FOR BELLO CHE FINITO");

}

void *thread_function(void*arg){
    printf("Ciao sono un nuovo thread e sono appena stato creato,adesso conterò fino a 4\n");
    for(i=0;i<5;i++){
        printf("%d\n",i);
        sleep(1);
    }

}