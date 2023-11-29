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
#define MAX_TRIES 5
#define MAX_REQID_SIZE 4

struct sockaddr_in serv_addr;
int status;
string sourceCode;

void error(const char *msg)
{
    perror(msg);
    // exit(0);
}

struct status_packet
{
    int type;
    int request_id;
    int position_in_queue;
};
struct request_response_packet
{
    bool is_last_packet;
    int bytes_to_read;
    char packet_buffer[MAX_PACKET_BUFFER_SIZE];
};

int send_code_to_server_from_file(int sockfd, string code_file)
{
    // Open the student code file
    cout << code_file << endl;
    int total_bytes_sent = 0;
    FILE *file = fopen(code_file.c_str(), "r");
    if (file == nullptr)
    {
        fclose(file);
        return -2;
    }
    long file_size;
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    // Send the content of the file to the server in chunks
    request_response_packet packet;
    packet.is_last_packet = false;
    while (file_size > 0)
    {
        int chunk_size = fread(packet.packet_buffer, 1, sizeof(packet.packet_buffer), file);
        packet.bytes_to_read = chunk_size;
        if (chunk_size < MAX_PACKET_BUFFER_SIZE)
            packet.is_last_packet = true;
        if (chunk_size == 0)
        {
            break;
        }
        packet.bytes_to_read = chunk_size;
        int n = write(sockfd, &packet, sizeof(packet));
        total_bytes_sent += n;
        if (n < 0)
        {
            fclose(file);
            return -1;
        }
        file_size -= chunk_size;
    }
    fclose(file);
    return (total_bytes_sent <= 0) ? -1 : 0;
}
int send_requestID_to_server(int sockfd, int request_ID)
{
    status_packet requestID;
    requestID.type = 0;
    requestID.request_id = request_ID;
    requestID.position_in_queue = 0;

    int bytes_wrote = write(sockfd, &requestID, sizeof(requestID));
    if (bytes_wrote <= 0)
    {
        return -1;
    }
    return 0;
}

int receive_response_from_server(int sockfd)
{
    int total_bytes_read = 0;
    request_response_packet packet;
    cout << "SERVER RESPONSE: \n";
    while (true)
    {
        int n = recv(sockfd, &packet, sizeof(packet), 0);
        total_bytes_read += n;
        if (n <= 0)
        {
            return -1;
        }
        cout << packet.packet_buffer;
        // int wroteBytes = write(STDOUT_FILENO, packet.packet_buffer, packet.bytes_to_read);
        if (packet.is_last_packet)
            break;
    }
    fflush(stdout);
    return 0;
}

int submit(int reqID)
{
    // --------------------- SOCKET CONNECT START HERE ----------------------

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        cerr << "ERROR opening socket" << endl;
        return -1;
    }

    int tries = 0;
    while (tries < MAX_TRIES)
    {
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0)
        {
            break;
        }
        sleep(1);
        tries += 1;
    }
    if (tries == MAX_TRIES)
    {
        cerr << "Error connecting to the server." << endl;
        return -1;
    }

    // -------------------------- SOCKET CONNECT END HERE ------------------------

    // ------------------------- 1ST SEND STATUS TO SERVER --------------------------

    status_packet status_p;
    status_p.type = status;
    status_p.request_id = 0;
    status_p.position_in_queue = 0;
    int bytes_wrote = write(sockfd, &status_p, sizeof(status_p));
    if (bytes_wrote <= 0)
    {
        cerr << "Error: Unable to send status to the server";
    }

    // ----------------- 2ND CHECK STATUS OF THE REQUEST AND TAKE RESPECTIVE ACTION ----------------------

    // if status is "new" just send file to the server and end ----------
    if (!status)
    {
        if (send_code_to_server_from_file(sockfd, sourceCode) != 0)
        {
            close(sockfd);
            cerr << "Error sending file." << endl;
            return -1;
        }
    }
    // if status is "check" send requestID to the server ------------
    else
    {
        if (send_requestID_to_server(sockfd, reqID) != 0)
        {
            close(sockfd);
            cerr << "Error sending requestID." << endl;
            return -1;
        }
    }

    // ------------------------- 3RD RECEIVE REQUEST STATUS FROM SERVER --------------------------
    // STATUS NO AND MESSAGE MATCHING:
    //
    // | Status No   |      Message                                      |
    // |_____________|___________________________________________________|
    // |    -1       |   Request ID invalid                              |
    // |     0       |   Request in Queue at some position               |
    // |     1       |   Request out of Queue, picked by a thread        |
    // |     2       |   Request Processing Done                         |
    // |     3       |   File accepted and request ID pushed in queue    |

    int bytes_read = read(sockfd, &status_p, sizeof(status_p));
    switch (status_p.type)
    {
    case -1:
        cout << "Request ID invalid. Please resend." << endl;
        break;
    case 0:
        cout << "Request is in queue at position " << status_p.position_in_queue << "." << endl;
        break;
    case 1:
        cout << "Request is being processed by a thread." << endl;
        break;
    case 2:
        cout << "Request is processed successfully." << endl;
        if (receive_response_from_server(sockfd) != 0)
        {
            cout << "Error: Unable to receive output file from server" << endl;
        }
        break;
    case 3:
        cout << "File accepted and request pushed in queue" << endl;
        return status_p.request_id;
        break;
    default:
        cout << "Error: wrong status received from server" << endl;
        break;
    }
    close(sockfd);
    return status_p.type;
}

int main(int argc, char *argv[])
{
    // --------------------- SOCKET SETUP -----------------------------------

    int sockfd, portno, n;
    struct hostent *server;
    char *buffer = nullptr;

    if (argc != 6)
    {
        cerr << "Usage: ./client ip-addr port status studentCode.cpp|requestID timeoutSeconds" << endl;
        exit(0);
    }

    portno = atoi(argv[2]);
    status = atoi(argv[3]);
    sourceCode = argv[4];
    int timeoutSeconds = atoi(argv[5]);

    server = gethostbyname(argv[1]);
    if (server == nullptr)
    {
        cerr << "ERROR, no such host" << endl;
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    // --------------------- SOCKET SETUP FINISH -----------------------------------

    
    // --------------------- SEND NEW REQUEST AND KEEP CHECKING THE STATUS --------------------------

    struct timeval start, end;
    long total_response_time = 0;
    int request_status;

    gettimeofday(&start, NULL);
    // Send a new request -----------
    status = 0;
    int request_ID = submit(0);
    cout << "Request ID : " << request_ID << endl;
    // Keep sending check request ---------
    while (1)
    {   
        sleep(5);
        status = 1;
        request_status = submit(request_ID);
        // If request status is 2 (i.e. Processing done)
        if (request_status == 2) 
        {
            break;
        }
        if (request_status == -1)
        {
            cout<<"Some error occured"<<endl;
            break;
        }
        
    }
    gettimeofday(&end, NULL);


    // --------------------- RESPONSE TIME CALCULATION -----------------------------------

    total_response_time += (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    cout<<"Response time = "<<total_response_time*1.0/1000000.0<<endl;

    return 0;
}