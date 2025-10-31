# Compiler settings
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -I./include
LDFLAGS := -lftxui-component -lftxui-dom -lftxui-screen -lboost_system -lssl -lcrypto -lpthread

# Directory structure
SRCDIR := src
OBJDIR := obj
BINDIR := bin

# Target executable
TARGET := $(BINDIR)/chat

# Find all source files recursively
SOURCES := $(shell find $(SRCDIR) -name '*.cpp')

# Generate object file names (preserve directory structure)
OBJECTS := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Generate dependency files
DEPS := $(OBJECTS:.o=.d)

# Create directories if they don't exist
$(shell mkdir -p $(OBJDIR) $(BINDIR))

# Main target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# Include dependency files
-include $(DEPS)

# Compile source files to object files (with automatic directory creation)
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: rebuild
rebuild: clean all
