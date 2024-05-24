/*
 * Copyright (c) Cornell University.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: I-Hsuan (Ethan) Huang
 * Email: ih246@cornell.edu
 * Project: Distributed-Raft-based-Chat-Server
 */

#include "Server.h"
#include <iostream>
#include <vector>
#include <utility>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>

// Function to handle commands from the proxy
void handleProxyCommands(Server &server, int port) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create server socket." << std::endl;
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind server socket." << std::endl;
        close(serverSocket);
        return;
    }

    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Failed to listen on server socket." << std::endl;
        close(serverSocket);
        return;
    }

    std::cout << "Server listening on port " << port << std::endl;

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }

        server.handleProxyCommand(clientSocket);

        close(clientSocket);
    }

    close(serverSocket);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <server_id> <server_count> <port>" << std::endl;
        return 1;
    }

    int id = std::stoi(argv[1]);
    int n = std::stoi(argv[2]);
    int port = std::stoi(argv[3]);

    std::vector<std::pair<std::string, int>> clusterConfig;
    for (int i = 0; i < n; i++) {
        clusterConfig.push_back(std::make_pair("127.0.0.1", 20000 + i));
    }

    Server server(id, clusterConfig);

    std::thread proxyThread(handleProxyCommands, std::ref(server), port);
    proxyThread.join();

    return 0;
}
