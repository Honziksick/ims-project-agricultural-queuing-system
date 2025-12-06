CC = g++
CFLAGS = -Wall -O2

# Libraries to link:
LIBS = -lsimlib -lm

# Directory for the executable
BIN_DIR = bin

TARGET = $(BIN_DIR)/farm_sim

SOURCES = main.cpp

all: $(TARGET)

$(TARGET): $(SOURCES) config.h farm_classes.h
	# Create the bin directory if it doesn't exist
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	# -r removes the directory recursively
	rm -rf $(BIN_DIR) farm_simulation_report.txt out

# Phony targets ensure make doesn't confuse these command names with file names
.PHONY: all run clean