#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>



int i;
pthread_t threadId1;/*inizializziamo la variabile che identificherà il nostro thread */
pthread_t threadId2;
pthread_t threadId3;
pthread_t threadId4;
pthread_t * array[] = {&threadId1,&threadId2,&threadId3,&threadId4}; //ci servirà per farci i cicli for sopra
cpu_set_t cpuset; /*maschera di 64 bit */
pthread_attr_t threadAttr; /* piu tardi voglio capire come usare USARE PTHREAD_ATTR_SETAFFINITY*/


void *thread_function(void* arg); /*definiamo la funzione che i threads eseguiranno*/


int main(int argc,char*argv[]){

    printf("numero CPU : %lu\n",sizeof(CPU_COUNT(&cpuset)));
    printf("INIZIO PROGRAMMA: \n");

    for(i=0;i<sizeof(CPU_COUNT(&cpuset));i++){

        CPU_ZERO(&cpuset); /*azzero la bitmap (sennò il N-esimo thread prenderà quella dell'N-1esimo che rimarrebbe residua)*/
        CPU_SET(i,&cpuset); //aggiungo la cpu i-esima al set

        pthread_create(array[i],NULL,thread_function,NULL); /*indirizzo variabile pthread_t,NULL = attributi di default, istruzioni che svolgerà, parametri da passare in input alla funzione*/
        
        if(pthread_setaffinity_np(*array[i],sizeof(cpuset),&cpuset) == 0){
            printf("affinità attribuita con successo\n");
        }
        
        if(pthread_join(*array[i],NULL) == 0){ //serve per attendere il l'esecuzione del thread
            printf("join effettuata con successo\n");
        }
        
        //vediamo quale cpu è affine
        for(int k = 0; k < 4; k++){ 
            if(CPU_ISSET(k,&cpuset)){
                printf("Il thread numero %d è affine alla CPU %d\n",i,k);
            }
        }

        

    }


    printf("Sembra tutto funzionante :  si ringraziano Andrea ed Alessia per il loro aiuto\n");

}

void *thread_function(void*arg){

    printf("Ciao sono un nuovo thread e sono appena stato creato,adesso conterò fino a 4\n");
    for(int j=0;j<5;j++){
        printf("%d\n",j);
        sleep(1);
    }

}