CC=gcc
CFLAGS=-std=c99 -lm -Wall -g -lpthread -w

compressRLE:
	$(CC) -o asst2 compressRLE.c $(CFLAGS)

compressT_LOLS:
	$(CC) -o asst2 compressT_LOLS.c lols.c $(CFLAGS)

compressR_LOLS:
	$(CC) -o asst2 compressR_LOLS.c compressR_worker_LOLS.c lols.c utils.c $(CFLAGS)

clean:
	rm -rf asst2
