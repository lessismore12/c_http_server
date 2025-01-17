libs = -lwsock32

cpp: main.cpp
	g++ main.cpp -o main $(libs)
	./main

windows: server.c
	gcc server.c -o server $(libs)
	./server

linux: server.c
	gcc -o linux_server server.c
	./linux_server
