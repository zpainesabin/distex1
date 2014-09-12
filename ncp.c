#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sendto_dbg.h"
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>


int main(int argc, char *argv[]) {
  
  if (argc != 4) {
   printf( "Bad input\n");
   exit(1);
  }
  
  int loss_percent = atoi(argv[1]);
  char * source_file = argv[2];
  char * destination = argv[3];
  
  char * token = strtok(destination, "@");
  char * dest_file_name = token;
  token = strtok(NULL, "@");
  char * dest_computer = token;

  printf("%i %s %s %s\n",loss_percent, source_file, dest_file_name, dest_computer);

  sendto_dbg_init(loss_percent);
  
  int rate, s, i;
  struct sockaddr_in to_addr;
  struct hostent h_ent;
  struct hostent *p_h_ent;
  long host_num;
  char * buf = "hello";

  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s<0) {
    perror("Ucast: socket");
    exit(1);
  } //Copied from Yair's test.c


  to_addr.sin_family = AF_INET;
  to_addr.sin_port = htons(10220);

  /* Get the IP address of the destination machine */
  p_h_ent = gethostbyname(dest_computer);
  if(p_h_ent == NULL ) {
    printf("Ucast: gethostbyname error.\n");
    exit(1);
  }

  memcpy( &h_ent, p_h_ent, sizeof(h_ent));
  memcpy( &host_num, h_ent.h_addr_list[0], sizeof(host_num) );

  to_addr.sin_addr.s_addr = host_num;

  for(i = 0; i < 10; i++) {
    sendto_dbg(s, buf, 6, 0, (struct sockaddr *)&to_addr,
               sizeof(to_addr));
  }

 }
