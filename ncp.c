#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sendto_dbg.h"
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "net_include.h"

int g_s;
struct sockaddr_in g_to_addr;

void ez_send(char * message, int size);
packet getNextPacket();

int cur_index = 0;

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


  /*Code Copied from Yair's test.c*/
  sendto_dbg_init(loss_percent);
  
  int rate, i;
  struct hostent h_ent;
  struct hostent *p_h_ent;
  long host_num;
  char * buf = "hello";

  g_s = socket(AF_INET, SOCK_DGRAM, 0);
  if (g_s<0) {
    perror("Ucast: socket");
    exit(1);
  }
  

  /* Get the IP address of the destination machine */
  p_h_ent = gethostbyname(dest_computer);
  if(p_h_ent == NULL ) {
    printf("Ucast: gethostbyname error.\n");
    exit(1);
  }

  memcpy( &h_ent, p_h_ent, sizeof(h_ent));
  memcpy( &host_num, h_ent.h_addr_list[0], sizeof(host_num) );

  g_to_addr.sin_family = AF_INET;
  g_to_addr.sin_addr.s_addr = host_num;
  g_to_addr.sin_port = htons(PORT); /*10220*/

  for(i = 0; i < 10; i++) {
    packet temp = getNextPacket();
    packet * temp_pointer = &temp;
    ez_send((char *) temp_pointer, sizeof(packet));
  }

 }

void ez_send(char * message, int size) {
  sendto_dbg(g_s, message, size, 0, (struct sockaddr *)&g_to_addr, sizeof(g_to_addr));
}

packet getNextPacket() {
  packet next;
  next.index = cur_index;
  next.packet_type = 0;
  next.payload = "TestTestTestTest\0";
  cur_index++;
  return next;
}


