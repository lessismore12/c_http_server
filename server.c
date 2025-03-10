#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define CLOSESOCKET closesocket
    #define Socket SOCKET
    #pragma comment(lib, "Ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define Socket int
    #define CLOSESOCKET close
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>


#define PORT 8080
#define BUFFER_SIZE 4096

struct Response {
    char http_type[64];
    char content_type[64];
    char content_length[64];
    char connection[64];
    char body[256];
};

void to_string(struct Response* response, char* res) {
    strcpy(res, response->http_type);
    strcat(res, response->content_type);
    strcat(res, response->content_length);
    strcat(res, response->connection);
    strcat(res, response->body);
}

void generate_http_response(const char* body, char* res) {
    struct Response responseStruct;

    sprintf(responseStruct.http_type, "HTTP/1.1 200 OK\r\n");
    sprintf(responseStruct.content_type, "Content-Type: text/html\r\n");
    sprintf(responseStruct.content_length, "Content-Length: %zu\r\n", strlen(body));
    sprintf(responseStruct.connection, "Connection: close\r\n\r\n");
    strcpy(responseStruct.body, body);

    to_string(&responseStruct, res);
}

void* handle_client(void* arg) {
    Socket clientSocket = *(Socket*) arg;
    free(arg);
#ifdef _WIN32
    printf("Handling client in thread ID: %lu\n", GetCurrentThreadId());
#else
    printf("Handling client in thread ID: %lu\n", pthread_self());
#endif

    char* buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("Failed to allocate memory for buffer");
        CLOSESOCKET(clientSocket);
    }

/*     if (strncmp(buffer, "GET /favicon.ico", 16) == 0) {
        printf("Ignoring favicon request.\n");
        free(buffer);
        CLOSESOCKET(clientSocket);
        return;
    } */

    int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    free(buffer);
    if (bytesRead == -1) {
#ifdef _WIN32
        printf("Socket creation failed: %d\n", WSAGetLastError());
        printf("recv failed: %d\n", WSAGetLastError());
        WSACleanup();
#else 
        printf("Socket creation failed\n");
        printf("recv failed\n");
#endif
        CLOSESOCKET(clientSocket);
    }

    buffer[bytesRead] = '\0'; // Null-terminate the buffer

    printf("Received request:\n%s\n", buffer);

    if (bytesRead > 0) {
        char* response = malloc(BUFFER_SIZE);
        if (!response) {
            printf("Failed to allocate memory for response");
            free(buffer);
            CLOSESOCKET(clientSocket);
        }

        response[0] = '\0'; // Initialize the string to be empty
        const char* body = "<html><body><h1>You're  doing great! Keep it up</h1></body></html>";
        //printf("Request received:\n%s\n", buffer);

        generate_http_response(body, response);
        //printf("Response:\n%s\n", response);

        send(clientSocket, response, strlen(response), 0);

        free(response);
    } else {
        printf("recv failed or connection closed by client.\n");
    }

    free(buffer);
    CLOSESOCKET(clientSocket);
}

int main() 
{
    Socket server_socket;
    struct sockaddr_in server_addr, client_addr;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
#endif

    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == -1) {
#ifdef _WIN32
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
#else 
        printf("setsockopt(SO_REUSEADDR) failed\n");
#endif
        CLOSESOCKET(server_socket);
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
#ifdef _WIN32
        printf("Bind failed: %d\n", WSAGetLastError());
        WSACleanup();
#else
        printf("Bind failed\n");
        CLOSESOCKET(server_socket);
#endif
        return 1;
    }

    if (listen(server_socket, SOMAXCONN) < 0) {
#ifdef _WIN32
        printf("Listen failed: %d\n", WSAGetLastError());
        WSACleanup();
#else
        printf("Listen failed\n");
        CLOSESOCKET(server_socket);
#endif
        return 1;
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        struct sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        Socket clientSocket = accept(server_socket, (struct sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSocket == -1) {
#ifdef _WIN32
            printf("Accept failed: %d\n", WSAGetLastError());
#else
            printf("Accept failed\n");
#endif
            continue;
        }

        pthread_t thread1;
        Socket* clientSocketPtr = malloc(sizeof(Socket));
        if (!clientSocketPtr) {
            perror("Failed to allocate memory for client socket");
            continue;
        }
        *clientSocketPtr = clientSocket;

        if (pthread_create(&thread1, NULL, handle_client, clientSocketPtr) != 0) {
            perror("Failed to create thread 1");
            return 1;
        }
    }

    CLOSESOCKET(server_socket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
