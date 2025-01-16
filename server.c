#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#pragma comment(lib, "Ws2_32.lib")
#define CLOSESOCKET closesocket
#define PORT 8080
#define BUFFER_SIZE 4096

struct Response {
    char httpType[64];
    char contentType[64];
    char contentLength[64];
    char connection[64];
    char body[256];
};

void to_string(struct Response* response, char* res) {
    strcpy(res, response->httpType);
    strcat(res, response->contentType);
    strcat(res, response->contentLength);
    strcat(res, response->connection);
    strcat(res, response->body);
}

void generate_http_response(const char* body, char* res) {
    struct Response responseStruct;

    sprintf(responseStruct.httpType, "HTTP/1.1 200 OK\r\n");
    sprintf(responseStruct.contentType, "Content-Type: text/html\r\n");
    sprintf(responseStruct.contentLength, "Content-Length: %zu\r\n", strlen(body));
    sprintf(responseStruct.connection, "Connection: close\r\n\r\n");
    strcpy(responseStruct.body, body);

    to_string(&responseStruct, res);
}

void handle_client(SOCKET clientSocket) {
    char* buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("Failed to allocate memory for buffer");
        CLOSESOCKET(clientSocket);
        return;
    }

    int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead == SOCKET_ERROR) {
        printf("recv failed: %d\n", WSAGetLastError());
        free(buffer);
        CLOSESOCKET(clientSocket);
        return;
    }

    buffer[bytesRead] = '\0'; // Null-terminate the buffer

    if (bytesRead > 0) {
        char* response = malloc(BUFFER_SIZE);
        if (!response) {
            perror("Failed to allocate memory for response");
            free(buffer);
            CLOSESOCKET(clientSocket);
            return;
        }

        response[0] = '\0'; // Initialize the string to be empty
        const char* body = "<html><body><h1>Hi Katie!</h1></body></html>";
        printf("Request received:\n%s\n", buffer);

        generate_http_response(body, response);
        printf("Response:\n%s\n", response);

        send(clientSocket, response, strlen(response), 0);

        free(response);
    } else {
        printf("recv failed or connection closed by client.\n");
    }

    free(buffer);
    CLOSESOCKET(clientSocket);
}

int main() {
    SOCKET server_socket;
    struct sockaddr_in server_addr, client_addr;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        CLOSESOCKET(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        CLOSESOCKET(server_socket);
        WSACleanup();
        return 1;
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        struct sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(server_socket, (struct sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSocket == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        handle_client(clientSocket);
    }

    CLOSESOCKET(server_socket);
    WSACleanup();
    return 0;
}
