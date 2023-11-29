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
#include <bits/stdc++.h>
#include <fstream>

using namespace std;

#define MAX_PACKET_BUFFER_SIZE 1024
#define MAX_REQID_SIZE 4
#define MAX_REQUESTID_SET_SIZE 500

void error(const char *msg);
int send_response_to_client_from_file(int sockfd, string filename);
int send_status_to_client(int sockfd, const char *msg, bool is_last_packet);
int receive_file_from_client_into_file(int sockfd, string filename);
void *handle_client(void *arg);
void *average_queue_size(void *arg);
void retrive_backup();

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

pthread_mutex_t queue_lock; 
pthread_cond_t queue_empty; 

pthread_mutex_t request_queue_lock;
pthread_cond_t request_queue_empty;

pthread_mutex_t set_lock;
pthread_cond_t set_empty;

pthread_mutex_t hashmap_lock;

queue<int> newsockfd_queue;
queue<int> request_queue;
unordered_map<int, int> status_hashmap;
unordered_set<int> requestID_set;

pthread_mutex_t generator_lock;
int random_no_generator;


const char SUBMISSIONS_DIR[] = "./submissions/";
const char EXECUTABLES_DIR[] = "./executables/";
const char OUTPUTS_DIR[] = "./outputs/";
const char DIFF_DIR[] = "./diffs/";
const char COMPILER_ERROR_DIR[] = "./compiler_error/";
const char RUNTIME_ERROR_DIR[] = "./runtime_error/";
const char BACKUP_DIR[] = "./backup/";
const char EXPECTED_OUTPUT[] = "./expected_output.txt";
const char PASS_MSG[] = "PASS\n";
const char COMPILER_ERROR_MSG[] = "COMPILER ERROR\n";
const char RUNTIME_ERROR_MSG[] = "RUNTIME ERROR\n";
const char OUTPUT_ERROR_MSG[] = "OUTPUT ERROR\n";
