OBJS=http_parser.o http_server.o longpoll.o recv_all.o http_response.o
CC=clang
CFLAGS+=-O3 -Wall -Wextra -Werror -std=c99
LFLAGS+=-lev

longpoll: $(OBJS)
	$(CC) $(OBJS) $(LFLAGS) -o longpoll

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

# compile and generate dependency info
%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	@$(CC) -MM $(CFLAGS) $*.c > $*.d

clean:
	@rm *.o *.d longpoll
