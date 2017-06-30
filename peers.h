#include <netinet/in.h>

//struct peer *peerSet = NULL;
//struct peer *lastPeer = NULL;
//int size = 0;
struct peer {
  struct peer *prev;
  struct peer *next;
  struct in_addr sin_addr; // IP of the peer (really an unsigned long)
  time_t last_seen;
};

extern struct peer *peerSet;
extern struct peer *lastPeer;
extern int size;


// append to list or update timestamp
int putip(struct in_addr *sa);
int del(struct in_addr sa);
