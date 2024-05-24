#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <string>
#include <map>
#include "Log.h"

class StateMachine {
public:
    StateMachine();
    void applyLogEntry(const LogEntry &entry);
    std::string getState() const;

private:
    std::map<std::string, std::string> currentState; // Example state representation
};

#endif // STATEMACHINE_H
