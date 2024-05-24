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
