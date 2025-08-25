#!/bin/bash

# Quick compile and run script
# Usage: ./test.sh <cpp_file_path>
# Example: ./test.sh src/test/hashing/test_mphf_utils.cpp

if [ $# -eq 0 ]; then
    echo "Usage: ./test.sh <cpp_file_path>"
    echo "Example: ./test.sh src/test/hashing/test_mphf_utils.cpp"
    exit 1
fi

CPP_FILE=$1
BASENAME=$(basename "$CPP_FILE" .cpp)
OUTPUT_FILE="/tmp/${BASENAME}"

# Check if file exists
if [ ! -f "$CPP_FILE" ]; then
    echo "Error: $CPP_FILE not found"
    exit 1
fi

echo "Compiling and running $CPP_FILE..."

# Compile and run in one line
g++ -std=c++11 -I. "$CPP_FILE" -o "$OUTPUT_FILE" && "$OUTPUT_FILE"

# Clean up
rm -f "$OUTPUT_FILE"
