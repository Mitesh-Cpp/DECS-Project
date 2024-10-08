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
int program_id;
string sourceCode_reqID;

void error(const char *msg)
{
    perror(msg);
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
    if (file == nullptr) {
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
    while (file_size > 0) {
        int chunk_size = fread(packet.packet_buffer, 1, sizeof(packet.packet_buffer), file);
        packet.bytes_to_read = chunk_size;
        if(chunk_size < MAX_PACKET_BUFFER_SIZE)
            packet.is_last_packet = true;
        if (chunk_size == 0) {
            break;  
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
    return (total_bytes_sent <= 0) ? -1 : 0;
}
int send_requestID_to_server(int sockfd, int request_ID)
{
    status_packet requestID;
    requestID.type = 0;
    requestID.request_id = request_ID;
    requestID.position_in_queue = 0;

    int bytes_wrote = write(sockfd,&requestID,sizeof(requestID));
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

void *submit(void *status_type)
{
    // --------------------- SOCKET CONNECT START HERE ----------------------

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        cerr << "ERROR opening socket" << endl;
        return (void *)-1;
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
        return (void *)-1;
    }

    // -------------------------- SOCKET CONNECT END HERE ------------------------

    // ------------------------- 1ST SEND STATUS TO SERVER --------------------------
   
    status_packet status_p;
    status_p.type = status;
    status_p.request_id=0;
    status_p.position_in_queue=0;
    int bytes_wrote = write(sockfd, &status_p, sizeof(status_p));
    if (bytes_wrote<=0)
    {
        cerr << "Error: Unable to send status to the server";
    }

    // ----------------- 2ND CHECK STATUS OF THE REQUEST AND TAKE RESPECTIVE ACTION ----------------------

    // if status is "new" just send file to the server and end ----------
    if (!status)
    {
        if (send_code_to_server_from_file(sockfd, sourceCode_reqID) != 0)
        {
            close(sockfd);
            cerr << "Error sending file." << endl;
            return (void *)-1;
        }
    }
    // if status is "check" send requestID to the server ------------
    else
    {
        if (send_requestID_to_server(sockfd, atoi(sourceCode_reqID.c_str())) != 0)
        {
            close(sockfd);
            cerr << "Error sending requestID." << endl;
            return (void *)-1;
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

    int bytes_read = read(sockfd,&status_p,sizeof(status_p));
    switch (status_p.type)
    {
    case -1:
        cout<<"Request ID invalid. Please resend."<<endl;
        break;
    case 0:
        cout<<"Request is in queue at position "<<status_p.position_in_queue<<"."<<endl;
        break;
    case 1:
        cout<<"Request is being processed by a thread."<<endl;
        break;
    case 2:
        cout<<"Request is processed successfully."<<endl;
        if(receive_response_from_server(sockfd)!=0){
            cout<<"Error: Unable to receive output file from server"<<endl;
        }
        break;
    case 3:
        cout<<"File accepted and request pushed in queue"<<endl;
        char request_ID_file_command[50];
        sprintf(request_ID_file_command,"echo %d > ./client_files/client_request_id_%d.txt",status_p.request_id,program_id);
        system(request_ID_file_command);
        break;
    default:
        cout<<"Error: wrong status received from server"<<endl;
        break;
    }


    close(sockfd);
    int *type = (int*)malloc(sizeof(int));
    *type = status_p.type;
    return type;
}

int main(int argc, char *argv[])
{
    // --------------------- SOCKET SETUP -----------------------------------

    int sockfd, portno, n;
    struct hostent *server;
    char *buffer = nullptr;

    if (argc != 7)
    {
        cerr << "Usage: ./client ip-addr port status studentCode.cpp|requestID timeoutSeconds programID" << endl;
        exit(1);
    }

    portno = atoi(argv[2]);

    status = atoi(argv[3]);
    sourceCode_reqID = argv[4];
    int timeoutSeconds = atoi(argv[5]);
    program_id = atoi(argv[6]);

    server = gethostbyname(argv[1]);
    if (server == nullptr)
    {
        cerr << "ERROR, no such host" << endl;
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    // --------------------- SOCKET SETUP FINISH -----------------------------------

    
    // --------------------- SEND NEW REQUEST AND KEEP CHECKING THE STATUS --------------------------


    struct timeval start, end, timeout;
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;
    struct timespec ts;
    pthread_t worker;

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        exit(1);
    ts.tv_sec += timeoutSeconds;

    
    // --------------------- SEND REQUEST THOUGH THREAD --------------------------
    pthread_create(&worker, NULL, submit, NULL);



    
    // --------------------- GET STATUS THOUGH THREAD JOIN --------------------------
    int *status_type;
    if (pthread_timedjoin_np(worker, (void**)&status_type, &ts) != 0)
    {
        pthread_detach(worker);
        cout << "\nTIME OUT" << endl;
        exit(1);
    }
    else
    {
        gettimeofday(&end, NULL);
        int request_status = *(int*)status_type;
        free(status_type);
        switch (request_status)
        {
        case 2:
            exit(3);
            break;
        
        default:
            exit(1);
            break;
        } 
        
        
    }

    return 0;
}