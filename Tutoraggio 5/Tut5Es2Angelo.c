/*Scrivere un prgramma che esegue i seguenti passi rispettando l'ordine
1)Apre file F in R/W e quindi mantiene il suo descrittore
2)fork-a un nuovo processo che ottiene un messaggio dall'utente da tastiera e lo scrive sul file F
3)Legge dal file F il messaggio che ha scritto il processo figlio e lo stampa sul terminale
*/




/*COSE DA FARE/RISOLVERE : 
1)mettere per bene tutti gli if per gestire gli errori
2)prima di staccare stavo risolvendo il problema che rimanevano dei residui(ultima lettera/parola della frase), credo di aver risolto facendo fclose(fd)
alla fine, ma rimane un mistero perchè a volte stampa caratteri a cazzo di cane alla fine della frase che stampa il padre mentre nel file pippo la frase è scritta bene
3)non si possono lasciare spazi alla fine della frase sennò stampa i suddetti caratteri a cazzo di cane
*/



#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

void main(int argc, char* argv[]){

    //VARIABILI PER FARE IL FORK
    pid_t pid; //variabile che identifica il processo figlio
    int status;//ancora non ho ben capito come il padre aspetta esattamente il figlio dandogli semplicemente la maniglia di un intero a caso(?)

    //VARIABILI PER MANIPOLARE FILE
    int BUFFERSIZE = 1024; //per comodità
    char buffer[BUFFERSIZE]; //x scriverci testo da mettere nel file (Ci sono altri modi più efficienti??)  
    FILE * fd; //puntatore a uno stream associato al file
    static const char *nomeFile = "pippo"; /*da capire una cose:  
    1)il path intero non serve perchè lo cerca nella directory corrente?
    2)come avrei fatto con il path intero /home....*/
    char bufferino[BUFFERSIZE];
    long unsigned int len;



    //APERTURA FILE
    fd = fopen(nomeFile, "w+");//PATHNAME, MODE(r+ e w+ sono per aprirlo in modalita sia read che write)

    if(fd == NULL){
        printf("Errore nell'apertura del file \n");
    }


    //FORK 
    pid = fork();


    //GESTIONE ERRORE FORK
    if(pid<0){
        printf("Errore nella creazione del processo figlio\n");
    }
    

    //PROCESSO FIGLIO
    if(pid ==0){
        printf("Ciao sono il processo figlio, scrivere un messaggio che vuoi scrivere sul file F:\n");
        fgets(buffer,BUFFERSIZE,stdin);
        len = strlen(buffer);
    
        if(fwrite(buffer,len,1,fd) != -1){//scriviamo su file passandogli: messsaggio da scrivere, grandezza in byte di ogni elemento, numero elementi, stream associato al file 
        /*NOTA: il figlio condivide la tabella dei descrittori del padre, per questo motivo è in grado di scrivere su un file il PADRE ha aperto*/
            printf("Frase scritta con successo nel file\n");
        }; 
        
        exit(len); //*il figlio lancia la palla al padre*
    }

    //PROCESSO PADRE
    else{
        long unsigned int lenp;
        if(waitpid(pid,&status,0) !=1){ //attendiamo il processo figlio
            printf("attesa del processo figlio fatta con successo");
        };

        lenp = WEXITSTATUS(status); //bel lancio figliolo

        printf("Sono il processo padre, adesso tocca a me\n");

        //dobbiamo spostare che il figlio ha spostato
        fseek(fd,0,SEEK_SET);//spostiamo puntatore passandogli: stream associato al file, a partire dall'inizio, il modo in cui farlo
        fread(bufferino,lenp-1,1,fd); //lenp-1 perchè l'ultimo carattere è strano e il terminale fa scherzi
        printf("La stringa scritta è : %s\n",bufferino);


        


        
    }

    fclose(fd);
}