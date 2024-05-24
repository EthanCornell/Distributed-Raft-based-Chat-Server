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

#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <string>
#include <queue>
#include "Log.h"
#include "Network.h"
#include "StateMachine.h" 
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <map>
#include <atomic>
#include <thread>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <functional>
#include <fstream>
#include <sstream> 

enum State {
    FOLLOWER,
    CANDIDATE,
    LEADER
};

struct RequestVoteResponse {
    int term;
    bool voteGranted;
};

struct RequestVoteArgs {
    int term;
    int candidateId;
    int lastLogIndex;
    int lastLogTerm;
    int socket;
};

struct AppendEntriesArgs {
    int term;
    int leaderId;
    int prevLogIndex;
    int prevLogTerm;
    std::vector<LogEntry> entries;
    int leaderCommit;
};

struct AppendEntriesAck {
    int term;
    int followerId;
    int commitIndex;
};

class Server {
public:
    std::atomic<bool> serverRunning{true};
    std::chrono::milliseconds heartbeatInterval{500};
    std::chrono::steady_clock::time_point lastHeartbeat;
    std::chrono::milliseconds electionTimeout{1500};
    
    Server(int id, const std::vector<std::pair<std::string, int>>& clusterConfig);
    ~Server(); // Ensure to clean up stateMachine if it's dynamically allocated

    void handleRequestVote(const RequestVoteArgs &args);
    void handleAppendEntries(const AppendEntriesArgs &args);
    void startElection();
    void appendLogEntry(const LogEntry &entry);
    State getState() const { return state; }
    int getId() const { return id; }

    void handleStartCommand(int n, int port);
    std::string getChatLog();
    void queueClientCommand(const std::string& command);
    void processClientCommands();
    void detectNetworkPartition();
    void persistClientMessage(const LogEntry& entry);
    void recoverUncommittedEntries();
    void handleWaitForAck(const std::string &msgId);

    void recoverNetwork();
    void periodicHealthCheck();
    void handleGetChatLog(int socket);
    // Proxy command handling
    void handleProxyCommand(int socket);
    void listenForProxyCommands(int proxyPort);  

    // Command handling methods
    void handleMessageCommand(const std::string &msgId, const std::string &message, int socket);
    void handleMsgCommand(const std::string& command, int socket);
    void handleWaitForAckCommand(const std::string& command);
    void handleCrashCommand();
    void handleExitCommand();
    void performFinalCleanup();
    void closeAllSockets();

private:
    std::vector<std::thread> activeThreads;
    std::mutex ackMutex;
    std::condition_variable ackCondVar;
    std::map<std::string, bool> messageAckStatus;
    std::vector<int> sockets;

    std::unordered_map<int, bool> nodeConnectivity;
    std::unordered_map<std::string, int> messageLogIndexMap;
    int serverCount; 
    int port; 
    int id;
    State state{FOLLOWER};
    int currentTerm{0};
    int votedFor{-1};
    int leaderId{-1};  // Default to -1 indicating no leader is known
    Log log;
    StateMachine *stateMachine{nullptr};
    int commitIndex{0};
    int lastApplied{0};
    std::vector<std::pair<std::string, int>> clusterConfig;
    std::queue<std::string> commandQueue;


    void heartbeat();
    void monitorLeader();
    void sendRequestVoteRPC();
    void sendAppendEntriesRPC();
    void sendRequestVote(int socket, const RequestVoteArgs &args);
    void sendAppendEntries(int socket, const AppendEntriesArgs &args);
    void applyCommittedEntries();
    void updateState(State newState);
    void sendAppendEntriesAck(int leaderId);
    bool receiveAppendEntriesAck(int socket);
    void handleClientMessage(const std::string &msg);
    void processClientMessage(const std::string &message, int clientSocket);
    void handleClientConnection(int clientSocket);

};

#endif // SERVER_H
