# file: Makefile
CC=gcc
CFLAGS=-Wall -Wextra -g

COMMON_OBJS=src/common/utils.o src/common/models.o src/common/db.o src/common/protocol.o
SERVER_OBJS=src/server/main_server.o src/server/server.o src/server/handlers/handler_auth.o src/server/handlers/handler_movie.o src/server/handlers/handler_show.o src/server/handlers/handle_admin.o
CLIENT_OBJS=src/client/main_client.o src/client/client.o

all: server client

server: $(COMMON_OBJS) $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o build/server $(COMMON_OBJS) $(SERVER_OBJS)

client: $(COMMON_OBJS) $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o build/client $(COMMON_OBJS) $(CLIENT_OBJS)

clean:
	rm -f $(COMMON_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS) build/server build/client

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
