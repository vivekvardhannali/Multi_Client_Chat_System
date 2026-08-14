#!/bin/bash

# Usage:
# ./run_test.sh <server_type> <num_clients> <duration>

if [ $# -ne 3 ]; then
    echo "Usage: $0 <server_type> <num_clients> <duration>"
    echo "Example: $0 select 10 60"
    exit 1
fi

SERVER_TYPE=$1
NUM_CLIENTS=$2
DURATION=$3

LAT_FILE="${SERVER_TYPE}_latency.txt"
MET_FILE="${SERVER_TYPE}_metrics.txt"

echo "---------------------------------"
echo "Preparing test..."
echo "Server type : $SERVER_TYPE"
echo "Clients     : $NUM_CLIENTS"
echo "Duration    : $DURATION seconds"
echo "---------------------------------"

# Remove old files if they exist
rm -f "$LAT_FILE"

echo "Old metric and latency files removed."

echo "Starting load test..."

for ((i=1; i<=NUM_CLIENTS; i++))
do
    ./loadtest $SERVER_TYPE user$i $DURATION &
done

# Wait for all background processes
wait

echo "---------------------------------"
echo "Test completed."
echo "Generated files:"
echo " - $LAT_FILE"
echo " - $MET_FILE"
echo "---------------------------------"