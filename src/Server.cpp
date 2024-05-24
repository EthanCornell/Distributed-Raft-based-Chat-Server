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


void Server::heartbeat() {
    if (state != LEADER) return;

    // Send empty AppendEntries to followers as a heartbeat
    AppendEntriesArgs args = {currentTerm, id, log.getLastIndex(), log.getEntry(log.getLastIndex()).term, {}, commitIndex};
    sendAppendEntriesRPC();
}

void Server::monitorLeader() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeatInterval));
        if (state == FOLLOWER && (lastHeartbeat < std::chrono::steady_clock::now() - electionTimeout)) {
            startElection(); // Trigger a new election if heartbeat timeout
        }
        heartbeat();
    }
}


// Initialize node connectivity status
Server::Server(int id, const std::vector<std::pair<std::string, int>>& clusterConfig)
    : id(id), state(FOLLOWER), currentTerm(0), votedFor(-1),
      commitIndex(0), lastApplied(0), clusterConfig(clusterConfig) {
    stateMachine = new StateMachine();
    for (const auto& node : clusterConfig) {
        nodeConnectivity[node.second] = true; // Assume all nodes are initially connected
    }
    recoverUncommittedEntries();
    std::thread(&Server::monitorLeader, this).detach();
}

int sendRequestVoteResponse(int socket, const RequestVoteResponse &response) {
    // Serializing and sending the response
    int bytesSent = send(socket, &response, sizeof(response), 0);
    return (bytesSent == sizeof(response)) ? 0 : -1;
}

void Server::handleRequestVote(const RequestVoteArgs &args) {
    if (args.term > currentTerm) {
        currentTerm = args.term;
        state = FOLLOWER;
        votedFor = -1;
    }

    bool voteGranted = false;

    if (args.term == currentTerm &&
        (votedFor == -1 || votedFor == args.candidateId) &&
        (args.lastLogTerm > log.getLastTerm() || 
         (args.lastLogTerm == log.getLastTerm() && 
          args.lastLogIndex >= log.getLastIndex()))) {
        votedFor = args.candidateId;
        voteGranted = true;
    }

    // Prepare the response
    RequestVoteResponse response = { currentTerm, voteGranted };

    // Assuming socket is provided via `args`
    int socket = args.socket; // Assuming `socket` is an additional attribute in `RequestVoteArgs`
    
    if (socket >= 0) {
        if (sendRequestVoteResponse(socket, response) < 0) {
            std::cerr << "Failed to send RequestVote response\n";
        }
        close(socket); // Closing the socket after sending the response
    }

    std::cout << "Vote granted: " << voteGranted << std::endl;
}


void Server::sendAppendEntriesAck(int leaderId) {
    AppendEntriesAck ack = {currentTerm, id, commitIndex};
    int socket = connectToServer(clusterConfig[leaderId].first, clusterConfig[leaderId].second);
    if (socket >= 0) {
        int32_t term = htonl(ack.term);
        int32_t followerId = htonl(ack.followerId);
        int32_t commitIndex = htonl(ack.commitIndex);

        if (send(socket, &term, sizeof(term), 0) != sizeof(term) ||
            send(socket, &followerId, sizeof(followerId), 0) != sizeof(followerId) ||
            send(socket, &commitIndex, sizeof(commitIndex), 0) != sizeof(commitIndex)) {
            std::cerr << "Failed to send AppendEntriesAck message\n";
        }
        close(socket);
    }
}


void Server::applyCommittedEntries() {
    while (lastApplied < commitIndex) {
        lastApplied++;
        stateMachine->applyLogEntry(log.getEntry(lastApplied));

        // Acknowledge the client
        std::cout << "Commit success: " << log.getEntry(lastApplied).command << std::endl;
    }
}

void Server::handleAppendEntries(const AppendEntriesArgs &args) {
    // Reject AppendEntries if term is outdated
    if (args.term < currentTerm) {
        // Optionally send a negative response to the leader
        return;
    }

    // Update term and switch to follower if current term is outdated
    if (args.term > currentTerm) {
        currentTerm = args.term;
        state = FOLLOWER;
        votedFor = -1;
    }

    // Check if the log matches the previous log entry
    if (args.prevLogIndex >= 0 && log.getEntry(args.prevLogIndex).term != args.prevLogTerm) {
        // Optionally send a negative response due to inconsistency
        return;
    }

    // Append new entries starting from prevLogIndex
    for (int i = 0; i < static_cast<int>(args.entries.size()); ++i) {
        const LogEntry &entry = args.entries[i];
        // Avoid duplicate entries
        if (log.getLastIndex() < entry.index) {
            log.addEntry(entry);
        }
    }

    // Update commit index and apply committed entries
    if (args.leaderCommit > commitIndex) {
        commitIndex = std::min(args.leaderCommit, log.getLastIndex());
        applyCommittedEntries();
    }

    // Send a positive acknowledgment to the leader
    sendAppendEntriesAck(args.leaderId);
}

void Server::sendRequestVoteRPC() {
    // RequestVoteArgs args = { currentTerm, id, log.getLastIndex(), log.getLastTerm() };
    RequestVoteArgs args = {currentTerm, id, log.getLastIndex(), log.getLastTerm(), -1}; // Initialize socket with -1


    for (const auto& server : clusterConfig) {
        int socket = connectToServer(server.first, server.second);
        if (socket < 0) {
            std::cerr << "Failed to connect to server " << server.first << ":" << server.second << std::endl;
            continue;
        }

        sendRequestVote(socket, args);
        close(socket);
    }
}


void Server::detectNetworkPartition() {
    for (const auto& node : clusterConfig) {
        int socket = connectToServer(node.first, node.second);
        if (socket < 0) {
            nodeConnectivity[node.second] = false; // Mark as disconnected
        } else {
            nodeConnectivity[node.second] = true; // Mark as connected
            close(socket);
        }
    }

    // Check if in a minority partition
    int connectedCount = std::count_if(nodeConnectivity.begin(), nodeConnectivity.end(),
                                       [](const auto& p) { return p.second; });
    if (connectedCount <= static_cast<int>(clusterConfig.size()) / 2) { // Explicitly cast to int
        std::cerr << "In a minority partition. Not eligible to become leader." << std::endl;
        votedFor = -1; // Revoke self-vote
    }
}

void Server::startElection() {
    // Check if the server is in a minority partition
    detectNetworkPartition();

    // Only proceed if not in a minority partition
    if (votedFor == -1) {
        std::cerr << "Cannot start election: in a minority partition." << std::endl;
        return;
    }

    // Proceed with the election
    state = CANDIDATE;
    currentTerm++;
    votedFor = id;

    sendRequestVoteRPC();
}



void Server::recoverNetwork() {
    // Check network connectivity
    detectNetworkPartition();

    // Adjust server state accordingly
    if (votedFor == -1) {
        std::cerr << "Network recovered but still in minority partition." << std::endl;
        return;
    }
    
    // Additional recovery actions if the server is not in a minority partition
    // Example actions:
    // - Trigger leader election if necessary
    // - Resume normal operations if was previously paused

    std::cout << "Network recovered and server is in majority partition." << std::endl;
}

void Server::periodicHealthCheck() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // Adjust interval as needed

        detectNetworkPartition();

        // Adjust server status or take other actions if needed
        if (votedFor == -1) {
            std::cerr << "Periodic check: in a minority partition." << std::endl;
        } else {
            // Additional actions to take if in the majority partition
            // Example actions:
            // - Send periodic heartbeats if leader
            // - Check for missed AppendEntries RPCs
            // - Initiate leader election if timeout detected
            std::cout << "Periodic check: in majority partition." << std::endl;
        }
    }
}

void Server::sendAppendEntriesRPC() {
    AppendEntriesArgs args = { currentTerm, id, log.getLastIndex() - 1, log.getEntry(log.getLastIndex() - 1).term, {}, commitIndex };

    for (const auto& server : clusterConfig) {
        int socket = connectToServer(server.first, server.second);
        if (socket < 0) {
            std::cerr << "Failed to connect to server " << server.first << ":" << server.second << std::endl;
            continue;
        }

        sendAppendEntries(socket, args);
        close(socket);
    }
}

void Server::sendRequestVote(int socket, const RequestVoteArgs &args) {
    // Convert integers to network byte order
    int32_t term = htonl(args.term);
    int32_t candidateId = htonl(args.candidateId);
    int32_t lastLogIndex = htonl(args.lastLogIndex);
    int32_t lastLogTerm = htonl(args.lastLogTerm);

    // Send each field sequentially
    if (send(socket, &term, sizeof(term), 0) != sizeof(term) ||
        send(socket, &candidateId, sizeof(candidateId), 0) != sizeof(candidateId) ||
        send(socket, &lastLogIndex, sizeof(lastLogIndex), 0) != sizeof(lastLogIndex) ||
        send(socket, &lastLogTerm, sizeof(lastLogTerm), 0) != sizeof(lastLogTerm)) {
        std::cerr << "Failed to send RequestVote message\n";
    }
}

void Server::sendAppendEntries(int socket, const AppendEntriesArgs &args) {
    // Convert integers to network byte order
    int32_t term = htonl(args.term);
    int32_t leaderId = htonl(args.leaderId);
    int32_t prevLogIndex = htonl(args.prevLogIndex);
    int32_t prevLogTerm = htonl(args.prevLogTerm);
    int32_t leaderCommit = htonl(args.leaderCommit);

    // Send each field sequentially
    if (send(socket, &term, sizeof(term), 0) != sizeof(term) ||
        send(socket, &leaderId, sizeof(leaderId), 0) != sizeof(leaderId) ||
        send(socket, &prevLogIndex, sizeof(prevLogIndex), 0) != sizeof(prevLogIndex) ||
        send(socket, &prevLogTerm, sizeof(prevLogTerm), 0) != sizeof(prevLogTerm) ||
        send(socket, &leaderCommit, sizeof(leaderCommit), 0) != sizeof(leaderCommit)) {
        std::cerr << "Failed to send AppendEntries message\n";
        return;
    }

    // Serialize and send the log entries
    for (const auto &entry : args.entries) {
        int32_t entryTerm = htonl(entry.term);
        int32_t entryIndex = htonl(entry.index);

        // Send the entry term and index
        if (send(socket, &entryTerm, sizeof(entryTerm), 0) != sizeof(entryTerm) ||
            send(socket, &entryIndex, sizeof(entryIndex), 0) != sizeof(entryIndex)) {
            std::cerr << "Failed to send log entry metadata\n";
            return;
        }

        // Send the entry command length and data
        uint32_t commandLength = htonl(entry.command.size());
        if (send(socket, &commandLength, sizeof(commandLength), 0) != sizeof(commandLength) ||
            send(socket, entry.command.c_str(), entry.command.size(), 0) != static_cast<ssize_t>(entry.command.size())) {
            std::cerr << "Failed to send log entry command\n";
        }
    }
}





void Server::updateState(State newState) {
    // Transition to the new state
    state = newState;
    if (newState == FOLLOWER) {
        votedFor = -1; // Reset votedFor when becoming a follower
    }
}

bool Server::receiveAppendEntriesAck(int socket) {
    int32_t term, followerId, commitIndex;

    // Read each field separately
    if (recv(socket, &term, sizeof(term), 0) != sizeof(term) ||
        recv(socket, &followerId, sizeof(followerId), 0) != sizeof(followerId) ||
        recv(socket, &commitIndex, sizeof(commitIndex), 0) != sizeof(commitIndex)) {
        std::cerr << "Failed to receive AppendEntriesAck message\n";
        return false;
    }

    // Deserialize the values
    term = ntohl(term);
    followerId = ntohl(followerId);
    commitIndex = ntohl(commitIndex);

    // Further processing based on the deserialized values (if required)
    
    return true;
}

std::string Server::getChatLog() {
    // Return chat log as a comma-separated string
    std::string chatLog;
    for (int i = 1; i <= log.getLastIndex(); ++i) {
        chatLog += log.getEntry(i).command + ",";
    }
    return chatLog;
}

void Server::handleClientMessage(const std::string &msg) {
    // Create log entry from the message and append it
    LogEntry entry = {currentTerm, msg, log.getLastIndex() + 1};
    appendLogEntry(entry);
}

void Server::processClientMessage(const std::string &message, int clientSocket) {
    if (state != LEADER) {
        // Reject message if not leader
        std::string errorMsg = "error: server not leader\n";
        ssize_t res = write(clientSocket, errorMsg.c_str(), errorMsg.size());
        if (res <= 0) {
            std::cerr << "Failed to send error message to client\n";
        }
        return;
    }

    // Handle 'msg' command
    if (message.substr(0, 4) == "msg ") {
        std::string chatMessage = message.substr(4);
        LogEntry entry = {currentTerm, chatMessage, log.getLastIndex() + 1};
        appendLogEntry(entry);

        // Acknowledge the client
        std::string ack = "ack " + std::to_string(entry.index) + "\n";
        ssize_t res = write(clientSocket, ack.c_str(), ack.size());
        if (res <= 0) {
            std::cerr << "Failed to send acknowledgment to client\n";
        }

        // Propagate to followers
        sendAppendEntriesRPC();
    } else if (message == "get chatLog\n") {
        std::string chatLog = getChatLog();
        chatLog += "\n";
        ssize_t res = write(clientSocket, chatLog.c_str(), chatLog.size());
        if (res <= 0) {
            std::cerr << "Failed to send chat log to client\n";
        }
    } else {
        std::string errorMsg = "error: unknown command\n";
        ssize_t res = write(clientSocket, errorMsg.c_str(), errorMsg.size());
        if (res <= 0) {
            std::cerr << "Failed to send error message to client\n";
        }
    }
}


void Server::handleClientConnection(int clientSocket) {
    // Placeholder for handling client connections and messages
    char buffer[1024];
    while (true) {
        int bytesRead = read(clientSocket, buffer, sizeof(buffer));
        if (bytesRead <= 0) {
            close(clientSocket);
            break;
        }

        buffer[bytesRead] = '\0';
        std::string message(buffer);

        // Process the message
        processClientMessage(message, clientSocket);
    }
}

void Server::handleStartCommand(int n, int port) {
    // Initialize the server
    this->serverCount = n;
    this->port = port;

    // Set up the server socket
    int serverSocket = setupServer(port);
    if (serverSocket < 0) {
        std::cerr << "Failed to start server on port " << port << std::endl;
        return;
    }

    std::cout << "Server started on port " << port << std::endl;

    // Start a thread to handle incoming connections
    std::thread([this, serverSocket]() {
        while (true) {
            sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
            if (clientSocket < 0) {
                std::cerr << "Failed to accept connection" << std::endl;
                continue;
            }

            // Handle the incoming connection in a separate thread
            std::thread(&Server::handleClientConnection, this, clientSocket).detach();
        }
    }).detach();

    // Start the health check thread
    std::thread(&Server::periodicHealthCheck, this).detach();
}

void Server::processClientCommands() {
    while (!commandQueue.empty()) {
        std::string command = commandQueue.front();
        commandQueue.pop();
        // Process each command
    }
}

void Server::queueClientCommand(const std::string& command) {
    // Queue incoming client commands
    commandQueue.push(command);
    processClientCommands();
}


void Server::persistClientMessage(const LogEntry& entry) {
    std::ofstream logFile("raft_log.txt", std::ios::app);
    if (logFile) {
        logFile << entry.term << " " << entry.index << " " << entry.command << "\n";
        logFile.close();
    }
}

void Server::recoverUncommittedEntries() {
    std::ifstream logFile("raft_log.txt");
    std::string line;
    while (std::getline(logFile, line)) {
        std::istringstream iss(line);
        int term, index;
        std::string command;
        if (!(iss >> term >> index >> command)) { break; }
        LogEntry entry = {term, command, index};
        log.addEntry(entry);
    }
}

void Server::appendLogEntry(const LogEntry& entry) {
    log.addEntry(entry);
    persistClientMessage(entry); // Persist before sending to followers
    sendAppendEntriesRPC();
}

void Server::handleGetChatLog(int socket) {
    std::string chatLog = getChatLog();
    chatLog += "\n";
    ssize_t res = write(socket, chatLog.c_str(), chatLog.size());
    if (res < 0) {
        std::cerr << "Failed to send chat log to proxy\n";
    }
}

void Server::handleWaitForAck(const std::string &msgId) {
    if (messageLogIndexMap.find(msgId) == messageLogIndexMap.end()) {
        std::cerr << "Message ID not found: " << msgId << std::endl;
        return;
    }

    int logIndex = messageLogIndexMap[msgId];
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        // Check if the message has been committed
        if (commitIndex >= logIndex) {
            std::cout << "Message " << msgId << " acknowledged.\n";
            break;
        }

        // Check for timeout
        auto elapsedTime = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsedTime).count() > 5) { // Timeout after 5 seconds
            std::cout << "Resending message " << msgId << " due to timeout.\n";

            // Resend the message
            LogEntry entry = log.getEntry(logIndex);
            appendLogEntry(entry);

            // Reset start time
            startTime = std::chrono::steady_clock::now();
        }

        // Sleep for a while to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Server::handleMessageCommand(const std::string &msgId, const std::string &message, int socket) {
    // Implement the logic to handle the message command.
    // This may include logging the message, updating state, etc.
    
    // Example implementation:
    LogEntry entry = {currentTerm, message, log.getLastIndex() + 1};
    appendLogEntry(entry);

    // Acknowledge the client
    std::string ack = "ack " + msgId + " " + std::to_string(entry.index) + "\n";
    ssize_t res = write(socket, ack.c_str(), ack.size());
    if (res <= 0) {
        std::cerr << "Failed to send acknowledgment to client\n";
    }
}

void Server::handleMsgCommand(const std::string& command, int socket) {
    std::istringstream iss(command);
    std::string cmd, msgId, messageContent;

    // Skipping the first token "msg" which is the command itself
    iss >> cmd >> msgId;

    // Extract the rest of the message after msgId
    getline(iss, messageContent);
    if (!messageContent.empty() && messageContent[0] == ' ') {
        messageContent = messageContent.substr(1); // Remove leading space
    }

    // Check if this server is the leader
    if (state == LEADER) {
        // As the leader, append the message to the log
        LogEntry newEntry = {currentTerm, messageContent, log.getLastIndex() + 1};
        log.addEntry(newEntry);

        // Send AppendEntries RPCs to all followers
        sendAppendEntriesRPC();

        // Build and send acknowledgment back to the proxy
        std::string ack = "ack " + msgId + " " + std::to_string(newEntry.index) + "\n";
        if (write(socket, ack.c_str(), ack.length()) != static_cast<ssize_t>(ack.length())) {
            std::cerr << "Failed to send acknowledgment to proxy." << std::endl;
        }
    } else if (leaderId >= 0) {
        // Not the leader, forward the message to the leader
        // Connect to the leader's server
        int leaderSocket = connectToServer(clusterConfig[leaderId].first, clusterConfig[leaderId].second);
        if (leaderSocket < 0) {
            std::cerr << "Error connecting to leader to forward message." << std::endl;
        } else {
            std::string forwardCmd = "msg " + msgId + " " + messageContent;
            if (write(leaderSocket, forwardCmd.c_str(), forwardCmd.length()) != static_cast<ssize_t>(forwardCmd.length())) {
                std::cerr << "Failed to forward message to leader." << std::endl;
            }
            close(leaderSocket);
        }
    } else {
        // In case there's no known leader, send an error ack
        std::string errorAck = "ack " + msgId + " -1\n";
        if (write(socket, errorAck.c_str(), errorAck.length()) != static_cast<ssize_t>(errorAck.length())) {
            std::cerr << "Failed to send error acknowledgment to proxy." << std::endl;
        }
        std::string errorMsg = "No leader available.\n";
        send(socket, errorMsg.c_str(), errorMsg.size(), 0);
    }
}

void Server::handleWaitForAckCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd, msgId;

    // Extract the message ID from the command
    iss >> cmd >> msgId;

    std::unique_lock<std::mutex> lock(ackMutex);
    if (messageAckStatus.find(msgId) == messageAckStatus.end()) {
        // Initialize the ack status as false
        messageAckStatus[msgId] = false;
    }

    // Wait for the acknowledgment to become true or for a timeout
    if (!ackCondVar.wait_for(lock, std::chrono::seconds(10), [&]{ return messageAckStatus[msgId]; })) {
        std::cerr << "Timeout waiting for acknowledgment for message ID: " << msgId << std::endl;
        // Handle the timeout scenario, such as retrying the message or reporting a failure
    } else {
        std::cout << "Acknowledgment received for message ID: " << msgId << std::endl;
        // Acknowledgment was received, proceed with whatever the next step is
    }

    // Clean up the ack status map
    messageAckStatus.erase(msgId);
}


void Server::closeAllSockets() {
    // Example of closing all sockets; you would have actual sockets in your server to close
    for (int sock : sockets) {
        close(sock);
    }
}

void Server::handleCrashCommand() {
    std::cout << "Simulating server crash..." << std::endl;

    // Set a global or shared atomic flag to signal all threads and operations to stop
    serverRunning.store(false);

    // Close all open network sockets
    closeAllSockets();

    // Wait for all threads to finish
    for (auto& th : activeThreads) {
        if (th.joinable()) {
            th.join();
        }
    }

    // Optionally, simulate immediate stop without cleanup
    std::exit(EXIT_FAILURE);  // Use std::exit to simulate an abrupt crash

    // Note: In a real implementation, you might want to handle persistent state saving before crashing
    // and ensure that you can recover adequately in a restart.
}

void Server::performFinalCleanup() {
    // Placeholder for any cleanup operations like closing file handles, writing final log entries, etc.
    std::cout << "Performing final cleanup operations." << std::endl;
    // Additional cleanup logic here
}

void Server::handleExitCommand() {
    std::cout << "Server is shutting down..." << std::endl;

    // Signal all parts of the application that the server is shutting down
    serverRunning.store(false);

    // Close all network sockets to stop receiving new requests
    closeAllSockets(); 

    // Wait for all server threads to finish
    for (auto& th : activeThreads) {
        if (th.joinable()) {
            th.join();
        }
    }

    // Perform any additional cleanup like flushing logs, saving state, etc.
    performFinalCleanup();

    // Exit the program
    std::exit(EXIT_SUCCESS);  // Use EXIT_SUCCESS to indicate a graceful shutdown
}


void Server::handleProxyCommand(int socket) {
    char buffer[1024];
    int bytesRead = read(socket, buffer, sizeof(buffer)-1);
    buffer[bytesRead] = '\0'; // Ensure null-termination
    std::string command(buffer);

    if (command.substr(0, 3) == "msg") {
        handleMsgCommand(command, socket);
    } else if (command.substr(0, 10) == "get chatLog") {
        handleGetChatLog(socket);
    } else if (command.substr(0, 10) == "waitForAck") {
        handleWaitForAckCommand(command);
    } else if (command == "crash") {
        handleCrashCommand();
    } else if (command == "exit") {
        handleExitCommand();
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
    }
    close(socket);
}



void Server::listenForProxyCommands(int proxyPort) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(proxyPort);

    bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 5);  // Listen on the socket for incoming connections.

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }
        std::thread(&Server::handleProxyCommand, this, clientSocket).detach();
    }
}


Server::~Server() {
    delete stateMachine;  // assuming stateMachine is dynamically allocated
}

