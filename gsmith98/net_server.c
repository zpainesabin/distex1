#include "net_include.h"
#include <sys/time.h>

int main()
{
    struct sockaddr_in name;
    int                s;
    fd_set             mask;
    int                recv_s[10];
    int                valid[10];  
    fd_set             dummy_mask,temp_mask;
    int                i,j,num;
    int                mess_len;
    int                neto_len;
    char               mess_buf[MAX_MESS_LEN];
    long               on=1;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s<0) {
        perror("Net_server: socket");
        exit(1);
    }

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
        perror("Net_server: setsockopt error \n");
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    if ( bind( s, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("Net_server: bind");
        exit(1);
    }
 
    if (listen(s, 4) < 0) {
        perror("Net_server: listen");
        exit(1);
    }

    i = 0;
    int is_first = 0;
    int no_reception = 0;
    FILE * fw;
    struct timeval first, last;
    gettimeofday(&first, NULL);
    int total_bytes=0;
    FD_ZERO(&mask);
    FD_ZERO(&dummy_mask);
    FD_SET(s,&mask);
    for(;;)
    {
        temp_mask = mask;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, NULL);
        if (num > 0) {
            if ( FD_ISSET(s,&temp_mask) ) {
                recv_s[i] = accept(s, 0, 0) ;
                FD_SET(recv_s[i], &mask);
                valid[i] = 1;
                i++;
            }
            for(j=0; j<i ; j++)
            {   if (valid[j]) {   
                if ( FD_ISSET(recv_s[j],&temp_mask) ) {
                    if (is_first == 0) {
                     is_first = 1;
                     int length;
                     if (recv(recv_s[j], &length, sizeof(length),0) > 0)                     {
                      int strlen = length - sizeof(length);
                      recv(recv_s[j], mess_buf, strlen, 0);
                      mess_buf[strlen] = '\0';
                      printf("Writing to %s\n", mess_buf);
                      gettimeofday(&first, NULL);
                      fw = fopen(mess_buf, "w");
                     }
                    }
                    int len = recv(recv_s[j],mess_buf,MAX_MESS_LEN,0);
                    if( len > 0) {
                        no_reception = 0;
                        fwrite(mess_buf, 1, len, fw);
                        total_bytes = total_bytes + len;
                     }
                      else {
                        no_reception++;
                        if (no_reception == 2) {
                          FD_CLR(recv_s[j], &mask);
                          close(recv_s[j]);
                          valid[j]=0;
                          if (fw != NULL) {
                            fclose(fw);
                          }
                          gettimeofday(&last, NULL);
                          double difference = (double)(last.tv_sec - first.tv_sec) + (double)(last.tv_usec - first.tv_usec) / 1000000.0;
                          double total = (double)total_bytes / 1000000.0;
                          printf("Transfer completed. Total Megabytes %f. Total Time %f. Average Rate (Mbits/sec) %f.\n", total, difference, (total*8.0) / difference);
                          exit(1);
                         }
                      }
                  }
               }
            }
        }
    }
    return 0;

}

