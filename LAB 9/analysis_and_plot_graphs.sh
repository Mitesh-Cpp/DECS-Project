#!/bin/bash
echo "Running analysis for varying number of clients."

# Different number of Client
numClient='
1
2
4
8
10
15
20
30
40
50
70
80
90
100
'

PROGRAM='loadtest.sh'
loopNum=5
sleepTime=0
timeouttime=100
cpu_utilization=0.0
ipaddr=0.0.0.0
port=2000
studentCode='studentCode.cpp'

capture_running=true
capture_stats() {
        vmstat 10 >> vmstat_snapshots.txt &
    while $capture_running; do
        (ps -eLf | grep '\./server' | head -n 1) >> process_snapshots.txt &
        sleep 10
    done
}

# Function to stop capturing statistics
stop_capture() {
    capture_running=false
    pkill -f 'vmstat 10'
    pkill -f 'ps -eLf'
}


# Function to calculate the average of the "id" column from vmstat data
calculate_cpu_utilization() {
    cpu_utilization=$(awk '/^[0-9 \t]+$/ {sum+=$15; count++} END {print sum/count}' vmstat_snapshots.txt)

}

calculate_average() {
    column_values=$(awk '{print $6}' process_snapshots.txt)
    average=$(echo "$column_values" | awk '{s+=$1} END {print s/NR}')
    nlwp=$average
}


# Run the analysis for different sizes of clients
for i in ${numClient}; do
    capture_stats &
    bash $PROGRAM $ipaddr $port $studentCode $i $loopNum $sleepTime $timeouttime
    stop_capture
    capture_running=true
    calculate_cpu_utilization
    calculate_average
    cpu_utilization=$(bc <<< "100 - $cpu_utilization")
    echo "$i $cpu_utilization" >> cpu_util.txt
    echo "$i $nlwp" >> nlwp_data.txt
    rm vmstat_snapshots.txt
    rm process_snapshots.txt
done


# Plotting final results
cat throughput_data.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Overall Throughtput vs No. of clients" -X "No. of Clients" -Y "Throughput" -r 0.25 > ./throughput.png

cat response_time_data.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Avg. Response Time vs No. of clients " -X "No. of Clients" -Y "Avg. Response Time(in s)" -r 0.25 > ./response_time.png

cat request_rate.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Avg. Request rate vs No. of clients " -X "No. of Clients" -Y "Avg. Requests(per s)" -r 0.25 > ./request_rate.png

cat goodput_data.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Overall Goodput vs No. of clients " -X "No. of Clients" -Y "Goodput" -r 0.25 > ./goodput_data.png

cat timeout_rate.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Avg. Timeout Rate vs No. of clients " -X "No. of Clients" -Y "Avg. Timeouts(per s)" -r 0.25 > ./timeout_rate.png

cat error_rate.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Avg. Error Rate vs No. of clients " -X "No. of Clients" -Y "Avg. Number of Errors(per s)" -r 0.25 > ./error_rate.png

cat cpu_util.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Avg. CPU Utilization vs No. of clients " -X "No. of Clients" -Y "Avg. CPU Utilization(in percentage)" -r 0.25 > ./cpu_util.png

cat nlwp_data.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Avg. NLWP vs No. of clients " -X "No. of Clients" -Y "Avg. Number of Threads(at any instance)" -r 0.25 > ./nlwp.png

echo "Analysis Done..!!"
