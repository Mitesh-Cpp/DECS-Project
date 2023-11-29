Performance Testing parameters like ip_address of server, port number of server, code_file to be graded, sleepTime, loopNum, TimeOut Seconds are hardcoded in file "analysis_and_plot_graphs.sh"
Can change from there, if needed

(Run clearall.sh: "bash clearall.sh" before every performance test to clear all the previous data)

Sequence for Performance Testing:

    1. Compile Server: g++ multithreaded_server.cpp -o server

    2. Compile Client: g++ client.cpp -o client

    3. Run Server: ./server <port>  (2000 is default port value which is hardcoded in bash file)

    4. Run analysis_and_plot_graph.sh: bash analysis_and_plot_graph.sh (This will take a few minutes)

The results will be stored in response_time.png, throughput.png, timeout_rate.png, request_rate.png, nlwp.png, goodput_data.png, error_rate.png, cpu_util.png

# Comparison Graphs for single threaded, create-destroy thread pool, multi-threaded server

The analysis was done on various servers using the "analysis_and_plot_graphs.sh" script.
The generated data in the text files was appended for all different types of servers and
can be seen in "graph_plot_data_text_files" folder. (19 entries per different server)

This data was processsed using "Graph_Plotter/compare_graphs.py" python code to generate suitable comparison graphs. These comparison graphs can be found on the root directory in the folder named "Comparison Graphs".
    Average Queue Size graph can also be found there.