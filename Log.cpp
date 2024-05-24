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
