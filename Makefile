# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# Project files
SRCS = Server.cpp Network.cpp Log.cpp StateMachine.cpp main.cpp
OBJS = $(SRCS:.cpp=.o)

# Target executable
TARGET = process

# Default rule to build the project
all: $(TARGET)

# Linking the object files to create the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule to compile the individual source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule to remove generated files
clean:
	rm -f $(OBJS) $(TARGET)

# Run rule to execute the server
run: $(TARGET)
	./$(TARGET)
