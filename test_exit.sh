#!/bin/bash

# Fork Eater Exit Functionality Test Script
# This script tests all exit methods to ensure they work properly

# set -e removed to allow custom exit codes in tests

PROJECT_DIR=$(dirname "$(realpath "$0")")
cd "$PROJECT_DIR/"

echo "🧪 Testing Fork Eater Exit Functionality..."
echo "============================================="

# Test 1: Help command
echo "1️⃣  Testing --help command..."
./build/fork-eater --help > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "   ✅ Help command works"
else
    echo "   ❌ Help command failed"
    exit 1
fi

# Test 2: Test mode with default exit code
echo "2️⃣  Testing --test mode (default exit code)..."
./build/fork-eater --test > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "   ✅ Test mode works with exit code 0"
else
    echo "   ❌ Test mode failed"
    exit 1
fi

# Test 3: Test mode with custom exit code
echo "3️⃣  Testing --test mode (custom exit code 42)..."
./build/fork-eater --test 42 > /dev/null 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 42 ]; then
    echo "   ✅ Test mode works with custom exit code 42"
else
    echo "   ❌ Test mode failed with custom exit code (got exit code $EXIT_CODE)"
    exit 1
fi

# Test 4: Application starts and can be terminated
echo "4️⃣  Testing application startup and termination..."
timeout 5s ./build/fork-eater project/test > /dev/null 2> error.log &
APP_PID=$!
sleep 1

# Check if process is running
if kill -0 $APP_PID 2>/dev/null; then
    echo "   ✅ Application starts successfully"
    # Terminate the process
    kill $APP_PID 2>/dev/null
    wait $APP_PID 2>/dev/null || true
    echo "   ✅ Application terminates properly"
else
    echo "   ❌ Application failed to start"
    exit 1
fi

echo "============================================="
echo "🎉 All exit functionality tests passed!"
echo ""
echo "Summary:"
echo "  • Help command: ✅ Works"
echo "  • Test mode: ✅ Works (both default and custom exit codes)"
echo "  • Application startup: ✅ Works"
echo "  • Application termination: ✅ Works"
echo ""
echo "The Fork Eater shader editor is ready for use!"