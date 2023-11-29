Sequence for Performance Testing:

    1. Compile Server: g++ server.cpp -o server

    2. Compile Client: g++ client.cpp -o client

    3. Run Server: ./server <port>

    4. Run analysis_and_plot_graph.sh: bash analysis_and_plot_graph.sh <server_ipaddress> <server_port> <codefile_to_be_graded.cpp> <loopNum> <sleepTimeSeconds>

    (can use "studentCode.cpp" as <codefile_to_be_graded.cpp> for testing purpose)

The results will be stored in response_time_plot.png and throughput_plot.png