ip=$1
port=$2
file=$3
timeout=$4
maxclient=$5

> total_response_time.txt
> avg_response_time.txt
> throughput.txt
max_c=$(($maxclient/10))
for ((j = 1 ; j <= $max_c; j++)); do
> response$j.txt
loop_num=$((10*$j))
for ((i = 0 ; i < $loop_num ; i++)); do
  { time bash test_client.sh $ip $port $file $timeout $i; } 2>&1 | grep "real" | awk 'BEGIN {FS="\t"} {print $2}' | awk 'BEGIN {FS="m"} {print $2}' | awk 'BEGIN {FS="s"} {print $1}' >> response$j.txt &
done

wait
filename="response$j.txt"
total_response_time=0
while read -r line; do
    response_time=$(echo "$line")
    total_response_time=$(echo "$total_response_time+$response_time" | bc)

done < "$filename"
echo $loop_num $total_response_time >> total_response_time.txt
avg_resp=$(echo $total_response_time/$loop_num | bc -l) 
echo "$loop_num $avg_resp">> avg_response_time.txt
#echo $loop_num/$total_response_time | bc -l >> throughput.txt 

rm response*.txt
done 


data_file="avg_response_time.txt"
python3 <<PY
import matplotlib.pyplot as plt
import numpy as np

# Load data from the file
data = np.loadtxt("$data_file")

# Plot Average Response Time vs. Number of Clients
plt.figure(figsize=(8, 6))
plt.plot(data[:, 0], data[:, 1], label='Avg Response Time', marker='o')
plt.title('Average Response Time vs. Number of Clients')
plt.xlabel('Number of Clients (M)')
plt.ylabel('Average Response Time')
plt.legend()
plt.savefig('./response_time_plot.png')
PY

echo "Performance results plots saved to response_time_plot.png"
