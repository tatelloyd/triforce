CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -pthread

# Directories
SRC_DIR = src
BUILD_DIR = build
TEST_DIR = test

# Files
TARGET = $(BUILD_DIR)/imu_parser
SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/imu_parser_api.cpp
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/imu_parser_api.o

# Include path for headers
CXXFLAGS += -I$(SRC_DIR)

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Install target (optional)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/imu_parser

# Create virtual serial ports for testing
test-setup:
	@echo "Creating virtual serial ports for testing..."
	@echo "Run this command in a separate terminal:"
	@echo "socat -d -d pty,raw,echo=0 pty,raw,echo=0"
	@echo ""
	@echo "Then note the two PTY devices created (e.g., /dev/pts/2 and /dev/pts/3)"
	@echo "Use one for the parser and the other for the simulator"

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(BUILD_DIR) $(TARGET)

# Run the parser with default virtual port
run: $(TARGET)
	@echo "Make sure virtual serial ports are set up first!"
	@echo "If not done already, run: make test-setup"
	$(TARGET) /dev/pts/2

# Run with custom port
run-port: $(TARGET)
	@read -p "Enter UART device path: " port; \
	$(TARGET) $$port

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build the IMU parser (default)"
	@echo "  debug        - Build with debug symbols"
	@echo "  clean        - Remove build artifacts"
	@echo "  install      - Install the binary to /usr/local/bin/"
	@echo "  test-setup   - Show how to create virtual serial ports"
	@echo "  run          - Run the parser with default port (/dev/pts/2)"
	@echo "  run-port     - Run the parser with custom port"
	@echo "  help         - Show this help message"

.PHONY: all clean install test-setup debug run run-port help