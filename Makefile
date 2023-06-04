all: pipe_server pipe_client1 pipe_client2

CC = gcc

pipe_server: pipe_server.c
	$(CC) -o $@ $< -pthread

pipe_client1: pipe_client1.c
	$(CC) -o $@ $< -pthread

pipe_client2: pipe_client2.c
	$(CC) -o $@ $< -pthread
