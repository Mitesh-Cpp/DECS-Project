Performance Testing parameters like ip_address of server, port number of server, code_file to be graded, sleepTime, loopNum, TimeOut Seconds are hardcoded in file "analysis_and_plot_graphs.sh"
Can change from there, if needed

(Run clearall.sh: "bash clearall.sh" before every performance test to clear all the previous data)

Sequence for Performance Testing:

    1. Compile Server: g++ multithreaded_server.cpp -o server

    2. Compile Client: g++ client.cpp -o client

    3. Run Server: ./server <port>  (2000 is default port value which is hardcoded in bash file)

    4. Run analysis_and_plot_graph.sh: bash analysis_and_plot_graph.sh (This will take a few minutes)

The results will be stored in response_time_plot.png and throughput_plot.png