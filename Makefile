OBJS=http_server.o longpoll.o http_response.o async.o
CC=clang
CFLAGS+=-O3 -Wall -Wextra -Werror
LFLAGS+=-ldill

longpoll: $(OBJS) httpc-worker
	$(CC) $(OBJS) $(LFLAGS) -o longpoll

httpc-worker: worker.c
	$(CC) $(CFLAGS) worker.c $(LFLAGS) -o httpc-worker

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

# compile and generate dependency info
%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	@$(CC) -MM $(CFLAGS) $*.c > $*.d

clean:
	@rm *.o *.d longpoll httpc-worker
