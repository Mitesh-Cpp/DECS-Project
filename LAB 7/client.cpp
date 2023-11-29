#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>

using namespace std;

#define MAX_PACKET_BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(0);
}

struct request_response_packet {
    bool is_last_packet;
    int bytes_to_read;
    char packet_buffer[MAX_PACKET_BUFFER_SIZE];
};

void send_code_to_server_from_file(int sockfd, string code_file) {
    // Open the student code file
    FILE *file = fopen(code_file.c_str(), "r");
    if (file == nullptr) {
        error("ERROR opening file");
    }
    long file_size;
    // Calculate the file size
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    // Send the content of the file to the server in chunks
    request_response_packet packet;
    packet.is_last_packet = false;
    while (file_size > 0) {
        int chunk_size = fread(packet.packet_buffer, 1, sizeof(packet.packet_buffer), file);
        packet.bytes_to_read = chunk_size;
        if(chunk_size < MAX_PACKET_BUFFER_SIZE)
            packet.is_last_packet = true;
        if (chunk_size == 0) {
            break;  // Reached end of file
        }
        packet.bytes_to_read = chunk_size;
        int n = write(sockfd, &packet, sizeof(packet));
        if (n < 0) {
            error("ERROR writing to socket");
        }
        file_size -= chunk_size;
    }
    fclose(file);
}

bool receive_response_from_server(int sockfd) {
    request_response_packet packet;
    cout<<"SERVER RESPONSE: \n";
    while(true) {
        int n = read(sockfd, &packet, sizeof(packet));
        if (n <= 0) {
            error("ERROR reading from socket");
            return false;
        }
        int wroteBytes = write(STDOUT_FILENO, packet.packet_buffer, packet.bytes_to_read);
        fflush(stdout);
        if(packet.is_last_packet)
            break;
    }
    return true;
}

int main(int argc, char *argv[]) {
    // ./client  <serverIP> <port>  <sourceCodeFileTobeGraded>  <loopNum> <sleepTimeSeconds>
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char *buffer = nullptr;

    if (argc != 6) {
        cerr << "Usage: ./client ip-addr port studentCode.cpp loopNum sleepTimeSeconds" << endl;
        exit(0);
    }

    portno = atoi(argv[2]);
    string studentCode = argv[3];
    int loopNum = atoi(argv[4]);
    int sleepTimeSeconds = atoi(argv[5]);

   

    server = gethostbyname(argv[1]);
    if (server == nullptr) {
        cerr << "ERROR, no such host" << endl;
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    
    
    struct timeval start, end, loopStart, loopEnd;
    long total_response_time = 0;
    long total_loop_time = 0;
    int successful_responses = 0;

    gettimeofday(&loopStart, NULL);
    for (int i = 0; i < loopNum; i++) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            error("ERROR opening socket");
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            error("ERROR connecting");
        send_code_to_server_from_file(sockfd, studentCode);
        gettimeofday(&start, NULL);
        if(receive_response_from_server(sockfd)) {
            successful_responses ++;
        }
        gettimeofday(&end, NULL);  // Capture the end time here
        total_response_time += (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
        close(sockfd);
        sleep(sleepTimeSeconds);
    }
    gettimeofday(&loopEnd, NULL);
    total_response_time += (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    total_loop_time = (loopEnd.tv_sec - loopStart.tv_sec) * 1000000 + (loopEnd.tv_usec - loopStart.tv_usec);

    cout << "Average Response Time: " << total_response_time/successful_responses << " microseconds , Throughput: " << successful_responses * 1000000.0 / total_response_time;
    cout << " , Successful Responses: " << successful_responses;
    cout << " , Time taken for completing the Loop: " << total_loop_time <<" microseconds";
    return 0;
}