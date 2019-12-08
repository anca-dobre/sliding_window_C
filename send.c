#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"
#include "mylib.h"

#define HOST "127.0.0.1"
#define PORT 10000

char checkSum(char *data, int lungime){
  int i;
  char sum = 0;
  for(i = 0; i < lungime; i++){
    sum = sum ^ data[i];
  }
  return sum;
}

void initMyMsg(mymsg *m, int nr, char tipPachet, char * data, int lungime){
  m->nrPachet = nr;
  m->tipPachet = tipPachet;
  m->data = calloc(lungime, 1);
  m->len = lungime;
  memcpy(m->data, data, lungime);
}

void copyToPayload(mymsg bufferElement, msg *m){
  memset(m->payload, 0, sizeof(m->payload));
  memcpy(m->payload, &bufferElement.nrPachet, 4);
  memcpy(m->payload + 4, &bufferElement.tipPachet, 1);
  memcpy(m->payload + 5, &bufferElement.len, 4);
  memcpy(m->payload + 9, bufferElement.data, bufferElement.len);
  char suma = checkSum(m->payload, 9 + bufferElement.len);
  memcpy(m->payload + 9 + bufferElement.len, &suma, 1);
  m->len = 10 + bufferElement.len;
}

int main(int argc,char** argv){

  init(HOST,PORT);
  int time = atoi(argv[3])/4;
  msg t,r;
  int bdp = (atoi(argv[2]) * atoi(argv[3]) * 1000) / (8 * sizeof(msg));
  int i,j;
  int fisier_input = open(argv[1], O_RDONLY);
      //dimensiunea fisierului
  struct stat st;
  stat(argv[1], &st);
  int file_size = st.st_size;
  int de_trimis = 0, de_primit = 0;
  int nr_mesaje = file_size/1390 + 2;
  mymsg buffer_sender[bdp];
  char buffer_citire[1390];
  
  	//trimit mesaj ce contine nume fisier + bdp
  char *nume_bdp = malloc(strlen(argv[1]) + 4);
  memcpy(nume_bdp, argv[1], strlen(argv[1]));
  memcpy(nume_bdp + strlen(argv[1]), &bdp, 4);
  initMyMsg(&buffer_sender[0], de_trimis, 'H', nume_bdp, strlen(argv[1]) + 4);
  copyToPayload(buffer_sender[0], &t);
  send_message(&t);
  de_trimis++;
  for(i = 1; i < bdp; i++){
    memset(buffer_citire, 0, 1390);
    int bytes_read = read(fisier_input, buffer_citire, 1390);
    if(bytes_read == 0){
      bdp = i;
      break;
    }
    else{
      initMyMsg(&buffer_sender[i], de_trimis, 'D', buffer_citire, bytes_read);
      copyToPayload(buffer_sender[i], &t);
      //Trimit mesajul
      send_message(&t);
      de_trimis++;
    }
  }
  for(i = 0; i < nr_mesaje - bdp; i++){
    memset(r.payload, 0, sizeof(r.payload));
    copyToPayload(buffer_sender[de_primit], &t);
    while(recv_message_timeout(&r, time) == -1){
      send_message(&t);
      memset(r.payload, 0, sizeof(r.payload));
    }
    char tip;
    memcpy(&tip, r.payload+4, 1);
    if(tip == 'N'){
      i--;
      //Retrimit mesajul
      send_message(&t);
    }
    else if(tip == 'A'){
      int nr, nr_de_asteptat;
      memcpy(&nr, r.payload, 4);
      nr_de_asteptat = buffer_sender[de_primit].nrPachet;
      for(j = nr_de_asteptat; j < nr; j++){
        memset(buffer_citire, 0, 1390);
        int bytes_read = read(fisier_input, buffer_citire, 1390);
        if(bytes_read != -1){
          free(buffer_sender[de_primit].data);
          initMyMsg(&buffer_sender[de_primit], de_trimis, 'D', buffer_citire, bytes_read);
          copyToPayload(buffer_sender[de_primit], &t);
          send_message(&t);
          de_trimis++;
        }
       
        de_primit = (de_primit + 1) % bdp;
      }
      i += nr - 1 - nr_de_asteptat;
    }
  }

  for(i = 0; i < bdp; i++){
    //Astept ultimele confirmari
    memset(r.payload, 0, sizeof(r.payload));
    copyToPayload(buffer_sender[de_primit], &t);
    while(recv_message_timeout(&r, time) == -1){
      send_message(&t);
      memset(r.payload, 0, sizeof(r.payload));
    }
    char tip;
    memcpy(&tip, r.payload+4, 1);
    if(tip == 'N'){
      i--;
      send_message(&t);
    }
    else if(tip == 'A'){
      int nr, nr_de_asteptat;
      memcpy(&nr, r.payload, 4);
      nr_de_asteptat = buffer_sender[de_primit].nrPachet;
      for(j = nr_de_asteptat; j < nr; j++){
        free(buffer_sender[de_primit].data);
        de_primit = (de_primit + 1) % bdp;
      }
      i += nr - 1 - nr_de_asteptat;
    }
  }
  close(fisier_input); 
  //nu mai am de citit
  mymsg m;
  //mesajul de terminare
  initMyMsg(&m, nr_mesaje, 'T', "", 1);
  copyToPayload(m, &t);
  send_message(&t);
  memset(r.payload, 0, sizeof(r.payload));
  while(1){
    while(recv_message_timeout(&r, time) == -1){
        send_message(&t);
        memset(r.payload, 0, sizeof(r.payload));
    }
    char tip;
    memcpy(&tip, r.payload+4, 1);
    if(tip == 'N'){
      send_message(&t);
    }
    else if(tip == 'A'){
      free(m.data);
      break;
    }
  }
  return 0;
}
