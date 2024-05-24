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

#include "Log.h"
#include <stdexcept>

Log::Log() {
    // Initialize the log with a dummy entry to start indices from 1
    entries.push_back(LogEntry{0, "", 0});
}

void Log::addEntry(const LogEntry &entry) {
    entries.push_back(entry);
}

LogEntry Log::getEntry(int index) const {
    if (index < 0 || index >= static_cast<int>(entries.size())) {
        throw std::out_of_range("Invalid log index");
    }
    return entries[index];
}

int Log::getLastTerm() const {
    return entries.back().term;
}

int Log::getLastIndex() const {
    return entries.back().index;
}
