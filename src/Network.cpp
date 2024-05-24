#include "Network.h"
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int setupServer(int port) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create server socket\n";
        return -1;
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind server socket\n";
        close(serverSocket);
        return -1;
    }

    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Failed to listen on server socket\n";
        close(serverSocket);
        return -1;
    }

    return serverSocket;
}

int connectToServer(const std::string &host, int port) {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Failed to create client socket\n";
        return -1;
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
        close(clientSocket);
        return -1;
    }

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection failed\n";
        close(clientSocket);
        return -1;
    }

    return clientSocket;
}

int sendMessage(int socket, const RaftMessage &message) {
    return write(socket, &message, sizeof(message)) == sizeof(message) ? 0 : -1;
}

int receiveMessage(int socket, RaftMessage &message) {
    return read(socket, &message, sizeof(message)) == sizeof(message) ? 0 : -1;
}
