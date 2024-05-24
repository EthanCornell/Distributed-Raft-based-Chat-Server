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
