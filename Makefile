blockchain: bc.o
	gcc -o blockchain bc.o -lpthread -lcrypto

bc.o: bc.c
	gcc -c bc.c
