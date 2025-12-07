CC = g++
CFLAGS = -Wall -O2

# Libraries to link:
LIBS = -lsimlib -lm

TARGET = simulation
RESULTS = out/test_results.csv

SOURCES = src/main.cpp

# Phony targets ensure make doesn't confuse these command names with file names
.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	# Create the bin directory if it doesn't exist
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)

run: $(TARGET)
	./$(TARGET)
	@$(MAKE) print-results

clean:
	# -r removes the directory recursively
	rm -rf $(BIN_DIR) farm_simulation_report.txt out simulation

print-results-interactive:
	@echo "\n$(RESULTS):"
	@cat $(RESULTS) | column -t -s, | less -S

print-results:
	@echo "\n$(RESULTS):"
	@cat $(RESULTS) | column -t -s,