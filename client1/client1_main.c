/*
 * TEMPLATE 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../sockwrap.h"
#define MAXBUF 1024
char * prog_name = "./client";

int main(int argc, char * argv[]) {

  if (argc < 4) {
    printf("Usage: ./client <Server Address> <Server Port> [Files to transfer]\n");
    return -1;
  }
  int read_result;
  int connfd;
  int port = atoi(argv[2]);
  struct sockaddr_in servaddr;

  connfd = Socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  //Filling server information 
  servaddr.sin_family = AF_INET; // IPv4 
  Inet_aton(argv[1], & servaddr.sin_addr);
  servaddr.sin_port = htons(port);

  Connect(connfd, (struct sockaddr * ) & servaddr, sizeof(servaddr));
  //printf("argc: %d argv[3]:%s\n", argc, argv[3]); 
  
  for (int i = 3; i < argc; i++) {
    
    if (sendn(connfd, "GET ", 4, MSG_NOSIGNAL) == -1) {
      break;
    }
    //printf("argv:%s strlen: %ld\n",argv[i], strlen(argv[i]));
    if (sendn(connfd, argv[i], strlen(argv[i]), MSG_NOSIGNAL) == -1) {
      break;
    }
    if (sendn(connfd, "\r\n", 2, MSG_NOSIGNAL) == -1) {
      break;
    }

    char my_buffer[MAXBUF];

    read_result = my_readline_unbuffered(connfd, my_buffer, MAXBUF);

    if (read_result != 5 && read_result != 6 && read_result != -5) { //5 è il caso +OK\r\n, 6 potrebbe essere il caso -ERR\r\n, -5 è il caso di timeout
      break;
    }
    //printf("my buffer: %s\n", my_buffer);
    else if (read_result == 5) {

      char initial_chars[6] = "0";
      memcpy(initial_chars, my_buffer, 5);
      initial_chars[6] = '\0';

      if (strcmp(initial_chars, "+OK\r\n") == 0) { //then write a file

        uint32_t size_file;

        int readn_size_file_bytes = my_readn(connfd, & size_file, 4);
        if (readn_size_file_bytes == -5) { //mi faccio ritornare -5 se la readn fa timeout             
          printf("Timeout 15 seconds passed\n");
          break;
        } else if (readn_size_file_bytes != 4 && readn_size_file_bytes != -5) {
          break;
        }
        size_file = ntohl(size_file); //network by order	       

        FILE * filefd;
        filefd = fopen(argv[i], "wb");
        if (filefd != NULL) {

          char buffer[MAXBUF];
          uint32_t n = 0, dim = 0, flag = 0, nleft = size_file;
          //int count=1;
          if (size_file < 1024)
            dim = size_file;
          else
            dim = 1024;

          while (nleft > 0) {
            //--------------------------------------------------------------------------------------------------------------------------			
            //needed for select()	
            struct timeval tv;
            fd_set cset;
            //needed for select()
            FD_ZERO( & cset);
            FD_SET(connfd, & cset);
            tv.tv_sec = 15; // 15 Secs Timeout
            tv.tv_usec = 0; // Not init'ing this can cause strange errors  

            n = 0;
            if ((n = select(FD_SETSIZE, & cset, NULL, NULL, & tv)) == -1) {
              printf("select() failed\n");
              flag = 46;
              break;
            }
            //printf("n_value: %d\n", n);
            if (n > 0) {
              //--------------------------------------------------------------------------------------------------------------------------
              n = read(connfd, buffer, dim);
              //printf("n_bytes:%d, count:%d\n", n, count);//debug
              //count++;
              //---------------------------------------------------//
              nleft = nleft - n; //really important
              if (nleft < 1024)
                dim = nleft;
              //---------------------------------------------------//			
              if (fwrite(buffer, n, 1, filefd) != 1) {
                printf("Attenzione! File:%s non scritto correttamente\n", argv[i]);
                flag = 46;
                break;
              }
            } else {
              printf("Timeout 15 seconds passed\n");
              flag = 46;
              break; /*esce dal ciclo while*/
            } //select                    
          } //end while(nleft>0)

          fclose(filefd);
          if (flag == 46)
            break;
        }
        uint32_t last_modified;

        int readn_last_modified_bytes = my_readn(connfd, & last_modified, 4);
        
        if (readn_last_modified_bytes == -5) { //mi faccio ritornare -5 se la readn fa timeout             
          printf("Timeout 15 seconds passed\n");
          break;
        } else if (readn_last_modified_bytes != 4 && readn_last_modified_bytes != -5) {
          break;
        }

        last_modified = ntohl(last_modified); //network by order

        //stampo i vari campi da stampare
        printf("Nome del file: %s\n", argv[i]); //print of the file name for clearness
        printf("Dimensione del file: %u\n", size_file);
        printf("Ultima modifica del file: %d\n", last_modified);

      } //end "+OK\r\n"	
      else break;//perchè se mi invia "pOK\r\n" sono 6 lettere non corrispondono a "+OK\r\n" e lui senza break fa ad inviare la get successiva, che non deve inviare
    } //end read_result=5
    else if (read_result == 6) {
      char initial_chars[7] = "0";
      memcpy(initial_chars, my_buffer, 6);
      initial_chars[7] = '\0';
      if (strcmp(initial_chars, "-ERR\r\n") == 0) {
        printf("Errore ritornato dal server\n");
      }
      break; //esce dal ciclo perchè se la readline legge 6 caratteri significa che ha ricevuto qualcosa di sbagliato dal server a prescindere
    } //end read_result ==6	
    else if (read_result == -5) { //mi faccio ritornare -5 se la readline fa timeout             
      printf("Timeout 15 seconds passed\n");
      break;
    }
  } // end for 		
  Close(connfd);
  return 0;
}