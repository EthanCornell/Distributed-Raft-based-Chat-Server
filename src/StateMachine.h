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
