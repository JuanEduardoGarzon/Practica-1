# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2

# Executables
INDEXER = indexer
SEARCH_ENGINE = search
UI = ui

.PHONY: all clean run search-system help

# Default target - build everything
all: $(INDEXER) $(SEARCH_ENGINE) $(UI)

# Build the indexer
$(INDEXER): indexer.c
	@echo "🔨 Compiling indexer..."
	$(CC) $(CFLAGS) -o $(INDEXER) indexer.c

# Build the search engine
$(SEARCH_ENGINE): search.c
	@echo "🔨 Compiling search engine..."
	$(CC) $(CFLAGS) -o $(SEARCH_ENGINE) search.c

# Build the UI
$(UI): ui.c
	@echo "🔨 Compiling UI interface..."
	$(CC) $(CFLAGS) -o $(UI) ui.c

# Create the index file and run complete system
run: $(INDEXER) $(SEARCH_ENGINE) $(UI)
	@echo "📊 Creating index file..."
	./$(INDEXER)
	@echo ""
	@echo "🚀 Starting complete book search system..."
	@echo "💡 Start the search engine in one terminal and UI in another"
	@echo ""
	@echo "Terminal 1: ./$(SEARCH_ENGINE)"
	@echo "Terminal 2: ./$(UI)"
	@echo ""

# Just run the search system (assumes index already exists)
search-system: $(SEARCH_ENGINE) $(UI)
	@echo "🔍 Starting search system..."
	@echo "💡 Start in two separate terminals:"
	@echo ""
	@echo "Terminal 1: ./$(SEARCH_ENGINE)"
	@echo "Terminal 2: ./$(UI)"
	@echo ""

# Clean build files
clean:
	@echo "🧹 Cleaning up..."
	rm -f $(INDEXER) $(SEARCH_ENGINE) $(UI)

# Show help
help:
	@echo "📚 Book Search System Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build all executables"
	@echo "  run          - Build everything, create index, and show run instructions"
	@echo "  search-system - Show instructions to run search system"
	@echo "  clean        - Remove all built files"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make run          # First time: builds, creates index, shows instructions"
	@echo "  make search-system # Subsequent runs: just shows instructions"
	@echo "  make all          # Just build executables without running"
