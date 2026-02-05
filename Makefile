CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS  = `pkg-config --libs --cflags raylib`

# Source files
SOURCES  = main.cpp core.cpp player.cpp enemy.cpp particles.cpp render.cpp

# Executable name
EXEC     = game

# Default target
all: $(EXEC)

# Link everything into the final executable
$(EXEC): $(SOURCES) game.h
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(EXEC) $(LDFLAGS)

# Clean up
clean:
	rm -f $(EXEC) *.o

# Run the game (convenience target)
run: $(EXEC)
	./$(EXEC)

# Phony targets
.PHONY: all clean run
