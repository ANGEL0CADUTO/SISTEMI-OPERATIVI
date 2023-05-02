/*
1) apre un file F in R/W e quindi mantiene il suo descritore
2) fork-a un nuovo processo che ottiene un mesaggio dall’utente da tastiera e lo scrive sul file F
3) legge dal file F il messaggio cha ha scritto il processo figlio e lo stampa sul terminale
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

/* roba che ho fatto per open,write e read
    int id_file = open("F.txt",O_CREAT|O_RDWR|O_TRUNC);
    int size_writed = write(id_file,buffer,size_write); 
    size_writed += write(id_file,&buffer[size_writed],size_write-size_writed);
    close(id_file); 
    new_size = read(id_file, buffer_read, size);
    new_size += read(id_file, &buffer_read[new_size], size-new_size);
*/
const char name_file[] = "F.txt";
void main(){

    FILE* fid = fopen(name_file,"w+"); /*Descrittore del file*/
    char* buffer;

    pid_t pid = fork();
    
    if (pid == 0){

        printf("Sono il figlio \nInserire una stringa da scrivere su file: ");
        scanf("%m[^\n]",&buffer);
        printf("Stringa presa correttamente: %s\n",buffer);

        int size_write = strlen(buffer);
        int size_writed = fwrite(buffer,sizeof(char),size_write,fid);

        printf("Byte scritti %d\n",size_writed);

        if (size_writed == -1){
            perror("Error in write");
        } 
        
        rewrite:
        if(size_writed != size_write){ /*non ho finito di scrivere la stringa*/
            size_writed += fwrite(&buffer[size_writed],sizeof(char),size_write-size_writed,fid);
            goto rewrite;
        }

        free(buffer); /*%m[^\n] usa la malloc e alloca lo spazio in memoria*/
        fclose(fid); /*Il figlio ha la copia del descrittore file*/
        exit(size_write);
        
    } else if (pid < 0){
        perror("Error in create process");
        exit(1);
    }

    /* Opera solo il padre */
    int status; /*Status contiene il numero di byte che sono stati scritti dal figlio*/
    int aux; // Variabile ausiliaria per il calcolo veritiero
    
    if (waitpid(pid,&status,0) == -1) {
        printf("Wait has failed\n");
        exit(-1);
    } else if ((aux = WEXITSTATUS(status)) == -1) {
        printf("Child process terminated with error\n");
        exit(-1);
    }

    printf("Il figlio ha terminato con stato %d, aux %d\n",status, aux);

    char buffer_read[aux + 1]; // +1 per \0

/* NON MI SERVE, MA UTILE RICORDARE 
    struct stat st;
    stat(name_file, &st);
    int size = st.st_size; /*Al posto di size potrei usare ftell(fid), dato che corrisponde a dove sono arrivata ora
*/

/* TUTORAGGIO
    int aux;
    int status;
    
    if (wait(&status) == -1) {
        printf("Wait has failed\n");
        exit(-1);
    } else if ((aux = WEXITSTATUS(status)) == -1) {
        printf("Child process terminated with error\n");
        exit(-1);
    }

    len = (size_t) aux;

    char text[len + 1];
*/
    
    int pointer_file_not_update = ftell(fid); //
    fseek(fid,0,SEEK_SET);
    int pointer_file_update = ftell(fid);
    printf("Devo riposizionare il pointer del file, prima: %d poi: %d\n",pointer_file_not_update,pointer_file_update);

    int new_size = fread(buffer_read,sizeof(char),aux,fid);
    
    if (new_size == -1){
        perror("Error Read");
    }
    printf("Ho letto %d char mancano %d\n",new_size, aux-new_size);

    resize:
    if (new_size != aux){
        new_size += fread(&buffer_read[new_size],sizeof(char),aux-new_size,fid);
        goto resize;
    } else 

    buffer_read[aux + 1] = '\0';
    printf("File letto: %s\n",buffer_read);

    fclose(fid);
    exit(0);
    
}