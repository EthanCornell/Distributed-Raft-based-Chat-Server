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

#ifndef LOG_H
#define LOG_H

#include <vector>
#include <string>

struct LogEntry {
    int term;
    std::string command;
    int index;
};

class Log {
public:
    Log();
    void addEntry(const LogEntry &entry);
    LogEntry getEntry(int index) const;
    int getLastTerm() const;
    int getLastIndex() const;

private:
    std::vector<LogEntry> entries;
};

#endif // LOG_H
