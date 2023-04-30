/*Scrivere un programma per Windows/Unix che
una volta mandato in esecuzione sia in grado di
creare ed attendere la terminazione di N threads
in sequenza
(create(T0) → join(T0) → create(T1) → join(T1) → ...)
in maniera tale che ogni thread Ti sia affine alla
CPU-core (i % #CPU).*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>

#define NUM_CPU 6 /*Numero di cpu del pc, meglio controllare con funzione apposita*/

void child_thread(); /*Funzione di vita del thread figlio*/

void main(){

    /*INIZIALIZZAZIONE*/
    int current_child = 0; /*Contatore del numero di figli effettuato*/
    void* status; /*Stato di ritorno del figlio che ha terminato*/
    int stack_size = 8388608;

    pthread_t thread_ids[NUM_CPU];  /*Array non allocato degli ids dei thread creati*/
    cpu_set_t cpuset; /*bitmask affinità cpu*/

    pthread_attr_t attr; /*Creo una variabile attributi*/

    printf("Sono il thread padre, inizio a creare thread\n");

    while(current_child < NUM_CPU){ 

        CPU_ZERO(&cpuset); /*Resetto la bitmask*/
        CPU_SET(current_child&NUM_CPU, &cpuset); /*Imposto a 1 la cpu affinity*/

        /*
		 * Inizializza una struttura di attributi per la creazione di un
		 * thread, che passiamo alla funzione "pthread_create" per impostare
		 * alcuni parametri dello stato dell'esecuzione del thread.
		 */
        if (pthread_attr_init(&attr)) {
			printf("Error. Unable to initialize thread attributes.\n");
			exit(1);
		}
		if (pthread_attr_setstacksize(&attr, stack_size)) {
			printf("Warning. Unable to set thread stack size.\n");
		}

        if(pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset) != 0){
           perror("Error in Pthread Affinity"); 
        }

        /*Fine settaggio struttura attributi*/

        /*Creo Thread*/
        if(pthread_create(&thread_ids[current_child],&attr, (void*)child_thread, NULL) != 0){
            perror("Error in Pthread Create");
        }

        printf("Il figlio %ld è stato creato, mi metto in attesa della sua terminaizone\n",thread_ids[current_child]);

        /*
		 * Quando non è più richiesta, e per un fututo riutilizzo, la
		 * struttura di attributi per la creazione del thread deve
		 * essere distrutta come segue. Questo non ha effetto sui
		 * threads che sono stati creati passandogli tale struttura.
		 */
		if (pthread_attr_destroy(&attr)) {
			printf("Warning. Unable to destroy thread attributes.\n");
		}
        
        /*Attendo la terminazione del thread creato*/
        if(pthread_join(thread_ids[current_child],&status) != 0){
            perror("Error in Pthread Join");
        }

        printf("il figlio numero %d ha terminato\n",current_child);
        current_child++;
    }

    printf("Il thread padre ha terminato\n");
    exit(0);
}

void child_thread(){
    printf("Sono il thread figlio %ld, passo e chiudo\n",pthread_self());
    int k = 0;
    while(k < 1000000000){ // Cose inutili per far sprecare memoria
        k += 1;
    }
    pthread_exit(0);
}