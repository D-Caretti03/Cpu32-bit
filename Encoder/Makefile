TARGET = Encoder
CFLAGS = -Wall -Wpedantic -g -O2
CXX = g++
SRC = Encoder.cpp

$(TARGET): $(SRC)
	$(CXX) $(CFLAGS) -o $(TARGET) $(SRC)

.PHONY: clean

clean: $(TARGET)
	rm $(TARGET)

.PHONY: all

all: clean
	$(MAKE) $(TARGET)
