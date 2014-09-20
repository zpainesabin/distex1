#include "net_include.h"

int getNextPacket(char * next);
FILE * s_file;
int main(int argc, char * argv[])
{
    struct sockaddr_in host;
    struct hostent     h_ent, *p_h_ent;
    char               host_name[80];
    char               *c;

    int                s;
    int                ret;
    int                mess_len;
    char               mess_buf[MAX_MESS_LEN];
    char               *neto_mess_ptr = &mess_buf[sizeof(mess_len)]; 

 if (argc != 3) {
   printf( "Bad input\n");
   exit(1);
  }

  char * source_file = argv[1];
  char * destination = argv[2];

  char * token = strtok(destination, "@");
  char * dest_file_name = token;
  token = strtok(NULL, "@");
  char * dest_computer = token;

  printf("%i %s %s\n", source_file, dest_file_name, dest_computer);

  printf("fopen opening\n");
  s_file = fopen(source_file, "r");
  printf("fopen opened\n");

    s = socket(AF_INET, SOCK_STREAM, 0); /* Create a socket (TCP) */
    if (s<0) {
        perror("Net_client: socket error");
        exit(1);
    }

    host.sin_family = AF_INET;
    host.sin_port   = htons(PORT);

    p_h_ent = gethostbyname(dest_computer);
    if ( p_h_ent == NULL ) {
        printf("net_client: gethostbyname error.\n");
        exit(1);
    }

    memcpy( &h_ent, p_h_ent, sizeof(h_ent) );
    memcpy( &host.sin_addr, h_ent.h_addr_list[0],  sizeof(host.sin_addr) );

    ret = connect(s, (struct sockaddr *)&host, sizeof(host) ); /* Connect! */
    if( ret < 0)
    {
        perror( "Net_client: could not connect to server"); 
        exit(1);
    }

    int filename_len = strlen(dest_file_name) + sizeof(int);
    memcpy(mess_buf, &filename_len, sizeof(int));
    printf("%p %s %i \n", &mess_buf[sizeof(int)], dest_file_name, strlen(dest_file_name));
    memcpy(&mess_buf[sizeof(int)], dest_file_name, strlen(dest_file_name));
    ret = send(s, mess_buf, filename_len, 0);
    int prev_size = MAX_MESS_LEN;
    while (prev_size == MAX_MESS_LEN)
    {
        char next[MAX_MESS_LEN];
        mess_len = getNextPacket(next);

        ret = send( s, next, mess_len, 0);
        if(ret != mess_len) 
        {
            perror( "Net_client: error in writing");
            exit(1);
        }
    }

    return 0;

}

int getNextPacket(char* next) {
  int bytesRead = fread(next, 1, MAX_MESS_LEN, s_file);
  return bytesRead;
}
