#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"
#include "mylib.h"

#define HOST "127.0.0.1"
#define PORT 10001

char checkSum(char *data, int lungime){
  int i;
  char sum = 0;
  for(i = 0; i < lungime; i++){
    sum = sum ^ data[i];
  }
  return sum;
}

void initMyMsg(mymsg *m, int nrPachet, char tipPachet, char * data, int lungime){
  m->nrPachet = nrPachet;
  m->tipPachet = tipPachet;
  m->data = calloc(lungime, 1);
  m->len = lungime;
  memcpy(m->data, data, lungime);
}

void copyMymsg(mymsg *dest, mymsg src){
  dest->tipPachet = src.tipPachet;
  dest->len = src.len;
  dest->data = calloc(dest->len, 1);
  memcpy(dest->data, src.data, dest->len);
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

void copyToMymsg(mymsg *bufferElement, msg m){
  memcpy(&bufferElement->tipPachet, m.payload + 4, 1);
  memcpy(&bufferElement->len, m.payload + 5, 4);
  bufferElement->data = calloc(bufferElement->len, 1);
  memcpy(bufferElement->data, m.payload + 9, bufferElement->len);
}

int verifSum(msg t){
  char *date_primite = malloc(t.len - 1);
  memcpy(date_primite, t.payload, t.len - 1);
  char sumaCalculata = checkSum(date_primite, t.len - 1);
  char sumaPrimita;
  memcpy(&sumaPrimita, t.payload + t.len - 1, 1);
  return (sumaPrimita == sumaCalculata);
}

int main(int argc,char** argv){
  
  msg r,t;
  init(HOST,PORT);
  
  int de_asteptat = 0;
  int bdp = 2000;
  int i;
  int am_trimis_nak = 0;
  int fisier_output;
  mymsg confirmari[2];

  initMyMsg(&confirmari[0], 0, 'A', "a", 1);
  initMyMsg(&confirmari[1], 0, 'N', "b", 1);

  mymsg *buffer_recv = malloc(bdp * sizeof(mymsg));
  int *a_ajuns = malloc(bdp * sizeof(int));

  while(1){
    memset(r.payload, 0, sizeof(r.payload));
    recv_message(&r);
    //Primeste mesaj
    if(verifSum(r) == 0){
      //Nu verifica suma
      confirmari[1].nrPachet = de_asteptat;
      copyToPayload(confirmari[1], &t);
      send_message(&t);
    }
    else {
      char tip;
      memcpy(&tip, r.payload + 4, 1);
      if(tip == 'T'){
        //avem pachetul de terminare
        close(fisier_output);
        confirmari[0].nrPachet = de_asteptat;
        copyToPayload(confirmari[0], &t);
        send_message(&t);
        break;
      }
      int nr;
      //Suma e corecta
      memcpy(&nr, r.payload, 4);
        //Se afla in intervalul ferestrei
      if(nr >= de_asteptat && nr < (de_asteptat + bdp)){
        if(nr != de_asteptat && am_trimis_nak == 0){
          //Nu e cel asteptat
          am_trimis_nak = 1;
          confirmari[1].nrPachet = de_asteptat;
          copyToPayload(confirmari[1], &t);
          send_message(&t);
        }
        if(a_ajuns[nr % bdp] == 0){
          //Nu a mai ajuns pana acum
          a_ajuns[nr % bdp] = 1;
          copyToMymsg(&buffer_recv[nr % bdp], r);
          int a_intrat = 0;
          while(a_ajuns[de_asteptat % bdp]){
            a_intrat = 1;
            //A ajuns cel asteptat
            mymsg *buffer_de_asteptat = &buffer_recv[de_asteptat % bdp];
            if(buffer_de_asteptat->tipPachet == 'H'){
              char *nume_fisier = malloc(buffer_de_asteptat->len + 5);
              strcpy(nume_fisier, "recv_");
              char *nume_fisier_primit = malloc(buffer_de_asteptat->len - 4);
              memcpy(nume_fisier_primit, buffer_de_asteptat->data, buffer_de_asteptat->len - 4);
              strcat(nume_fisier, nume_fisier_primit);
              fisier_output = open(nume_fisier, O_CREAT| O_TRUNC| O_WRONLY, 0644);
              int new_bdp;
              memcpy(&new_bdp, buffer_de_asteptat->data + buffer_de_asteptat->len - 4, 4);
              mymsg aux_buffer[bdp];
              int aux_a_ajuns[bdp];
              for(i = 0; i < bdp; i++){
                copyMymsg(&aux_buffer[i], buffer_recv[i]);
                aux_a_ajuns[i] = a_ajuns[i];
              }
              free(buffer_recv);
              free(a_ajuns);
              bdp = new_bdp;
              buffer_recv = malloc(bdp * sizeof(mymsg));
              a_ajuns = malloc(bdp * sizeof(int));
              for(i = 0; i < bdp; i++){
                copyMymsg(&buffer_recv[i], aux_buffer[i]);
                a_ajuns[i] = aux_a_ajuns[i];
              }
            }
            else{
              write(fisier_output, buffer_de_asteptat->data, buffer_de_asteptat->len);
            }
            am_trimis_nak = 0;
            a_ajuns[de_asteptat % bdp] = 0;
            de_asteptat++;
          }
          if(a_intrat){
            confirmari[0].nrPachet = de_asteptat;
            copyToPayload(confirmari[0], &t);
            send_message(&t);
          }
        }
      }
    }

  }
  //free(confirmari[0].data);
  //free(confirmari[1].data);
  return 0;
}
