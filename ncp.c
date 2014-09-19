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

int g_s;
struct sockaddr_in g_to_addr;

void ez_send(char * message, int size);
int getNextPacket(packet * next);

int cur_index = 0;
void  send_file(char * source_file);
int gethostname(char*,size_t);
int getRecvd(int acks[], int startIndex);
int send_start_packet(char * dest_file_name);

int ez_select(int tout);

int ez_receive();

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
    FILE *                s_file;

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

  printf("fopen opening\n"); 
  s_file = fopen(source_file, "r"); 
  printf("fopen opened\n"); 
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

  send_file(source_file);

 }

void  send_file(char * source_file) {
  int done = 0;
  int no_response_counter = 0;
  int final_size = 0;
  int last_pack_index = WINDOW_SIZE-1;
  int total_size_bytes = 0;
  packet * window[WINDOW_SIZE];
  int received[WINDOW_SIZE];
  for (int x=0; x<WINDOW_SIZE; x++) {
    packet nextPacket;
    int bytes = getNextPacket(&nextPacket);
    received[x] = 0;
    if (bytes == 0) {
      received[x] = 1;
      if (x < last_pack_index) {
        last_pack_index = x;
        received[x] = 0;
      }
    }
    else if (bytes < MAX_MESS_LEN-8) {
        done = 1;
        final_size = bytes;
        last_pack_index = x;
    } 
    packet * curr_packet_point = malloc(sizeof(packet));
    memcpy(curr_packet_point, &nextPacket, sizeof(packet));
    window[x] = curr_packet_point;
    
  }
    
  int cont = 0;
  while (cont == 0) {
    int count_not_received = 0;
    int send_count = 0;
    for (int x=0; x<WINDOW_SIZE; x++) {
      if ((received[x] == 0) && ((done==0) || (x<last_pack_index))) {
        count_not_received++;
        ez_send((char *) window[x], sizeof(packet));
      } else if (received[x] == 0) {
        count_not_received++;
        ez_send((char *) window[x], final_size + 8);
      }
    }
    if (count_not_received == 0){
      exit(1);
    } 

    int acks[WINDOW_SIZE] = {0};
    int acked = getRecvd(acks, window[0]->index);
    if (acked == 0) {
      no_response_counter++;
      if (no_response_counter > 10) {
        printf("No response for 10 attempts, exiting");
        exit(1);
      }
    } else {
      no_response_counter = 0;
    }
    for (int i=0; i<WINDOW_SIZE; i++) {
      if (acks[i] == 1) {
        
        received[i] = 1;
      }
    }
     while ((received[0] == 1) && (done==0)) {
      free(window[0]);
      for (int n=0; n<WINDOW_SIZE-1; n++) {
        window[n] = window[n+1];
        received[n] = received[n+1];
      }
      packet nextPacket;
      int bytes = getNextPacket(&nextPacket);
      if (bytes < MAX_MESS_LEN-8) {
        done = 1;
        final_size = bytes;
      }
      packet * curr_packet_point = malloc(sizeof(packet));
      memcpy(curr_packet_point, &nextPacket, sizeof(packet));
      window[WINDOW_SIZE-1] = curr_packet_point;
      received[WINDOW_SIZE-1] = 0;
    }

  }

}

int getRecvd(int acks[], int startIndex) {
  int got_an_ack = 0;
  for (int x=0; x<WINDOW_SIZE; x++) {
    int num;    
    if (x==0) {
      num = ez_select(1000);
    } else {
      num = ez_select(10);
    }
   if (num > 0 && FD_ISSET(sr, &temp_mask)) {
      packet in_packet;
      bytes = ez_receive();
      from_ip = from_addr.sin_addr.s_addr;
      in_packet = *((packet *) mess_buf);
      if (in_packet.index < startIndex) {
        printf("UHOH");
      }
      printf("%i %i packet type packet index \n", in_packet.packet_type, in_packet.index);
      if (in_packet.packet_type == 1) {
          got_an_ack = 1;
          int z = in_packet.index - startIndex + 1;
          for (int i=0; i<z; i++) {
            acks[i] = 1;
          }
      } else if (in_packet.packet_type == 2)  {
          got_an_ack = 1;
          acks[in_packet.index - startIndex] = 1;
      }
    }
  }
  printf("\n %i acks: ", startIndex);
  for (int i=0; i<WINDOW_SIZE; i++) {
    printf("%i", acks[i]);
  }
  printf("\n");
  return got_an_ack;
}
 


int send_start_packet(char * dest_file_name){
  int success = 0;

  while (success == 0) {
    packet start_packet;
    start_packet.index = 0;
    start_packet.packet_type = 3;
    strcpy(start_packet.payload, dest_file_name);
    packet * start_packet_pointer = &start_packet;
    printf("%s\n", start_packet.payload);
    ez_send((char *) start_packet_pointer, sizeof(packet));
    num = ez_select(1000);
    if (num > 0 && FD_ISSET(sr, &temp_mask)) {
      packet in_packet;
      bytes = ez_receive();
      from_ip = from_addr.sin_addr.s_addr;
      in_packet = *((packet *) mess_buf);
      if (in_packet.packet_type == 4) {
        success = 1;
      //} else {
        //printf("Denied, entering waiting mode");
        //waitForStartMessage();
      }
    }
  }
  return 1;
}

void waitForStartMessage() {
  int success = 0; 
  while (success == 0) {
    int num = ez_select(1000);
    if (num > 0 && FD_ISSET(sr, &temp_mask)) {
    packet start_packet;
    bytes = ez_receive();
    start_packet = *((packet *) mess_buf);
    if (start_packet.packet_type == 4) {
      return;
    }
  }
   } 
}


void ez_send(char * message, int size) {
  printf("sending packet %i size %i\n", ((packet *)message)->index, size);
  sendto_dbg(g_s, message, size, 0, (struct sockaddr *)&g_to_addr, sizeof(g_to_addr));
}

int getNextPacket(packet* next) {
  next->index = cur_index;
  next->packet_type = 0;
  int bytesRead = fread(&(next->payload), 1, MAX_MESS_LEN-8, s_file);
  printf("%i bytes read from file", bytesRead);
  cur_index++;
  return bytesRead;
}

int ez_select(int tout) {
    temp_mask = mask;
    timeout.tv_sec = tout/1000;
    timeout.tv_usec = tout%1000;
    return select(FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
}

int ez_receive() {
    from_len = sizeof(from_addr);
    return  recvfrom( sr, mess_buf, sizeof(mess_buf), 0, (struct sockaddr *)&from_addr, &from_len);
}

