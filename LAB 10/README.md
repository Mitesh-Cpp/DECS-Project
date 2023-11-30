
# Asynchronous Autograder 
The command required to check the implementation and working of the autograder server are given below. Please follow the sequence of the commands else there will be errors.

## Initialize 
bash initialize.sh

- Run this to make all the directories which will be required for server and client to store their files.  

## Create executables
make 

- This will create the executables server and client.

## Running server
./server port num_file_receive_thread num_handler_thread 0/1(Restart/Continue)

Example 1:    ./server 8080 10 10 0
- This will create 10 threads for receiving files and 10 for handling requests from request_queue
- '0' will restart the server, with all data structures initialized to default

Example 2:    ./server 8080 10 10 1
- '1' will reload the server from where it was closed earlier i.e. it will reload its request_queue and hashmap data from backup

NOTE: When the server is started for the first time or after clear_all.sh the last argument should be '0'.

## Running client
./client ip port status filename/request_ID timeout program_ID

Example 1: ./client 0.0.0.0 8080 0 studentCode.cpp 20 3
- status value = 0, means this is a new request and studentCode is the file that needs to be graded
- this will send the file to the server and get request_ID from the server. The request_ID will be stored in file named "client_request_id_3.txt", this file is named based on the program_ID argument passed

Example 2: ./client 0.0.0.0 8080 1 499 20 1
- status value = 1, means this is a request to check status of the old request. So along with this request_ID should be passed instead of filename.

NOTE: When the status is '0' ('new'), send filename and not the request_ID. When the status is '1' (check), send requestID and not the filename.

## Get response time for a single client request to get completely processed
{ time ./test_client.sh $ip $port $file $timeout $i; } 2>&1 | grep "real" | awk 'BEGIN {FS="\t"} {print $2}' | awk 'BEGIN {FS="m"} {print $2}' | awk 'BEGIN {FS="s"} {print $1}' >> response$j.txt 

- This will run the client initially for a new request and then keep checking for its status periodically until the request is processed completely. The time command will give the response time for this request.

## Run multiple clients parallelly and get performance
bash analysis_and_plot_graphs.sh ip port file timeout max_client loop_num think_time

Example 1: bash analysis_and_plot_graphs.sh 0.0.0.0 8080 studentCode.cpp 20 180 5 2

- this script will run the 10, 20, 30, ... so on upto 180, clients simultaneosly and plot their average response time, throughput, utilization with increasing number of clients.

## Clear the temporary data created by client and server
bash clearall.sh


