CXX = g++
CXXFLAGS = -std=c++17 -Wall -I/usr/include -I/usr/local/include
LDFLAGS = -lglfw -lGLEW -lGL -lm

SRC = main.cpp shape.cpp model.cpp hnode.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = modeller

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
