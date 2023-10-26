#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <iomanip>

using namespace std;

#define MAX_PACKET_BUFFER_SIZE 1024


struct request_response_packet {
    bool is_last_packet;
    int bytes_to_read;
    char packet_buffer[MAX_PACKET_BUFFER_SIZE];
    // bool is_last_loop;
};

int send_code_to_server_from_file(int sockfd, string code_file) {
    // Open the student code file
    int total_bytes_sent = 0;
    FILE *file = fopen(code_file.c_str(), "r");
    if (file == nullptr) {
        fclose(file);
        return -2;
        // error("ERROR opening file");
    }
    long file_size;
    // Calculate the file size
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    // Send the content of the file to the server in chunks
    request_response_packet packet;
    packet.is_last_packet = false;
    // packet.is_last_loop = is_last_loop;
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
        total_bytes_sent += n;
        if (n < 0) {
            fclose(file);
            return -1;
        }
        file_size -= chunk_size;
    }
    fclose(file);
    return total_bytes_sent;
}

int receive_response_from_server(int sockfd) {
    int total_bytes_read = 0;
    request_response_packet packet;
    cout<<"SERVER RESPONSE: \n";
    while(true) {
        int n = read(sockfd, &packet, sizeof(packet));
        total_bytes_read += n;
        if (n <= 0) {
            // error("ERROR reading from socket");
            return -1;
        }
        cout<<packet.packet_buffer;
        //int wroteBytes = write(STDOUT_FILENO, packet.packet_buffer, packet.bytes_to_read);
        if(packet.is_last_packet)
            break;
    }
    return total_bytes_read;
}

int main(int argc, char *argv[]) {
    // ./submit  <serverIP:port>  <sourceCodeFileTobeGraded>  <loopNum> <sleepTimeSeconds>
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char *buffer = nullptr;

    if (argc != 7) {
        cerr << "Usage: ./client ip-addr port studentCode.cpp loopNum sleepTimeSeconds timeoutSeconds" << endl;
        exit(0);
    }

    portno = atoi(argv[2]);
    string studentCode = argv[3];
    int loopNum = atoi(argv[4]);
    int sleepTimeSeconds = atoi(argv[5]);
    int timeoutSeconds = atoi(argv[6]);
    int num_error = 0;
    

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

    
    
    struct timeval start, end, loopStart, loopEnd, timeout;
    long total_response_time = 0;
    long total_loop_time = 0;
    int successful_responses = 0, numTimeouts = 0;
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;

    gettimeofday(&loopStart, NULL);
    num_error = 0;
    for (int i = 0; i < loopNum; i++) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            cerr << "ERROR opening socket" << endl;
            num_error ++;
            continue;
            }
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            cerr << "Error setting receive timeout." << endl;
            num_error ++;
            close(sockfd);
            continue;
        }
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
		        cerr << "Connection timeout." << endl;
		        numTimeouts++;
		    } 
            else {
		        cerr << "Error connecting to the server." << endl;
		        num_error ++;
		    }
		    close(sockfd);
		    continue; // Continue to the next iteration of the loop
	    }
        // cout<<"here";
        int bytesSent = send_code_to_server_from_file(sockfd, studentCode);
        if (bytesSent < 0) {
		    // Check if the error was due to a timeout
            if(bytesSent == -1) {
		        cerr << "Error Writing file." << endl;
                num_error ++; 
            }
            else if(bytesSent == -2) {
                cerr << "Error Opening file." << endl;
                num_error ++;
            }
            else {
		        cerr << "Error sending data." << endl;
		        num_error ++;
		    }
		    close(sockfd);
		    continue; // Continue to the next iteration of the loop
	    }
        gettimeofday(&start, NULL);


        int bytesRead = receive_response_from_server(sockfd);

        if (bytesRead == -1) {
		    // Check if the error was due to a timeout
		    if (errno == EAGAIN || errno == EWOULDBLOCK) {
		        cerr << "Receive timeout." << endl;
		        numTimeouts++;
		    } 
            else {
		        cerr << "Error receiving data." << endl;
		        num_error ++;
		    }
		    close(sockfd);
		    continue;
	    }
        else {
            // Incrementing the No. Successful Response
           successful_responses ++;
        }

        gettimeofday(&end, NULL);  // Capture the end time here
        total_response_time += (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
        
        close(sockfd);
        sleep(sleepTimeSeconds);
    }

   
    gettimeofday(&loopEnd, NULL);
    total_loop_time = (loopEnd.tv_sec - loopStart.tv_sec) * 1000000 + (loopEnd.tv_usec - loopStart.tv_usec);
    /*
    cout << "Average Response Time: " << total_response_time/successful_responses << " microseconds , Throughput: " << successful_responses * 1000000.0 / total_response_time;
    cout << ", Successful Responses: " << successful_responses;
    cout << ", Time taken for completing the Loop: " << total_loop_time <<" microseconds";
    */
   cout<<"\n";
    cout << "Average Response Time (seconds):" << total_response_time/(1000000.0 * successful_responses) << endl;
    cout << "Number of Successful Responses:" << successful_responses << endl;
    cout << "Time for Completing the Loop (seconds):" << total_loop_time/1000000.0 << endl;
    //cout << "Request sent rate:" << 1.0*loopNum/total_loop_time << endl;
    //cout << "Successful request rate:"<< 1.0*successful_responses/total_loop_time <<endl;
    
    cout << "Request sent rate: " << fixed << setprecision(10) << 1.0 * loopNum / total_loop_time << endl;
    cout << "Successful request rate: " << fixed << setprecision(10) << 1.0 * successful_responses / total_loop_time << endl;

    
    cout << "Timeout rate:"<< 1.0*numTimeouts/total_loop_time<<endl;
    cout << "Error rate:"<<1.0*(num_error)/total_loop_time << endl;

    return 0;
}
