#!/bin/bash

SERVER_TYPES=("fork" "thread" "select")
CLIENT_COUNTS=(5 10 20 30 40 50)
DURATION=60
GRACE_TIME=70   # server will be force-killed after 70 seconds

cleanup() {
    pkill -f server_fork 2>/dev/null
    pkill -f server_thread 2>/dev/null
    pkill -f server_select 2>/dev/null
    pkill -f discovery 2>/dev/null
}

trap cleanup INT TERM EXIT

for SERVER_TYPE in "${SERVER_TYPES[@]}"
do
    echo "==============================="
    echo "Starting stress test for $SERVER_TYPE"
    echo "==============================="

    SUMMARY_FILE="${SERVER_TYPE}_summary.txt"
    echo "clients avg_latency avg_cpu avg_vmrss avg_pss" > $SUMMARY_FILE

    for CLIENTS in "${CLIENT_COUNTS[@]}"
    do
        echo "Running $SERVER_TYPE with $CLIENTS clients..."

        cleanup
        sleep 2

        rm -f ${SERVER_TYPE}_latency.txt
        rm -f ${SERVER_TYPE}_metrics.txt

        # Start discovery
        ./discovery &
        sleep 1

        # Start server
        ./server_$SERVER_TYPE &
        sleep 2

        # Run loadtest with timeout protection
        timeout $GRACE_TIME bash -c "
            for i in \$(seq 1 $CLIENTS)
            do
                ./loadtest $SERVER_TYPE user\$i $DURATION &
            done
            wait
        "

        echo "Stopping server after timeout..."

        cleanup
        sleep 2

        # ----- Compute averages -----

        if [ -f ${SERVER_TYPE}_latency.txt ]; then
            AVG_LATENCY=$(awk '{sum+=$1} END {if(NR>0) print sum/NR; else print 0}' ${SERVER_TYPE}_latency.txt)
        else
            AVG_LATENCY=0
        fi

        if [ -f ${SERVER_TYPE}_metrics.txt ]; then
            AVG_CPU=$(awk 'NR>1 {sum+=$2; count++} END {if(count>0) print sum/count; else print 0}' ${SERVER_TYPE}_metrics.txt)
            AVG_VMRSS=$(awk 'NR>1 {sum+=$3; count++} END {if(count>0) print sum/count; else print 0}' ${SERVER_TYPE}_metrics.txt)
            AVG_PSS=$(awk 'NR>1 {sum+=$4; count++} END {if(count>0) print sum/count; else print 0}' ${SERVER_TYPE}_metrics.txt)
        else
            AVG_CPU=0
            AVG_VMRSS=0
            AVG_PSS=0
        fi

        echo "$CLIENTS $AVG_LATENCY $AVG_CPU $AVG_VMRSS $AVG_PSS" >> $SUMMARY_FILE

        echo "Completed $CLIENTS clients for $SERVER_TYPE"
    done

    echo "Finished $SERVER_TYPE stress test"
done

echo "================================="
echo "All stress tests completed"
echo "================================="