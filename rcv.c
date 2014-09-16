#include "net_include.h"

#define NAME_LENGTH 80

int gethostname(char*,size_t);

int ez_select();

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


 
int main(int argc, char *argv[])
{
    /*Much code taken from tutorial's Ucast cs437*/
    if (argc < 2) {
        printf("rcv loss_percent\n");
        exit(1);
    } 
    loss_percent = atoi(argv[1]);
    
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

/* 
    ss = socket(AF_INET, SOCK_DGRAM, 0); // socket for sending (udp)
    if (ss<0) {
        perror("Ucast: socket");
        exit(1);
    }
*/  
  
/*    PromptForHostName(my_name,host_name,NAME_LENGTH);
    
    p_h_ent = gethostbyname(host_name);
    if ( p_h_ent == NULL ) {
        printf("Ucast: gethostbyname error.\n");
        exit(1);
    }

    memcpy( &h_ent, p_h_ent, sizeof(h_ent));
    memcpy( &host_num, h_ent.h_addr_list[0], sizeof(host_num) );
*/
/*
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = host_num; 
    send_addr.sin_port = htons(PORT);
*/
    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );
    FD_SET( (long)0, &mask ); /* stdin */

/* AT THIS POINT WE ARE SET UP TO RECEIVE*/


    for(;;)
    {
       num = ez_select();
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                /* WE RECEIVED SOMETHING*/

               bytes = ez_receive(); /*sets mess_buf to received string, and from_addr to sender's address*/
               mess_buf[bytes] = 0; /*0 acts as end of packet character*/
               /* from_ip = from_addr.sin_addr.s_addr;

                printf( "Received from (%d.%d.%d.%d): %s\n", 
                                (htonl(from_ip) & 0xff000000)>>24,
                                (htonl(from_ip) & 0x00ff0000)>>16,
                                (htonl(from_ip) & 0x0000ff00)>>8,
                                (htonl(from_ip) & 0x000000ff),
                                mess_buf );
              */
 }
        } else {
            printf(".");
            fflush(0);
        }
    }

    return 0;

}

int ez_select() {
    temp_mask = mask;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    return select(FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
}

int ez_receive() {
    from_len = sizeof(from_addr);
    return  recvfrom( sr, mess_buf, sizeof(mess_buf), 0, (struct sockaddr *)&from_addr, &from_len);
}
 
/*
void PromptForHostName( char *my_name, char *host_name, size_t max_len ) {

    char *c;

    gethostname(my_name, max_len );
    printf("My host name is %s.\n", my_name);

    printf( "\nEnter host to send to:\n" );
    if ( fgets(host_name,max_len,stdin) == NULL ) {
        perror("Ucast: read_name");
        exit(1);
    }
    
    c = strchr(host_name,'\n');
    if ( c ) *c = '\0';
    c = strchr(host_name,'\r');
    if ( c ) *c = '\0';

    printf( "Sending from %s to %s.\n", my_name, host_name );
*/

