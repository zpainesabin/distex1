#include "net_include.h"
#include "sendto_dbg.h"
#include <string.h>

#define NAME_LENGTH 80

int ez_select();
int ez_receive();
void ez_send(packet message, int size);

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
        char * filename;
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
    packet * window[WINDOW_SIZE]; /*ARRAY OF POINTERS TO PACKETS*/
    int win_packs = 0;
    FILE *fp;
    int last_index = -1; /*final packet index; -1 if not set*/
    int last_bytes = -1;
    int bytes_written = 0;

    for(i=0; i<WINDOW_SIZE; i++) {
                    window[i] = NULL; 
    }

    for(;;)
    { 
        num = ez_select();
        if (num > 0 && FD_ISSET(sr, &temp_mask)) {
            /* WE RECEIVED SOMETHING*/
            bytes = ez_receive(); /*sets mess_buf to received 'string', and from_addr to sender's address*/
            from_ip = from_addr.sin_addr.s_addr;

            in_packet = *((packet *) mess_buf);

            if (from_ip != current_ip) {
                if (current_ip == 0) { /*sevice him*/
                    send_addr.sin_addr.s_addr = from_ip;
                    current_ip = from_ip; 
                    curr_index = 0; 

                    char filename[bytes - 8];   /* TRY sizeof in_packet.payload!!!!!!!!!!!!!!!!!!!!*/
                    memcpy(filename, in_packet.payload, bytes-8);
                    char concat[MAX_MESS_LEN]; 
                    strcpy(concat, "/tmp/\0");
                    printf(strcat(concat, filename));
                    printf("\n");
                    fp = fopen(concat, "w");
                    /*Will fall down to the type 3 case below*/ 
                } else { /*queue him*/
                    qTail->next = malloc(sizeof(qNode));
                    qTail = qTail->next;
                    qTail->ip = from_ip;
                    qTail->filename = malloc(bytes-8);
                    memcpy(qTail->filename, in_packet.payload, bytes-8); 
                    qTail->next = NULL;
                                            /*ONLY QUEUE HIM IF HE"S NOT ALREADY IN QUEUEi!!!!!!!!!!!!!!!*/
                    /*Send a nack*/
                    packet nack;
                    nack.packet_type = 2;
                    send_addr.sin_addr.s_addr = from_ip;
                    ez_send(nack, 8);
                    send_addr.sin_addr.s_addr = current_ip;
                    continue; /*Do nothing else with message*/
                }        
            }

            /* Handle packet --------------------------------------------------------------------------*/
            if (in_packet.packet_type == 3) { /*initiator 3*/
                packet first_ack;
                first_ack.packet_type = 4; /*initiator confirmation 4*/
                ez_send(first_ack, 8); /*signals that rcv is ready*/

            } else if (in_packet.index == curr_index) { /*data to write*/
                packet ack;
                ack.packet_type = 1;
                fwrite(in_packet.payload, 1, bytes-8, fp);
                bytes_written += bytes;
                    
                curr_index++;
                int num_written = 1;
                /*Write array up to first null*/   
                for( int n=1; n<WINDOW_SIZE && window[n] !=NULL; n++) {
 	            if (window[n]->index != last_index) {
                        fwrite(window[n]->payload, 1, bytes-8, fp);
                        bytes_written += bytes;
                    } else {
                        fwrite(window[n]->payload, 1, last_bytes-8, fp);
                        bytes_written += last_bytes;
                    }
                    free(window[n]);
//                    window[n] = NULL; /*POSSIBLE ERROR!!! CODE SEGFAULTS IF THIS IS GONE!!!!!!!!!!!!!!!!*/
                    curr_index++;
                    num_written++;
                    win_packs--;
                }
                /*Shift over*/             /*RIGHT NOW I'M SHIFTING OVER EVERY TIME!!!!!!!!!!*/
                if (win_packs !=0) {                           /*ADD IF WINPACKS!=0!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
                    for(int n=0; n < WINDOW_SIZE; n++) {
                        if (n+num_written < WINDOW_SIZE) {
                            window[n] = window[n+num_written];
                        } else { 
                            window[n] = NULL;
                        }
                    }
                }
                ack.index = curr_index-1;
                ez_send(ack, 8);
                printf("JUST ACKED %i\n", ack.index);

                if (bytes < MAX_MESS_LEN) { /*this was the last packet*/
                    last_index = in_packet.index;
                    printf("last_index is %i", last_index);
                }

                if (last_index != -1 && curr_index > last_index) { /*I've acked every packet*/
                    fclose(fp);
                    printf("IT'S OVER!!!\n");
                    exit(1);          /*MOVE ON To NEXT GUY!!!!!!!!!!!!!!!!!!!!!!!!!!*/
                }

            } else if (in_packet.index < curr_index) { /*An ack might have been missed, send cumulative ack again*/
                packet ack;
                ack.packet_type = 1;
                ack.index = curr_index - 1;
                ez_send(ack, 8);
                printf("JUST RE-ACKED %i\n", ack.index); 
            } else { /*We missed something before what we just received*/
                printf("packet %i, curr %i\n", in_packet.index, curr_index);
                if (window[in_packet.index-curr_index] == NULL) { 
                    window[in_packet.index-curr_index] = malloc(sizeof(packet));
                    memcpy(window[in_packet.index-curr_index], &in_packet, sizeof(packet));
                    win_packs++;
                }

                packet nack;
                nack.index = in_packet.index;
                nack.packet_type = 2;
                ez_send(nack, 8);

                if (bytes < MAX_MESS_LEN) { /*this was the last packet*/
                    last_index = in_packet.index;
                    last_bytes = bytes;
                    printf("last_index is %i\n", last_index);
                }

            } /* End handle packet --------------------------------------------------------------------------*/

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

void ez_send(packet message, int size) {
  sendto_dbg(ss, (char*) &message, size, 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
  printf("%i\n", message.packet_type);
}
