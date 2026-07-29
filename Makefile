CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 $(shell pkg-config --cflags raylib)
LIBS = $(shell pkg-config --libs raylib) -lm -lpthread -lcurl
SRC = src/main.cpp src/render.cpp src/agents.cpp src/ui.cpp src/network.cpp src/llm_client.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = meta-office

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)
