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
