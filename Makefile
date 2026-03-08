TARGET = Cpu
CFLAGS = -Wall -Wpedantic -g
CXX = g++
SRC = Cpu.cpp

$(TARGET): $(SRC)
	$(CXX) $(CFLAGS) -o $(TARGET) $(SRC)

.PHONY: clean

clean:
	rm $(TARGET)

.PHONY: all

all: $(TARGET)

.PHONY: rebuild

rebuild:
	$(MAKE) clean
	$(MAKE) all
