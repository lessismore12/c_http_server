libs = -lwsock32 -lpthread

# Default target
all: windows

windows: server.c
	gcc server.c -o windows_server $(libs)
	./windows_server

linux: server.c
	gcc -o linux_server server.c
	./linux_server
