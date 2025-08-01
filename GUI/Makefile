# Simple Makefile for Environment Variables Viewer (Single File)
# MinGW-64 g++ with Qt6

# Application name
TARGET = EnvironmentViewer.exe

# Qt installation path (adjust this to your Qt installation)
QT_PATH = /c/Qt/6.9.1/mingw_64

# Compiler and tools
CXX = g++
MOC = $(QT_PATH)/bin/moc.exe

# Directories
BUILD_DIR = build

# Qt modules
QT_MODULES = Core Gui Widgets

# Include paths
QT_INCLUDES = $(addprefix -I$(QT_PATH)/include/Qt, $(QT_MODULES))
INCLUDES = -I$(QT_PATH)/include $(QT_INCLUDES)

# Library paths and libraries
LIBPATH = -L$(QT_PATH)/lib
QT_LIBS = $(addprefix -lQt6, $(QT_MODULES))

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 $(INCLUDES)
CXXFLAGS += -DQT_NO_DEBUG -DQT_WIDGETS_LIB -DQT_CORE_LIB

# Linker flags
LDFLAGS = $(LIBPATH) -Wl,-subsystem,windows

# Source and generated files
SOURCE = EnvironmentViewer.cpp
MOC_FILE = EnvironmentViewer.moc
OBJECT = $(BUILD_DIR)/EnvironmentViewer.o

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p "$(BUILD_DIR)"

# Generate MOC file
$(MOC_FILE): $(SOURCE)
	@echo "Generating MOC..."
	$(MOC) $(INCLUDES) $(SOURCE) -o $(MOC_FILE)

# Compile the source file (depends on MOC file)
$(OBJECT): $(SOURCE) $(MOC_FILE)
	@echo "Compiling $(SOURCE)..."
	$(CXX) $(CXXFLAGS) -c $(SOURCE) -o $(OBJECT)

# Link the executable
$(TARGET): $(OBJECT)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJECT) -o $@ $(LDFLAGS) $(QT_LIBS)
	@echo "Build complete: $(TARGET)"

# Clean build files
clean:
	@echo "Cleaning build files..."
	@rm -rf "$(BUILD_DIR)"
	@rm -f "$(TARGET)"
    @rm EnvironmentViewer.moc
	@echo "Clean complete."

# Clean and rebuild
rebuild: clean all

# Run the application
run: $(TARGET)
	@echo "Running $(TARGET)..."
	@./$(TARGET)

# Debug build
debug: CXXFLAGS += -g -DDEBUG -DQT_DEBUG
debug: CXXFLAGS := $(filter-out -O2 -DQT_NO_DEBUG, $(CXXFLAGS))
debug: all

# Check Qt installation
check-qt:
	@echo "Checking Qt installation..."
	@echo "Qt Path: $(QT_PATH)"
	@if [ -f "$(QT_PATH)/bin/moc.exe" ]; then \
		echo "✓ MOC found: $(QT_PATH)/bin/moc.exe"; \
	else \
		echo "✗ ERROR: MOC not found at $(QT_PATH)/bin/moc.exe"; \
	fi
	@if [ -f "$(QT_PATH)/bin/g++.exe" ]; then \
		echo "✓ G++ found: $(QT_PATH)/bin/g++.exe"; \
	else \
		echo "✗ ERROR: G++ not found at $(QT_PATH)/bin/g++.exe"; \
	fi
	@if [ -d "$(QT_PATH)/include/QtCore" ]; then \
		echo "✓ Qt includes found: $(QT_PATH)/include/QtCore"; \
	else \
		echo "✗ ERROR: Qt includes not found at $(QT_PATH)/include/QtCore"; \
	fi
	@if [ -f "$(QT_PATH)/lib/libQt6Core.a" ]; then \
		echo "✓ Qt libraries found: $(QT_PATH)/lib/libQt6Core.a"; \
	else \
		echo "✗ ERROR: Qt libraries not found at $(QT_PATH)/lib/libQt6Core.a"; \
	fi

# Show configuration
config:
	@echo "================================"
	@echo "Environment Variables Viewer Build Configuration"
	@echo "================================"
	@echo "Target: $(TARGET)"
	@echo "Source: $(SOURCE)"
	@echo "Compiler: $(CXX)"
	@echo "Qt Path: $(QT_PATH)"
	@echo "Includes: $(INCLUDES)"
	@echo "Libs: $(QT_LIBS)"
	@echo "CXX Flags: $(CXXFLAGS)"
	@echo "LD Flags: $(LDFLAGS)"
	@echo "================================"

# Help target
help:
	@echo "Available targets:"
	@echo "  all       - Build the application (default)"
	@echo "  clean     - Remove build files"
	@echo "  rebuild   - Clean and build"
	@echo "  run       - Build and run the application"
	@echo "  debug     - Build with debug information"
	@echo "  check-qt  - Verify Qt installation"
	@echo "  config    - Show build configuration"
	@echo "  help      - Show this help message"

# Phony targets
.PHONY: all clean rebuild run debug check-qt config help directories