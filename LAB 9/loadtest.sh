# Script to run n client parallely / perform a load test on server

#!/usr/bin/bash

argc=$#
if [ $argc -ne 7 ]
then
echo "Usage : ./loadtest.sh <ip-addr> <port> <studentCode.cpp> <numClients> <loopNum> <sleepTimeSeconds> <timeout-seconds>"
exit 1
fi


ipaddr=$1
port=$2
studentCode=$3
numClients=$4
loopNum=$5
sleepTimeSeconds=$6
ct=$4
waittime=$7

for ((i=1;i<=$ct;i++));
do 
 echo "Starting client $i in background"
./client $1 $2 $3 $5 $6 $7 > "clientoutput$i.txt" &
done

wait

overallthroughput=0
avgResponseTime=0
numberofResponses=0
avgreqrate=0.0
avgsuccessfulrate=0
avgtimeoutrate=0
avgerrorrate=0

for ((j = 1; j <= numClients; j++)); do
  avgResponseTime_i=$(grep "Average Response Time (seconds):" "clientoutput$j.txt" | cut -d ':' -f 2)
  throughput_i=$(grep "Throughput:" "clientoutput$j.txt" | cut -d ':' -f 2)
  successfulResponses_i=$(grep "Number of Successful Responses:" "clientoutput$j.txt" | cut -d ':' -f 2)
  avgreqrate_i=$(grep "Request sent rate:" "clientoutput$j.txt" | cut -d ':' -f 2)
  avgsuccessfulrate_i=$(grep "Successful request rate:" "clientoutput$j.txt" | cut -d ':' -f 2)
  avgtimeoutrate_i=$(grep "Timeout rate:" "clientoutput$j.txt" | cut -d ':' -f 2)
  avgerrorrate_i=$(grep "Error rate:" "clientoutput$j.txt" | cut -d ':' -f 2)
  avgResponseTime=$(echo "$avgResponseTime + $avgResponseTime_i" | bc -l)
  overallthroughput=$(echo "$overallthroughput + $throughput_i" | bc -l)
  numberofResponses=$(echo "$numberofResponses + $successfulResponses_i" | bc -l)
  avgreqrate=$(echo "$avgreqrate + $avgreqrate_i" | bc -l)
  avgsuccessfulrate=$(echo "$avgsuccessfulrate + $avgsuccessfulrate_i" | bc -l)
  avgtimeoutrate=$(echo "$avgtimeoutrate + $avgtimeoutrate_i" | bc -l)
  avgerrorrate=$(echo "$avgerrorrate + $avgerrorrate_i" | bc -l)
done
avgResponseTime=$(echo "$avgResponseTime / $numClients" | bc -l)
echo "Avg Response Time = $avgResponseTime"
echo "$numClients $overallthroughput" >> throughput_data.txt
echo "Throughput = $overallthroughput"
echo "$numClients $avgResponseTime" >> response_time_data.txt
echo "$numClients $avgreqrate" >> request_rate.txt
echo "Avg Req Rate = $avgreqrate"
echo "$numClients $avgsuccessfulrate" >> goodput_data.txt
echo "Avg Successful Rate = $avgsuccessfulrate"
echo "$numClients $avgtimeoutrate" >> timeout_rate.txt
echo "Avg Timeout Rate = $avgtimeoutrate"
echo "$numClients $avgerrorrate" >> error_rate.txt
echo "Avg Error Rate = $avgerrorrate"

rm clientoutput*