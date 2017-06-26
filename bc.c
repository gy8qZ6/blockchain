#include <stdlib.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define handleError(msg) do { perror(msg); exit(1); } while (0)

#define DEBUG 1
#define BLOCK_SIZE 512
#define HASH_SIZE SHA256_DIGEST_LENGTH
#define IP "127.0.0.1"
#define PORT 4444
#define HASHBLOCK_SIZE HASH_SIZE+BLOCK_SIZE+sizeof(((struct block*)0)->nonce)

struct peerConnection {
  int sock;
  struct sockaddr_in sa_in;
  int sa_in_length;
};

struct block {
  struct block *prevBlock;
  struct block *nextBlock;
  char hash[HASH_SIZE];  // is calculated over prevBlock.hash+data+nonce
  char data[BLOCK_SIZE];
  int nonce; // 4Byte
};

char existsNewBlock = 0;
char *ip = NULL;
int port = 0;
int clientFlag=0,serverFlag=0;
struct block *lastBlock = NULL;
struct block *firstBlock = NULL;
struct block *currentBlock = NULL;

/* initialize program state:
   - load blockchain from disk
   - ask peers for new blocks and download them
   - get challenge (from peers or otherwise) and start solving
int init() {

}
*/
int printBlocks(struct block *start) {
  struct block *cur = start;
  int count = 0;
  int i;
  while (cur != NULL) {
    printf("[block #%d\n", count);
    printf("hash: ");
    for (i=0;i<SHA256_DIGEST_LENGTH;i++) {
      printf("%02x", (unsigned char)cur->hash[i]);
    }
    putchar('\n');
    printf("nonce: %d\n", cur->nonce);
    printf("data: ");
    for (i=0;i<512;i++) {
      printf("%02x", (unsigned char)cur->data[i]);
    }
    puts("]\n");

    count++;
    cur = cur->nextBlock;
  }
}

/* does a calculted block hash meet the hardness requirement? */
int beatsDifficulty(char *hash, int dif) {
  // sanity check: stop if difficulty is 'harder' or equal than bit length of the hash
  if (dif >= 8*HASH_SIZE) return 0;

  int i = 0;
  for (i;dif>0 && i<=HASH_SIZE;i++) {
      for (int j=7;j>=0 && dif>0;j--,dif--) {
          if (hash[i] >> j) return 0;
        //  printf("i: %d j: %d\n", i,j);
      }
  }
  return 1;
}
/* take data and brute force a solution 
   i.e. find a hash(prevHash+block+i) < difficulty
   NOTE: there is a chance to exhaust the nonce value range 
         without finding a solution, better check for that 
   difficulty designates the number of leading zeros required */
int solve(struct block *lastBlock, struct block *b, char difficulty) {
  
  // build the data to hash
  // NOTE: try version without copying data around, use incremental hash calculation
  //       init, update, final 
  char hashBlock[HASHBLOCK_SIZE];
  memcpy(hashBlock, lastBlock->hash, HASH_SIZE);
  memcpy(hashBlock+HASH_SIZE, b->data, BLOCK_SIZE);

  // initialize nonce so that peers don't all calculate the same thing
  b->nonce = random();
  memcpy(hashBlock+HASH_SIZE+BLOCK_SIZE, &(b->nonce), sizeof(((struct block*)0)->nonce));

  long int numberOfTries = 0;

  //unsigned char hash[SHA256_DIGEST_LENGTH];
  do {
    b->nonce++;
    // NOTE: replace memcpy with int *nonce = hashBlock+HASH_SIZE+BLOCK_SIZE; *nonce++;
    //       remember to copy nonce to b->nonce when hash is solved
    memcpy(hashBlock+HASH_SIZE+BLOCK_SIZE, &(b->nonce), sizeof(((struct block*)0)->nonce));
    SHA256(hashBlock, HASHBLOCK_SIZE, b->hash);
    if (DEBUG==2) {
      int i;
      printf("trying ");
      for (i=0;i<SHA256_DIGEST_LENGTH;i++) {
        printf("%02x", (unsigned char)b->hash[i]);
      }
      putchar('\n');
    }
    numberOfTries++;
  } while (!beatsDifficulty(b->hash, difficulty));

  if (DEBUG) printf("number of tries to find hash: %lu\n", numberOfTries);

} 

/* recv a logical unit of data
   first 2 bytes signal the length, then recv that many bytes and return */
int recvUnit(int sock, char* buf) {
  int short bytesToReceive;
  recv(sock, &bytesToReceive, 2, 0);
  if (DEBUG) printf("unit size: %d\n", bytesToReceive);

  // recv loop
  int bytesReceived = 0;
  int ret = 0;
  while (bytesToReceive) {
    if ((ret = bytesReceived = recv(sock, buf+bytesReceived, bytesToReceive, 0)) == -1) {
      handleError("recv");
    }
    bytesToReceive -= ret;
    bytesReceived += ret;
  }
  return (bytesToReceive == 0) ? 0 : -1;
}

void* netClient(void *a) {
  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_port = port ? port : htons(PORT);
  struct in_addr ia;
  ia.s_addr = inet_addr(ip == NULL ? IP : ip);
  sa.sin_addr = ia;

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) 
    handleError("socket");
  
  if (connect(sock, (struct sockaddr*) &sa, sizeof(sa)))
    handleError("connect");

  char request[256];
  char command[] = "getBlocksNewerThan#";
  int i = sizeof(command) - 1 ;
  memcpy(request+2, command, i);
  i += 2;
  //i++;
  // we have no blocks, so no hash to send
  if (lastBlock != NULL) {
    memcpy(request+i, lastBlock->hash, HASH_SIZE);
    i+=HASH_SIZE;
  }
  request[i++] = '$';
  //sprintf(request, ;

  //DEBUG
  request[i+1] = '\0';
  printf("sending: %s\n", request);
  //DEBUG end
  short int len = i - 2;
  memcpy(request, &len, 2);
  send(sock, request, i, 0);
  
  // recv loop
  // for now, stay in recv state forever
  while (1) {
    char buf[1024];

//  make sure to recv a whole chunk of parseable data
    if (recvUnit(sock, buf) == -1) {
      handleError("recvUnit");
    }

    //printf("recvd, now parsing\n");
    // parse block
    struct block *cur = calloc(1, sizeof(struct block));
    char *ptr = buf + sizeof(strtok(buf, "#")) -2;
    if (*ptr == '$') continue; // received command end signal ('$' right after '#')
    //printf("1 %s \n", ptr);
    //char *ptr = strtok(NULL, "#");
    //printf("ptr: %p\n", ptr);

    memcpy(cur->hash, ptr, HASH_SIZE);
    ptr+=HASH_SIZE+1;
    //ptr = strtok(NULL, ":");
    memcpy(cur->data, ptr, BLOCK_SIZE);
    ptr+=BLOCK_SIZE+1;
    //ptr = strtok(NULL, "$");
    memcpy(&cur->nonce, ptr, sizeof(((struct block*)0)->nonce));

    //printf("after parsing\n");

    // insert block into local blockchain
    if (lastBlock == NULL) {
      // local chain is empty
      firstBlock = cur;
      lastBlock = cur;
    } else {
      // first check if Hash is correct
      // NOTE: make this a separate function when it works
      //size_t len = HASH_SIZE+BLOCK_SIZE+sizeof(((struct block*)0)->nonce);
      char hashBlock[HASHBLOCK_SIZE];
      char calculatedHash[HASH_SIZE];
      memcpy(hashBlock, lastBlock->hash, HASH_SIZE);
      memcpy(hashBlock+HASH_SIZE, cur->data, BLOCK_SIZE);
      memcpy(hashBlock+HASH_SIZE+BLOCK_SIZE, &(cur->nonce), sizeof(((struct block*)0)->nonce));
      SHA256(hashBlock, HASHBLOCK_SIZE, calculatedHash);
      if (DEBUG) {
        if (memcmp(cur->hash, calculatedHash, HASH_SIZE)) {
          handleError("recvd Block: Hashes don't match!\n");
        } else {
          printf("recvd Block: Hashes match!\n");
        }
      }

      // append block to local chain
      cur->prevBlock = lastBlock;
      lastBlock->nextBlock = cur;
      lastBlock = cur;
    }

    /*
    int i;
    if (DEBUG) {
      for (i=0;i<512;i++) {
        printf("%02x", (unsigned char)cur->data[i]);
      }
      putchar('\n');
      for (i=0;i<SHA256_DIGEST_LENGTH;i++) {
        printf("%02x", (unsigned char)cur->hash[i]);
      }
      putchar('\n');
      printf("nonce: %d\n", cur->nonce);
    }
    //printf("buffer: %s\n", buf);
    */
    if (DEBUG) printf("received blocks\n");
    if (DEBUG) printBlocks(lastBlock);
  }

  while (1) {}
  close(sock);
}

int sendBlocks(struct block *start, int socket) {

  char buf[1024];
  struct block *current = start;
  do {  
    char com[] = "block#";
    short int i = sizeof(com) - 1;
    memcpy(buf+2, com, i);
    i += 2; // leave room for length variable at start of buffer
    memcpy(buf+i, current->hash, HASH_SIZE); 
    i+=HASH_SIZE;
    buf[i++] = ':';
    memcpy(buf+i, current->data, BLOCK_SIZE);
    i+=BLOCK_SIZE;
    buf[i++] = ':';
    memcpy(buf+i, &(current->nonce), sizeof(((struct block*)0)->nonce));
    i+=sizeof(((struct block*)0)->nonce);
    buf[i++] = '$';
    int len = i - 2;
    memcpy(buf, &len, 2); // copy length to the start of the buffer
    send(socket, buf, i, 0); // send length and data in buffer
    current = current->nextBlock;
  } while (current != NULL);

  // signal end of transmission
  char command2[] = "block#$";
  short int i = sizeof(command2) - 1;
  memcpy(buf+2, command2, i);
  memcpy(buf, &i, 2);
  send(socket, buf, i+2, 0);
}

void* handlePeer(void* a) {
  struct block *current = NULL;
  struct peerConnection *peer = a;
  printf("peer connection established\n");

  char buf[1024];
  recvUnit(peer->sock, buf);
  printf("received: %s\n", buf);
  
  char *command = strtok(buf, "#");
  char *arg = strtok(NULL, ":");

  if (!strcmp(command, "getBlocksNewerThan")) {
    printf("got command: getBlocksNewerThan\n");
    if (!strcmp(arg, "$")) {
      // send all the blocks, that is:
      // hash, data, nonce
      printf("got arg $\n");


      current = firstBlock;
      sendBlocks(current, peer->sock);
      // remember last sent block for this client
      // NOTE: this obviously creates a race condition that can lead to the peer missing blocks
      // but we don"t care for now
      current = lastBlock;

      
    } else {
      //send blocks newer than hash in arg
    } 
  }
  printf("sent blocks\n");  

  while(1) {
    // NOTE: again, creating race condition with main() thread here
    if (existsNewBlock) {
      // send it to the client peer
      if (current->nextBlock != NULL) {
        printf("before sending more\n");
        sendBlocks(current->nextBlock, peer->sock);
        current = lastBlock;
        printf("after sending more\n");
      } else {
        printf("NULL\n");
      }

      existsNewBlock = 0;
    }
  }

  close(peer->sock);
  free(peer);
  printf("connection closed, thread exiting\n");
} 

/* this thread controls stuff
   initially and if known peers stop responding, run a thread to connect to peers
   run a thread that listens for new connections and handle those
   */
void* network(void *arg) {
  if (DEBUG) {
    printf("this is me, your network thread\n");
  }

  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_port = port ? port : htons(PORT);
  struct in_addr ia;
  ia.s_addr = inet_addr(ip == NULL ? IP : ip);
  sa.sin_addr = ia;

  // tcp socket
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) 
    handleError("socket");
  
  if (bind(sock, (struct sockaddr*)&sa, sizeof(sa))) 
    handleError("bind");

  if (listen(sock, 5)) {
    char *err = strerror(errno);
    fprintf(stderr, "Error: %s\n", err);
    //handleError("listen");
    exit(1);
  }

  while (1) {
    struct peerConnection *peer = malloc(sizeof(struct peerConnection));
    memset(peer, '\0', sizeof(struct peerConnection));
    peer->sa_in_length = sizeof(peer);
    peer->sock = accept(sock, (struct sockaddr*) &peer->sa_in, &peer->sa_in_length);
    if (peer->sock == -1) handleError("accept");
    int err;
    pthread_t peerThread;
    if (err = pthread_create(&peerThread, NULL, &handlePeer, (void*) peer))
      handleError("pthread_create");
  }

//  if (ip == NULL) i
  // pthread_create(handleConsThread, NULL, &handleCons, NULL);
}


/* listen for connections, handle each connection in a separate thread */
void* handleCons(void *arg) {
}

int main(int argc, char **argv) {
  // parse command line options
  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, "csh")) != -1)
    switch (c) {
      case 'c':
        clientFlag = 1;
        break;
      case 's':
        serverFlag = 1;
        break; 
      case 'h':
        fprintf(stdout, "usage: %s -[c|s] \n", argv[0]);
        exit(0);
        break; 
      default:
        fprintf(stderr, "Error: unrecognized command line option\n");
        abort();
    }

  if (clientFlag && serverFlag) {
    fprintf(stderr, "c[lient] and s[erver] options can't be used together\n");
    abort();
  } else if (!clientFlag && !serverFlag) {
    serverFlag = 1;
  }

  // parse ip and port
  if (optind < argc) ip = argv[optind++];
  if (optind < argc) port = strtol(argv[optind], NULL, 10);

  // init random generator for nonces
  //NOTE: replace this with something better (/dev/urandom)
  srandom(time(NULL));

  int err;
  if (serverFlag) {
    pthread_t netThread;
    if (err = pthread_create(&netThread, NULL, &network, NULL))
      handleError("pthread_create(serverThread)");
  } else if (clientFlag) {
    pthread_t clientThread;
    if (err = pthread_create(&clientThread, NULL, &netClient, NULL))
      handleError("pthread_create(clientThread)");
  }

  if (serverFlag) {
    struct block first = { NULL, NULL, "", "hello world", 1337 };
    struct block current = { &first, NULL, "", "cracking it", 0 };
    // NOTE: don't link the block until its hash is solved
    //       instead publish the second block as a challenge to all clients
    //       and then start trying to solve it
    //first.nextBlock = &current;
    firstBlock = &first;
    lastBlock = &first;
    currentBlock = &current;

    if (DEBUG) {
      int i;
      for (i=0;i<512;i++) {
        printf("%02x", (unsigned char)current.data[i]);
      }
      putchar('\n');
      for (i=0;i<SHA256_DIGEST_LENGTH;i++) {
        printf("%02x", (unsigned char)current.hash[i]);
      }
      putchar('\n');
    }
  }

  if (serverFlag) {
    // publish block challenge to peers
    // -> we publish those when a client connects, no need to do anything here
  } else if (clientFlag) {
    // recv block challenge 
    //
  }

  // solving loop
  // simple for now: server solves task, publishes solved block, creates new task block; clients standby and receive new blocks
  if (serverFlag) {
    while (1) {
      solve(lastBlock, currentBlock, 20);
      
      // append solved block to blockchain
      lastBlock->nextBlock = currentBlock;
      currentBlock->prevBlock = lastBlock;
      lastBlock = currentBlock;

      //distribute finished block to peers
      // NOTE: creates Race Condition, use semaphore
      existsNewBlock = 1;

      // create new block
      currentBlock = calloc(1, sizeof(struct block));
      sprintf(currentBlock->data, "Data>RandomNumber:%ld", random());

    }
  }

  while (1) {}
}
