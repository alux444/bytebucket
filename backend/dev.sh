#!/bin/bash

# ByteBucket Development Script
# Usage: ./dev.sh [command] [options]

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Project directories
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

# Helper functions
print_header() {
    echo -e "${BLUE}=================================================================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}=================================================================================${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

# Check if build directory exists
ensure_build_dir() {
    if [ ! -d "$BUILD_DIR" ]; then
        print_info "Creating build directory..."
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake ..
        print_success "Build directory created and configured"
    fi
}

# Build the project
build() {
    print_header "Building ByteBucket"
    ensure_build_dir
    
    cd "$PROJECT_DIR"
    cmake --build build
    
    if [ $? -eq 0 ]; then
        print_success "Build completed successfully"
    else
        print_error "Build failed"
        exit 1
    fi
}

# Run tests
test() {
    print_header "Running Tests"
    build
    
    cd "$PROJECT_DIR"
    
    if [ ! -f "$BUILD_DIR/bytebucket_tests" ]; then
        print_error "Test executable not found. Build may have failed."
        exit 1
    fi
    
    echo -e "${BLUE}Running unit tests...${NC}"
    "$BUILD_DIR/bytebucket_tests"
    
    if [ $? -eq 0 ]; then
        print_success "All tests passed!"
    else
        print_error "Some tests failed"
        exit 1
    fi
}

# Run tests with detailed output
test_verbose() {
    print_header "Running Tests (Verbose)"
    build
    
    cd "$PROJECT_DIR"
    
    echo -e "${BLUE}Running unit tests with detailed output...${NC}"
    "$BUILD_DIR/bytebucket_tests" --success
    
    if [ $? -eq 0 ]; then
        print_success "All tests passed!"
    else
        print_error "Some tests failed"
        exit 1
    fi
}

# Run tests through CTest
test_ctest() {
    print_header "Running Tests (CTest)"
    build
    
    cd "$BUILD_DIR"
    
    echo -e "${BLUE}Running tests through CTest...${NC}"
    ctest --verbose
    
    if [ $? -eq 0 ]; then
        print_success "All CTest tests passed!"
    else
        print_error "Some CTest tests failed"
        exit 1
    fi
}

# Start the server
server() {
    print_header "Starting ByteBucket Server"
    build
    
    cd "$PROJECT_DIR"
    
    if [ ! -f "$BUILD_DIR/bytebucket" ]; then
        print_error "Server executable not found. Build may have failed."
        exit 1
    fi
    
    print_info "Starting server on http://0.0.0.0:8080"
    print_info "Health check: http://0.0.0.0:8080/health"
    print_info "Press Ctrl+C to stop the server"
    echo ""
    
    "$BUILD_DIR/bytebucket"
}

# Clean build directory
clean() {
    print_header "Cleaning Build Directory"
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_success "Build directory cleaned"
    else
        print_info "Build directory doesn't exist, nothing to clean"
    fi
}

# Full rebuild (clean + build)
rebuild() {
    print_header "Full Rebuild (Clean + Build)"
    
    # Force complete clean - remove entire build directory
    if [ -d "$BUILD_DIR" ]; then
        print_info "Removing build directory completely..."
        rm -rf "$BUILD_DIR"
        print_success "Build directory removed"
    fi
    
    # Create fresh build directory and configure
    print_info "Creating fresh build directory..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake ..
    
    if [ $? -eq 0 ]; then
        print_success "CMake configuration completed"
    else
        print_error "CMake configuration failed"
        exit 1
    fi
    
    # Build from scratch
    cd "$PROJECT_DIR"
    print_info "Building from scratch..."
    cmake --build build
    
    if [ $? -eq 0 ]; then
        print_success "Complete rebuild successful!"
    else
        print_error "Build failed"
        exit 1
    fi
}

# Run all checks (build + tests)
check() {
    print_header "Running Full Check (Build + Tests)"
    test
    print_success "All checks passed!"
}

# Run all checks without exiting on failure (for watch mode)
check_no_exit() {
    print_header "Running Full Check (Build + Tests)"
    
    # Always ensure we're in the right directory
    cd "$PROJECT_DIR"
    
    # Ensure build directory exists and is properly configured
    ensure_build_dir
    
    # Always run a full build to detect changes
    echo -e "${BLUE}Building project...${NC}"
    cmake --build build
    
    local build_result=$?
    if [ $build_result -ne 0 ]; then
        print_error "Build failed"
        return 1
    fi
    
    print_success "Build completed successfully"
    
    # Run tests
    if [ ! -f "$BUILD_DIR/bytebucket_tests" ]; then
        print_error "Test executable not found. Build may have failed."
        return 1
    fi
    
    echo -e "${BLUE}Running unit tests...${NC}"
    "$BUILD_DIR/bytebucket_tests"
    
    local test_result=$?
    if [ $test_result -eq 0 ]; then
        print_success "All tests passed!"
        return 0
    else
        print_error "Some tests failed"
        return 1
    fi
}

# Build and run server without exiting on failure (for watch mode)
build_and_run_no_exit() {
    print_header "Building and Starting Server"
    
    # Always ensure we're in the right directory
    cd "$PROJECT_DIR"
    
    # Ensure build directory exists and is properly configured
    ensure_build_dir
    
    # Build the project
    echo -e "${BLUE}Building project...${NC}"
    cmake --build build
    
    local build_result=$?
    if [ $build_result -ne 0 ]; then
        print_error "Build failed"
        return 1
    fi
    
    print_success "Build completed successfully"
    
    # Check if server executable exists
    if [ ! -f "$BUILD_DIR/bytebucket" ]; then
        print_error "Server executable not found. Build may have failed."
        return 1
    fi
    
    # Run tests first to make sure everything works
    if [ -f "$BUILD_DIR/bytebucket_tests" ]; then
        echo -e "${BLUE}Running quick test check...${NC}"
        "$BUILD_DIR/bytebucket_tests" --reporter compact --verbosity quiet
        
        if [ $? -ne 0 ]; then
            print_error "Tests failed - server not started"
            return 1
        fi
        print_success "Tests passed!"
    fi
    
    # Kill any existing server process on port 8080
    print_info "Checking for existing server on port 8080..."
    if lsof -ti:8080 >/dev/null 2>&1; then
        print_info "Stopping existing server..."
        kill -9 $(lsof -ti:8080) 2>/dev/null || true
        sleep 1
    fi
    
    # Start the server in background
    print_info "Starting server on http://0.0.0.0:8080"
    print_info "Health check: http://0.0.0.0:8080/health"
    "$BUILD_DIR/bytebucket" &
    
    local server_pid=$!
    sleep 2
    
    # Check if server started successfully
    if kill -0 $server_pid 2>/dev/null; then
        print_success "Server started successfully (PID: $server_pid)"
        return 0
    else
        print_error "Server failed to start"
        return 1
    fi
}

# Watch mode for development (requires fswatch)
watch() {
    print_header "Starting Watch Mode"
    
    if ! command -v fswatch &> /dev/null; then
        print_error "fswatch is not installed. Please install it first:"
        echo "  macOS: brew install fswatch"
        echo "  Linux: apt-get install fswatch"
        exit 1
    fi
    
    print_info "Watching for file changes..."
    print_info "Will rebuild and run tests on changes to .cpp, .hpp, or .cmake files"
    print_info "Press Ctrl+C to stop watching"
    echo ""
    
    # Initial build and test (don't exit on failure in watch mode)
    if check_no_exit; then
        print_success "✨ Initial build successful! Watching for changes..."
    else
        print_error "❌ Initial build failed, but continuing to watch for fixes..."
    fi
    
    # Watch for changes
    fswatch -o -r --event Created --event Updated --event Removed \
        --include='.*\.(cpp|hpp|h|cmake|txt)$' \
        "$PROJECT_DIR/src" "$PROJECT_DIR/include" "$PROJECT_DIR/tests" "$PROJECT_DIR/CMakeLists.txt" | \
    while read num; do
        echo ""
        print_info "File changes detected (${num} events), rebuilding..."
        
        # Ensure we're in the project directory
        cd "$PROJECT_DIR"
        
        # Run build and test, capturing the result
        if check_no_exit; then
            print_success "✨ Ready for more changes!"
        else
            print_error "❌ Build or tests failed - fix the issues and save to try again"
        fi
        
        echo ""
        print_info "Watching for more changes..."
    done
}

# Watch mode with server restart (requires fswatch)
watch_server() {
    print_header "Starting Watch Mode with Server"
    
    if ! command -v fswatch &> /dev/null; then
        print_error "fswatch is not installed. Please install it first:"
        echo "  macOS: brew install fswatch"
        echo "  Linux: apt-get install fswatch"
        exit 1
    fi
    
    print_info "Watching for file changes..."
    print_info "Will rebuild and restart server on changes to .cpp, .hpp, or .cmake files"
    print_info "Press Ctrl+C to stop watching and server"
    echo ""
    
    # Initial build and server start
    if build_and_run_no_exit; then
        print_success "✨ Initial server started! Watching for changes..."
    else
        print_error "❌ Initial server start failed, but continuing to watch for fixes..."
    fi
    
    # Watch for changes
    fswatch -o -r --event Created --event Updated --event Removed \
        --include='.*\.(cpp|hpp|h|cmake|txt)$' \
        "$PROJECT_DIR/src" "$PROJECT_DIR/include" "$PROJECT_DIR/tests" "$PROJECT_DIR/CMakeLists.txt" | \
    while read num; do
        echo ""
        print_info "File changes detected (${num} events), rebuilding and restarting server..."
        
        # Kill existing server
        if lsof -ti:8080 >/dev/null 2>&1; then
            print_info "Stopping current server..."
            kill -9 $(lsof -ti:8080) 2>/dev/null || true
            sleep 1
        fi
        
        # Ensure we're in the project directory
        cd "$PROJECT_DIR"
        
        # Rebuild and restart server
        if build_and_run_no_exit; then
            print_success "✨ Server restarted! Ready for more changes!"
        else
            print_error "❌ Build failed or server couldn't start - fix the issues and save to try again"
        fi
        
        echo ""
        print_info "Watching for more changes..."
    done
    
    # Cleanup on exit
    print_info "Stopping server..."
    if lsof -ti:8080 >/dev/null 2>&1; then
        kill -9 $(lsof -ti:8080) 2>/dev/null || true
    fi
}

# Install dependencies (macOS with Homebrew)
install_deps() {
    print_header "Installing Dependencies"
    
    if [[ "$OSTYPE" == "darwin"* ]]; then
        if ! command -v brew &> /dev/null; then
            print_error "Homebrew is not installed. Please install it first: https://brew.sh"
            exit 1
        fi
        
        print_info "Installing dependencies via Homebrew..."
        brew install boost catch2 cmake vcpkg
        print_success "Dependencies installed"
    else
        print_warning "Dependency installation is only supported on macOS with Homebrew"
        print_info "Please install manually: boost, catch2, cmake, vcpkg"
    fi
}

# Show project status
status() {
    print_header "ByteBucket Project Status"
    
    echo "Project Directory: $PROJECT_DIR"
    echo "Build Directory: $BUILD_DIR"
    echo ""
    
    # Check if build directory exists
    if [ -d "$BUILD_DIR" ]; then
        print_success "Build directory exists"
        
        # Check if executables exist
        if [ -f "$BUILD_DIR/bytebucket" ]; then
            print_success "Server executable built"
        else
            print_warning "Server executable not found"
        fi
        
        if [ -f "$BUILD_DIR/bytebucket_tests" ]; then
            print_success "Test executable built"
        else
            print_warning "Test executable not found"
        fi
    else
        print_warning "Build directory not found"
    fi
    
    echo ""
    print_info "Available commands:"
    echo "  build     - Build the project"
    echo "  test      - Run tests"
    echo "  server    - Start the server"
    echo "  clean     - Clean build directory"
    echo "  check     - Build and test"
    echo "  watch     - Watch for changes and auto-rebuild"
    echo "  status    - Show this status"
}

# Help function
help() {
    echo "ByteBucket Development Script"
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  build           Build the project"
    echo "  test            Run tests"
    echo "  test-verbose    Run tests with detailed output"
    echo "  test-ctest      Run tests through CTest"
    echo "  server          Start the ByteBucket server"
    echo "  clean           Clean build directory"
    echo "  rebuild         Complete rebuild (clean + build from scratch)"
    echo "  check           Build and run tests"
    echo "  watch           Watch for changes and auto-rebuild/test"
    echo "  watch-server    Watch for changes and auto-rebuild/restart server"
    echo "  install-deps    Install dependencies (macOS only)"
    echo "  status          Show project status"
    echo "  help            Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 check          # Build and test"
    echo "  $0 server         # Start the server"
    echo "  $0 watch          # Development mode with auto-rebuild/test"
    echo "  $0 watch-server   # Development mode with auto-rebuild/restart server"
}

# Main command dispatcher
case "${1:-help}" in
    "build")
        build
        ;;
    "test")
        test
        ;;
    "test-verbose")
        test_verbose
        ;;
    "test-ctest")
        test_ctest
        ;;
    "server")
        server
        ;;
    "clean")
        clean
        ;;
    "rebuild")
        rebuild
        ;;
    "check")
        check
        ;;
    "watch")
        watch
        ;;
    "watch-server")
        watch_server
        ;;
    "install-deps")
        install_deps
        ;;
    "status")
        status
        ;;
    "help"|"--help"|"-h")
        help
        ;;
    *)
        print_error "Unknown command: $1"
        echo ""
        help
        exit 1
        ;;
esac
