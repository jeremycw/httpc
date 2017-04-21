OBJS=http_server.o longpoll.o http_response.o
CC=clang
CFLAGS+=-g -Wall -Wextra -Werror
LFLAGS+=-ldill

longpoll: $(OBJS)
	$(CC) $(OBJS) $(LFLAGS) -o longpoll

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

# compile and generate dependency info
%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	@$(CC) -MM $(CFLAGS) $*.c > $*.d

clean:
	@rm *.o *.d longpoll httpc-worker
