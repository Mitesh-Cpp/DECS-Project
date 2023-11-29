#!/bin/bash

# Check if the correct number of arguments are provided
if [ "$#" -ne 6 ]; then
  echo "Usage: <ipaddress> <port> <numClients> <codefile> <loopNum> <sleepTimeSeconds>"
  exit 1
fi

ipaddress=$1
port=$2
numClients=$3
codefile=$4
loopNum=$5
sleepTime=$6

# Create a directory for client output files
mkdir -p client_output

# Create a file to store the performance data
data_file="performance_data.dat"

# Start M clients in the background
for ((i = 0; i < $numClients; i++)); do
  ./client "$ipaddress" "$port" "$codefile" "$loopNum" "$sleepTime" > "./client_output/client_$i.txt" &
done

# Wait for all clients to finish
wait


totalResponses=0
totalResponseTime=0
totalThroughput=0
for ((i = 0; i < numClients; i++)); do
  responses=$(grep "Successful Responses" "client_output/client_$i.txt" | awk '{print $12}')
  throughput=$(grep "Average Response Time" "client_output/client_$i.txt" | awk '{print $8}')
  responseTime=$(grep "Time taken for completing the Loop" "client_output/client_$i.txt" | awk '{print $4}')
  totalResponses=$((totalResponses + responses))
  totalThroughput=$(echo "$totalThroughput + ($responses * $throughput)" | bc)
  totalResponseTime=$(($totalResponseTime + ($responses * $responseTime)))
done
avgResponseTime=$(echo "$totalResponseTime / $numClients" | bc -l)
# Save this data into a file
echo "$numClients $totalThroughput $avgResponseTime" >> "$data_file"

# Print the results
echo "Overall Throughput: $totalThroughput"
echo "Average Response Time: $avgResponseTime"