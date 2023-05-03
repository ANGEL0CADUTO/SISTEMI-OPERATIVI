/*Scrivere un programma che sia in grado di invertire l’ordine dei caratteri in un file.
Primo carattere in ultima posizione, ultimo carattere in prima posizione, secondo carattere in
penultima posizione, penultimo carattere in seconda posizione, e così via.
Senza utilizzare un file aggiuntivo e senza utilizzare un buffer per accomodare l’intero file in memoria.*/

#include <stdio.h>
#include <stdlib.h>

const char file_name[] = "F.txt";

void main(){
    
    FILE* fid = fopen(file_name,"r+"); /*Ottengo il descrittore file*/
    char aux1, aux2;

    fseek(fid,0,SEEK_END); /*Offset 0 dalla fine*/
    int num_of_cicles = ftell(fid)/2; /*Ottengo il size totale del file*/

    printf("Il file contiene %d byte, devo fare %d giri\n",num_of_cicles*2,num_of_cicles);

    // Caso pari, vanno inverite tutte le lettere
    // Caso dispari, la lettera centrale va ignorata

    int curr = 0;

    while(curr < num_of_cicles){

        fseek(fid,(-curr-1),SEEK_END); /* Inizio dalla coda, tolgo -1 per l'EOF*/
        fread(&aux1,sizeof(char),1,fid); /* Leggo l'ultimo elemento e lo salvo in aux1 */

        printf("Ho letto %c\n",aux1);

        fseek(fid,curr,SEEK_SET); /*Posizionata all'inizio*/
        fread(&aux2,sizeof(char),1,fid); /* Leggo il primo elemento e lo salvo in aux2 */

        // Sono in seconda posizione, lo sfrutto o meno?
        printf("Ho letto %c\n",aux2);

        fseek(fid,-1,SEEK_CUR); /*Posizionata all'inizio*/
        fwrite(&aux1,sizeof(char),1,fid); /*scritto l'ultimo sul primo*/

        fseek(fid,-curr-1,SEEK_END); /*Posizionata alla fine*/
        fwrite(&aux2,sizeof(char),1,fid);

        curr++;
    }
    exit(0);

}