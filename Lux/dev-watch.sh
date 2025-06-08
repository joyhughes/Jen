#!/bin/bash

# File Watcher for Automatic C++ Rebuilds
# Monitors C++ source files and triggers incremental builds

set -e

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}👀 Starting C++ file watcher...${NC}"

# Check if inotify-tools is available, install if needed
if ! command -v inotifywait &> /dev/null; then
    echo -e "${YELLOW}📦 Installing inotify-tools...${NC}"
    apt-get update > /dev/null 2>&1
    apt-get install -y inotify-tools > /dev/null 2>&1
fi

# Source directories to watch
SRC_DIR="/app/src"
HEADER_DIRS="/app/src /app/stb_image"

# Make build script executable
chmod +x /app/build-cpp.sh

echo -e "${GREEN}✅ Watching C++ files for changes...${NC}"
echo -e "${BLUE}📁 Monitoring: ${SRC_DIR}${NC}"
echo -e "${BLUE}📋 Will trigger: ./build-cpp.sh${NC}"
echo ""

# Function to run build
run_build() {
    echo -e "${YELLOW}🔄 Change detected, rebuilding...${NC}"
    echo ""
    /app/build-cpp.sh
    echo ""
    echo -e "${GREEN}🎉 Ready! Browser will auto-refresh.${NC}"
    echo -e "${BLUE}👀 Continuing to watch for changes...${NC}"
    echo ""
}

# Watch for changes in C++ source files and headers
inotifywait -m -r -e modify,create,delete,move \
    --format '%w%f %e' \
    "$SRC_DIR" $HEADER_DIRS 2>/dev/null | \
while read file event; do
    # Only trigger on C++ source files and headers
    if [[ "$file" =~ \.(cpp|hpp|h)$ ]]; then
        # Debounce: wait a moment to avoid multiple rapid triggers
        sleep 0.5
        run_build
    fi
done 