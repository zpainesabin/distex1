#include "net_include.h"
#include <string.h>

#define NAME_LENGTH 80

int ez_select();
int ez_receive();
void ez_send(packet message);

void setup();


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
    int                   from_ip;
    int                   ss;
    int                   bytes;
    int                   num;
    char                  mess_buf[MAX_MESS_LEN];
    char                  input_buf[80];
    int                   loss_percent;

    struct                hostent h_ent;

    typedef struct dummy_node {
        int ip;
        struct dummy_node *next;
    } qNode;
        
    qNode   qHead;
    qNode * qTail;
 
 
int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("rcv loss_percent\n");
        exit(1);
    } 
    loss_percent = atoi(argv[1]);
    
    sendto_dbg_init(loss_percent);

    setup(); /*all the one time setup for receiving and some other business*/

    /*setup queue*/
    qHead.next = NULL;
    qTail = &qHead;

    packet in_packet;
    int i; /*for iterating for loops*/
    int current_ip = 0;
    int curr_index = 0; /*the index expected next*/
    int window_size = 5; /*arbitrarily chosen default alue that shouldn't ever be used*/
    packet ** window; /*WANT THIS TO BE AN ARRAY OF POINTERS TO PACKETS (SIZE UNKNOWN)!!!!!!!!!!!!!!!*/
    for(;;)
    { 
        num = ez_select();
        if (num > 0 && FD_ISSET(sr, &temp_mask)) {
            /* WE RECEIVED SOMETHING*/
            bytes = ez_receive(); /*sets mess_buf to received 'string', and from_addr to sender's address*/
            from_ip = from_addr.sin_addr.s_addr;

            if (from_ip != current_ip) {
                if (current_ip == 0) { /*sevice him*/
                    send_addr.sin_addr.s_addr = from_ip;
                    current_ip = from_ip; 
                    curr_index = 0; 
                    /*Will fall down to the type 3 case below*/ 
                } else { /*queue him*/
                    qTail->next = malloc(sizeof(qNode));
                    qTail = qTail->next;
                    qTail->ip = from_ip;
                    qTail->next = NULL;

                    /*Send a nack*/
                    packet nack;
                    nack.packet_type = 2;
                    ez_send(nack);
                }        
            }

            in_packet = *((packet *) mess_buf);

            if (in_packet.packet_type == 3) { /*initiator*/

                /*INITIATOR PAYLOAD: "FILENAME|WINDOWSIZE"*/
                char * init_load = in_packet.payload;
                char * filename = strtok(init_load, "|");
                window_size = atoi(strtok(NULL, "|"));                
                window = malloc(window_size*sizeof(packet *));
                for(i=0; i<window_size; i++) {
                    window[i] = NULL; 
                }
                //OPEN FILE USING FILENAME (tmp folder)!!!!!!!!!!!!!!!!!!!!!

                packet first_ack;
                first_ack.packet_type = 1;
                ez_send(first_ack); /*signals that rcv is ready*/
            } else { /*it's data*/
                if (in_packet.index == curr_index) {
                    packet ack;
                    ack.packet_type = 1;
                    /*CHECK WHAT I HAVE TO SEE WHAT I SHOULD ACK!!!!!!!!!!!!!!!!!!*/
                    ack.index = curr_index;
                    ez_send(ack);
                    
                    /*WRITE WHAT I HAVE!!!!!!!!!!!!!!!!!!!!*/
                    /*'MOVE WINDOW'!!!!!!!!!!!!!!!!!!!!!!!!!1*/
                    

                } else { /*missed something before what we just received*/
                    /*store what we just got!!!!!!!!!!!!!!!!!!!!!1*/
                    /*STORE ALL NACKS!!!!!!!!!!!!!!!!!!!!!*/
                    packet nack;
                    nack.index = curr_index-1;
                    strcpy(nack.payload, "LIST OF NACKS!!!!!!!"); /*LIST NACKS!!!!!!!!!!!!!*/
                    ez_send(nack);

                }
            }
        } else if (num <= 0) {
            printf(".");
            fflush(0);
        }
    }
    return 0;
}



void setup() {
    /*Much code taken from tutorial's Ucast cs437*/

    /*ONE TIME SETUP-------------------------------------------------------------*/
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

    ss = socket(AF_INET, SOCK_DGRAM, 0); // socket for sending (udp)
    if (ss<0) {
        perror("Ucast: socket");
        exit(1);
    }  
  
    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );
    FD_SET( (long)0, &mask ); /* stdin */

    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(PORT);
 
/* AT THIS POINT WE ARE SET UP TO RECEIVE------------------------------------------------------*/
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

void ez_send(packet message) {
  sendto_dbg(ss, (char*) &message, sizeof(packet), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
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

