# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I/usr/include -I/usr/local/include
LDFLAGS = -lglfw -lGLEW -lGL -lm

# Source and target
SRC = main.cpp input.cpp HEIRARCHIAL_NODE.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = modeller

# Default target
all: $(TARGET)

# Link step
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

# Compile step
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJ) $(TARGET)

