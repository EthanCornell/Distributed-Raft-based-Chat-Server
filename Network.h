#ifndef NETWORK_H
#define NETWORK_H

#include <string>

enum MessageType {
    REQUEST_VOTE,
    APPEND_ENTRIES
};

// Base struct for all Raft messages
struct RaftMessage {
    MessageType type;
    int term;
    int senderId;
};

// RequestVote message
struct RequestVoteMessage : RaftMessage {
    int lastLogIndex;
    int lastLogTerm;
};

// AppendEntries message
struct AppendEntriesMessage : RaftMessage {
    int prevLogIndex;
    int prevLogTerm;
    std::string entries; // For simplicity, serialized log entries
    int leaderCommit;
};

// Network functions
int sendMessage(int socket, const RaftMessage &message);
int receiveMessage(int socket, RaftMessage &message);
int setupServer(int port);
int connectToServer(const std::string &host, int port);

#endif // NETWORK_H
