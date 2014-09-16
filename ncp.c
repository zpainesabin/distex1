#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sendto_dbg.h"
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "net_include.h"
#include <time.h>

#define NAME_LENGTH 80
#define WINDOW_SIZE 10

int g_s;
struct sockaddr_in g_to_addr;

void ez_send(char * message, int size);
packet getNextPacket();

int cur_index = 0;

int gethostname(char*,size_t);

int ez_select();

int ez_receive();

int  send_start_packet(char * dest_file_name);

/*void PromptForHostName( char *my_name, char *host_name, size_t max_len );*/

    fd_set                mask;
    fd_set                dummy_mask,temp_mask;
    int                   sr;
    struct timeval        timeout;

    struct sockaddr_in    name;
    struct sockaddr_in    send_addr;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    struct hostent        h_ent;
    struct hostent        *p_h_ent;
    char                  host_name[NAME_LENGTH] = {'\0'};
    char                  my_name[NAME_LENGTH] = {'\0'};
    int                   host_num;
    int                   from_ip;
    int                   ss;
    int                   bytes;
    int                   num;
    char                  mess_buf[MAX_MESS_LEN];
    char                  input_buf[80];
    int                   loss_percent;


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

  
    sr = socket(AF_INET, SOCK_DGRAM, 0);  /* socket for receiving (udp) */
    if (sr<0) {
        perror("Ucast: socket");
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT); /*10220*/

    if ( bind( sr, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("Ucast: bind");
        exit(1);
    }

    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );
    FD_SET( (long)0, &mask ); /* stdin */

/* AT THIS POINT WE ARE SET UP TO RECEIVE*/

  
  int rate, i;
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

  int start = send_start_packet(dest_file_name);

  if (start > 0) {
    
    printf("Starting transmission");

  }
/*
  send_file(source_file);





  for(i = 0; i < 10; i++) {
    packet temp = getNextPacket();
    packet * temp_pointer = &temp;
    ez_send((char *) temp_pointer, sizeof(packet));
    printf("Sending %i \n", sizeof(packet));
    //sleep(1);
    //ez_send(buf,6);
  }

  int num_r = 0;
  while (num_r < 10) {
    num = ez_select();
    if (num > 0 && FD_ISSET(sr, &temp_mask)) {
      packet in_packet;
      bytes = ez_receive();
      from_ip = from_addr.sin_addr.s_addr;
      in_packet = *((packet *) mess_buf);
      printf("%s\n", in_packet.payload);
      num_r++;
    } else {
      printf(".");
      fflush(0);
    }
    }
*/
 }
/*
void  send_file(char * source_file) {

  packet * window[WINDOW_SIZE];
  for (int x=0; x<5; x++) {
    packet nextPacket = getNextPacket();
    if (packet.packet_type == 9) {
      return;
    } else {
      curr_packet_point = malloc(sizeof(packet));
      memcpy(curr_packet_point, &nextPacket, sizeof(packet));
      window[x] = curr_packet_point;
  bool cont = true;
  while (cont) {
    packet nextPacket = getNextPacket();
    if (packet.packet_type == 9) {
      cont = false;
    } else {
      

*/

int send_start_packet(char * dest_file_name){
  int success = 0;

  while (success == 0) {
    packet start_packet;
    start_packet.index = 0;
    start_packet.packet_type = 3;
    strcpy(start_packet.payload, dest_file_name);
    packet * start_packet_pointer = &start_packet;
    ez_send((char *) start_packet_pointer, sizeof(packet));
    num = ez_select();
    if (num > 0 && FD_ISSET(sr, &temp_mask)) {
      packet in_packet;
      bytes = ez_receive();
      from_ip = from_addr.sin_addr.s_addr;
      in_packet = *((packet *) mess_buf);
      if (in_packet.packet_type == 1) {
        printf(in_packet.payload);
        success = 1;
      } else {
        printf("Denied, entering waiting mode");
        //TODO WAIT FOR START ACK
    }
  }
}
  return 1;
}


void ez_send(char * message, int size) {
  sendto_dbg(g_s, message, size, 0, (struct sockaddr *)&g_to_addr, sizeof(g_to_addr));
}

packet getNextPacket() {
  packet next;
  next.index = cur_index;
  next.packet_type = 0;
  strcpy(next.payload, "TestTestTestTest\0");
  cur_index++;
  return next;
}

int ez_select() {
    temp_mask = mask;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    return select(FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
}

int ez_receive() {
    from_len = sizeof(from_addr);
    return  recvfrom( sr, mess_buf, sizeof(mess_buf), 0, (struct sockaddr *)&from_addr, &from_len);
}

