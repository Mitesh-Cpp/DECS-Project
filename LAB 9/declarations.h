#include <iostream>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <cstring>
#include <string>
#include <pthread.h>
#include <queue> //P

using namespace std;

#define MAX_PACKET_BUFFER_SIZE 1024

void error(const char *msg);
int send_response_to_client_from_file(int sockfd, string filename);
int send_status_to_client(int sockfd, const char *msg, bool is_last_packet);
int receive_file_from_client_into_file(int sockfd, string filename);
void *handle_client(void *arg);
void *average_queue_size(void *arg);

struct request_response_packet
{
    bool is_last_packet;
    int bytes_to_read;
    char packet_buffer[MAX_PACKET_BUFFER_SIZE];
};

pthread_mutex_t queue_lock; 
pthread_cond_t queue_empty; 

queue<int> newsockfd_queue; 

const char SUBMISSIONS_DIR[] = "./submissions/";
const char EXECUTABLES_DIR[] = "./executables/";
const char OUTPUTS_DIR[] = "./outputs/";
const char DIFF_DIR[] = "./diffs/";
const char COMPILER_ERROR_DIR[] = "./compiler_error/";
const char RUNTIME_ERROR_DIR[] = "./runtime_error/";
const char EXPECTED_OUTPUT[] = "./expected_output.txt";

const char PASS_MSG[] = "PASS\n";
const char COMPILER_ERROR_MSG[] = "COMPILER ERROR\n";
const char RUNTIME_ERROR_MSG[] = "RUNTIME ERROR\n";
const char OUTPUT_ERROR_MSG[] = "OUTPUT ERROR\n";