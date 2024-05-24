
# Distributed Raft-based Chat Server

## Overview
This project implements a distributed chat server using the Raft consensus algorithm to manage replicated logs and state machines across multiple nodes. The server handles client messages, applies them to a state machine, and maintains consistency across the distributed cluster.

## Features
- Raft-based consensus for leader election and log replication
- Handling client commands for messages and state queries
- Network setup and communication using sockets
- Logging and state management

## Directory Structure
```
.
├── src
│   ├── main.cpp
│   ├── Server.cpp
│   ├── Server.h
│   ├── Network.cpp
│   ├── Network.h
│   ├── StateMachine.cpp
│   ├── StateMachine.h
│   ├── Log.cpp
│   ├── Log.h
├── Makefile
└── README.md
```

## Prerequisites
- C++11 or higher
- CMake
- POSIX-compatible environment (Linux, macOS)

## Build Instructions
1. Clone the repository:
    ```sh
    git clone https://github.com/EthanCornell/Distributed-Raft-based-Chat-Server.git
    cd Distributed-Raft-based-Chat-Server
    ```

2. Build the project using the Makefile:
    ```sh
    make
    ```

## Run the Server
To run a server instance, use the following command:
```sh
./server <server_id> <server_count> <port>
```
- `server_id`: Unique identifier for the server instance.
- `server_count`: Total number of server instances in the cluster.
- `port`: Port number for the server to listen on.

For example:
```sh
./server 1 3 20001
```

## File Descriptions
- **main.cpp**: Entry point for the server application, sets up the server and starts handling commands.
- **Server.h & Server.cpp**: Core implementation of the server, including handling Raft consensus operations, client commands, and network communication.
- **Network.h & Network.cpp**: Functions for setting up the network, sending, and receiving messages.
- **StateMachine.h & StateMachine.cpp**: Simple key-value store state machine to apply log entries.
- **Log.h & Log.cpp**: Implementation of the log structure to store and manage log entries.

## Raft Implementation Details
- **Leader Election**: Follower nodes timeout and become candidates to start a new election term, soliciting votes from other nodes.
- **Log Replication**: The leader node replicates log entries to follower nodes and commits entries once a majority acknowledges.
- **State Machine**: Applies committed log entries to ensure consistency across all nodes.

## Example Usage
1. Start three server instances on the same machine:
    ```sh
    ./server 0 3 20000 &
    ./server 1 3 20001 &
    ./server 2 3 20002 &
    ```

2. Connect to any server instance and send commands:
    ```sh
    telnet localhost 20000
    msg Hello, Raft!
    get chatLog
    ```

## Contributing
Contributions are welcome! Please submit pull requests or open issues to contribute to the project.

## License
This project is licensed under the MIT License.

