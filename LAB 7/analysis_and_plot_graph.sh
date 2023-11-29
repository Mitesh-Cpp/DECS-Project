#!/bin/bash

# Check if the correct number of arguments are provided
if [ "$#" -ne 5 ]; then
  echo "Usage: $0 <ipaddress> <port> <codefile> <loopNum> <sleepTimeSeconds>"
  exit 1
fi

ipaddress=$1
port=$2
codefile=$3
loopNum=$4
sleepTime=$5

for i in 2 4 8 16 32 40 50 60 70 75 80 85; do
  ./loadtest.sh "$1" "$2" "$i" "$3" "$4" "$5"
done

wait
data_file="performance_data.dat"
python3 <<PY
import matplotlib.pyplot as plt
import numpy as np

# Load data from the file
data = np.loadtxt("$data_file")

# Plot Throughput vs. Number of Clients
plt.figure(figsize=(8, 6))
plt.plot(data[:, 0], data[:, 1], label='Throughput', marker='o')
plt.title('Throughput vs. Number of Clients')
plt.xlabel('Number of Clients (M)')
plt.ylabel('Throughput')
plt.legend()
plt.savefig('./throughput_plot.png')

# Plot Average Response Time vs. Number of Clients
plt.figure(figsize=(8, 6))
plt.plot(data[:, 0], data[:, 2], label='Avg Response Time', marker='o')
plt.title('Average Response Time vs. Number of Clients')
plt.xlabel('Number of Clients (M)')
plt.ylabel('Average Response Time')
plt.legend()
plt.savefig('./response_time_plot.png')
PY

echo "Performance results plots saved to throughput_plot.png and response_time_plot.png"
rm ./client_output/*