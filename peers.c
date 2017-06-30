//#include <sys/socket.h>
//#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <time.h>
#include "peers.h"

#define MAX_PEERS 5

//struct peer {
//  struct peer *prev; 
//  struct peer *next; 
//  struct in_addr sin_addr; // IP of the peer (really an unsigned long)
//  time_t last_seen;
//};
//
struct peer *peerSet = NULL;
struct peer *lastPeer = NULL;
int size = 0;

// append to list or update timestamp
int putip(struct in_addr *sa) {
  struct peer *iter = peerSet;

  while (iter != NULL) {
    if (iter->sin_addr.s_addr == sa->s_addr) {
      // we already know this peer
      // update it's timestamp and move it to the front of the list
      iter->last_seen = time(NULL);
      if (iter == peerSet) {
        // peer is already first in line, no need to move it there
        return 1;
      } 

      if (iter->next != NULL) {
        iter->next->prev = iter->prev;
      }

      iter->prev->next = iter->next;
      if (iter->next == NULL) {
        lastPeer = iter->prev;
      }

      iter->next = peerSet;
      iter->prev = NULL;
      peerSet->prev = iter;
      peerSet = iter;

      return 1;
      }
    iter = iter->next;
  }
  
  // we haven't seen this peer before
  // so put it in front
  struct peer *p = calloc(sizeof(struct peer), 1);
  p->sin_addr = *sa;
  p->last_seen = time(NULL);
  if (peerSet == NULL) {
    lastPeer = p;
  } else {
    peerSet->prev = p;
  }
  p->next = peerSet;
  peerSet = p;

  if (size == MAX_PEERS) {
    // kick the longest inactive peer, i.e. the peer on the end 
    p = lastPeer;
    lastPeer->prev->next = NULL;
    lastPeer = lastPeer->prev;
    free(p);
  } else {
    size++;
  }
  return 2;
}
