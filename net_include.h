#include <stdio.h>

#include <stdlib.h>

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h> 
#include <netdb.h>

#include <errno.h>

#define PORT	     10220

#define MAX_MESS_LEN 8192

typedef struct
{
  int packet_type;
  int index;
  char payload[MAX_MESS_LEN - 8]
} packet;

