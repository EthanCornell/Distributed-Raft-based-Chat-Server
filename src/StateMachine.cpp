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

#include "StateMachine.h"

StateMachine::StateMachine() {
    // Initialize the state machine if necessary
}

void StateMachine::applyLogEntry(const LogEntry &entry) {
    // Simulate applying the log entry by updating the state
    // Assuming a simple key-value model for demonstration
    // The command could be a simple "key=value" string
    auto separatorPos = entry.command.find('=');
    if (separatorPos != std::string::npos) {
        std::string key = entry.command.substr(0, separatorPos);
        std::string value = entry.command.substr(separatorPos + 1);
        currentState[key] = value;
    }
}

std::string StateMachine::getState() const {
    // Return a representation of the current state
    std::string state;
    for (const auto &pair : currentState) {
        state += pair.first + "=" + pair.second + "\n";
    }
    return state;
}
