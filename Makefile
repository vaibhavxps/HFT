SHELL = C:/msys64/usr/bin/sh.exe # Or just sh if in MSYS2 MinGW terminal and PATH is set
CXX = g++
# Note: Removed a stray non-breaking space character after -pthread if it was there
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -pthread
LDFLAGS = -lws2_32 # Add this for Winsock linking on Windows
RM = rm -f

# Directories
SRCDIR = src
INCDIR = include
BUILDDIR = build
TARGET = $(BUILDDIR)/hft_core_phase1

# Source files and object files
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(@D) 
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $@"

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR) 
	@mkdir -p $(@D) 
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

clean:
	$(RM) $(TARGET) $(OBJS)
	@echo "Cleaned build files."
	-$(RM) -r $(BUILDDIR)









# SHELL = C:/msys64/usr/bin/sh.exe
# CXX = g++
# CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -pthread # -pthread for std::mutex if needed by older g++
# LDFLAGS =
# RM = rm -f

# # Directories
# SRCDIR = src
# INCDIR = include
# BUILDDIR = build
# TARGET = $(BUILDDIR)/hft_core_phase1

# # Source files and object files
# SRCS = $(wildcard $(SRCDIR)/*.cpp)
# OBJS = $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SRCS))

# .PHONY: all clean

# all: $(TARGET)

# $(TARGET): $(OBJS)
# 	@mkdir -p $(@D) # Create directory for target if it doesn't exist
# 	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
# 	@echo "Build complete: $@"

# $(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR) # Prerequisite on BUILDDIR ensures it's created
# 	@mkdir -p $(@D) # Create directory for object files
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# $(BUILDDIR):
# 	@mkdir -p $(BUILDDIR)

# clean:
# 	$(RM) $(TARGET) $(OBJS)
# 	@echo "Cleaned build files."
# 	-$(RM) -r $(BUILDDIR) # Remove build directory, '-' ignores error if it doesn't exist