/*
 * TEMPLATE 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../sockwrap.h"
#define MAXBUF 1024
char * prog_name = "./server";

int main(int argc, char * argv[]) {

  if (argc != 2) {
    printf("Usage: ./server <Port>\n");
    return -1;
  }
  
  uint16_t port = atoi(argv[1]);
  struct sockaddr_in servaddr, cliaddr;
  socklen_t addrlen;
  int sockfd, connfd;
  char my_buffer[MAXBUF];

  sockfd = Socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  servaddr.sin_family = AF_INET; // IPv4 
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(port);

  Bind(sockfd, (struct sockaddr * ) & servaddr, sizeof(servaddr));

  Listen(sockfd, 15);

  while (1) { //read to handle connection (accept)
    addrlen = sizeof(cliaddr);
    connfd = accept(sockfd, (struct sockaddr * ) & cliaddr, & addrlen);
    //printf("connection accepted\n");

    while (1) { //read until there are gets from client in a specified connection

      int read_result = my_readline_unbuffered(connfd, my_buffer, MAXBUF);
      //printf("result: %d\n", read_result);
      if (read_result == 0) break;
      else if (read_result > 0) {

        //printf("Ricevuto:%s\n", my_buffer);

        char initial_chars[5] = "0";
        memcpy(initial_chars, my_buffer, 4);
        initial_chars[5] = '\0'; //metto il carattere terminatorio per poter confrontare dopo con la strcmp
        //printf("caratteri iniziali:%s\n", initial_chars);

        if (strcmp(initial_chars, "GET ") == 0) {

          //copy of the name_file from the message received by client
          char name_file[MAXBUF];
          int i, j = 0;
          for (i = 4; my_buffer[i] != '\r'; i++) {
            name_file[j] = my_buffer[i];
            j++;
          }
          name_file[j] = '\0';

          FILE * filefd;
          filefd = fopen(name_file, "rb");

          if (filefd != NULL) {

            fseek(filefd, 0, SEEK_END); //seek to end of file
            uint32_t size = ftell(filefd); //get current file pointer
            fseek(filefd, 0, SEEK_SET); //seek back to beginning of file

            uint32_t size_nbo = size;
            size_nbo = htonl(size_nbo);
            if (sendn(connfd, "+OK\r\n", 5 /*5bytes da inviare*/ , MSG_NOSIGNAL) == -1) break;
            if (sendn(connfd, (void * ) & size_nbo, sizeof(uint32_t), MSG_NOSIGNAL) == -1) break;

            char buffer[1024];
            //printf("Dimensione del file:%u\n", size);

            int bytes_read, flag = 0;
            if (sizeof(buffer) < size) {
              while (!feof(filefd)) {
                //fread read sizeof(buffer) of 1 bytes e return the number of element read
                if ((bytes_read = fread(buffer, 1, sizeof(buffer), filefd)) > 0) {
                  //printf("roba %d -- %s -- %d\n", i, buffer, bytes_read);					
                  if (sendn(connfd, buffer, bytes_read, MSG_NOSIGNAL) == -1) {
                    //printf("break\n");
                    fclose(filefd);
                    flag = 46;
                    break;
                  }
                } //else{printf("1 Errore nella fread\n");} Non mi serve più questo controllo perchè mi funziona
              }
              if (flag == 46) break;

            } else {
              if (fread(buffer, size, 1, filefd) == 1) {
                //printf("roba %d -- %s -- buffer:%ld sizefile: %d\n", i, buffer, sizeof(buffer), size);					
                if (sendn(connfd, buffer, size, MSG_NOSIGNAL) == -1) {
                  fclose(filefd);
                  break;
                }
              } //else{printf("2 Errore nella fread\n");}
            }
            fclose(filefd);

            uint32_t last_modified;
            struct stat attrib;
            stat(name_file, & attrib);
            last_modified = (uint32_t) attrib.st_mtime;
            //printf("Last modified:%u\n", last_modified);
            last_modified = htonl(last_modified);

            if (sendn(connfd, (void * ) & last_modified, 4, MSG_NOSIGNAL) == -1) 
              break;

          } else {
            printf("file error\n");
            sendn(connfd, "-ERR\r\n", 6 /*6bytes da inviare*/ , MSG_NOSIGNAL);
            break;
          }
        } else {
          sendn(connfd, "-ERR\r\n", 6 /*6bytes da inviare*/ , MSG_NOSIGNAL);
          break;
        }
      } else if (read_result == -5) { //mi faccio ritornare -5 se la readline fa timeout             
        printf("Timeout 15 seconds passed\n");
        break;

      } else {
        printf("read_result error\n");
        sendn(connfd, "-ERR\r\n", 6 /*6bytes da inviare*/ , MSG_NOSIGNAL);
        break;
      }
    } //while that serves gets from client	
    printf("Close(connfd)\n");
    close(connfd);
  } //while that serves accept

  return 0;
}