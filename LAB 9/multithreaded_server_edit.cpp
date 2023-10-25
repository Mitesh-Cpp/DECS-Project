#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

using namespace std;

#define MAX_PACKET_BUFFER_SIZE 1024

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

void error(const char *msg) {
    // perror(msg);
    // exit(EXIT_FAILURE);
    cerr << msg <<endl;
    pthread_exit(NULL);
}

struct request_response_packet {
    bool is_last_packet;
    int bytes_to_read;
    char packet_buffer[MAX_PACKET_BUFFER_SIZE];
};

void send_response_to_client_from_file(int sockfd, string filename) {
    request_response_packet packet;
    int bytesRead;
    int fileDescriptor = open(filename.c_str(), O_RDONLY);
    while ((bytesRead = read(fileDescriptor, packet.packet_buffer, sizeof(packet.packet_buffer))) > 0) {
        packet.bytes_to_read = bytesRead;
        packet.is_last_packet = false;
        if (bytesRead < MAX_PACKET_BUFFER_SIZE)
            packet.is_last_packet = true;
        int n = write(sockfd, &packet, sizeof(packet));
        if (n < 0) {
            error("ERROR writing to socket");
        }
    }
}

void send_status_to_client(int sockfd, const char *msg, bool is_last_packet) {
    request_response_packet packet;
    strncpy(packet.packet_buffer, msg, sizeof(packet.packet_buffer));
    packet.bytes_to_read = sizeof(packet.packet_buffer);
    packet.is_last_packet = is_last_packet;
    int n = write(sockfd, &packet, sizeof(packet));
    if (n < 0) {
        error("ERROR writing to socket");
    }
}

void receive_file_from_client_into_file(int sockfd, string filename) {
    request_response_packet packet;
    int fileDescriptor = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);
    while (true) {
        int n = read(sockfd, &packet, sizeof(packet));
        if (n < 0) {
            error("ERROR writing to socket");
        }
        int wroteBytes = write(fileDescriptor, packet.packet_buffer, packet.bytes_to_read);
        if (packet.is_last_packet)
            break;
        memset(packet.packet_buffer, 0, sizeof(packet.packet_buffer));
    }
}

void* handle_client(void* newsockfd_ptr) {
    int newsockfd = *((int*)newsockfd_ptr);
    // Generate unique filenames based on the thread ID
    pthread_t self_thread = pthread_self();
    char source_file[100], executable[100], output_file[100], compiler_error_file[100], runtime_error_file[100], diff_file[100];
    sprintf(source_file, "./submissions/gradeFileAtServer_%lu.cpp", self_thread);
    sprintf(executable, "./executables/gradeThisFile_%lu", self_thread);
    sprintf(output_file, "./outputs/output_%lu.txt", self_thread);
    sprintf(compiler_error_file, "./compiler_error/Cerror_%lu.txt", self_thread);
    sprintf(runtime_error_file, "./runtime_error/runtime_error_%lu.txt", self_thread);
    sprintf(diff_file, "./diffs/diff_%lu.txt", self_thread);

    string compile_command = "g++ " + string(source_file) + " -o " + string(executable) + " > /dev/null 2> " + string(compiler_error_file);
    string run_command = "./" + string(executable) + " > " + string(output_file) + " 2> " + string(runtime_error_file);
    string compare_command = "diff " + string(output_file) + " " + string(EXPECTED_OUTPUT) + " > " + string(diff_file);

    
    string remove_source_file_command = "rm " + string(source_file);
    string remove_executable_command = "rm " + string(executable);
    string remove_output_file_command = "rm " + string(output_file);
    string remove_compiler_error_file_command = "rm " + string(compiler_error_file);
    string remove_runtime_error_file_command = "rm " + string(runtime_error_file);
    string remove_diff_file_command = "rm " + string(diff_file);
    

    receive_file_from_client_into_file(newsockfd, string(source_file));

    int compiling = system(compile_command.c_str());
    if (compiling != 0) {
        send_status_to_client(newsockfd, COMPILER_ERROR_MSG, false);
        send_response_to_client_from_file(newsockfd, string(compiler_error_file));
        system(remove_source_file_command.c_str());
        system(remove_compiler_error_file_command.c_str());
    } else {
        int runTheFile = system(run_command.c_str());
        if (runTheFile != 0) {
            send_status_to_client(newsockfd, RUNTIME_ERROR_MSG, true);
        } else {
            int difference = system(compare_command.c_str());
            if (difference != 0) {
                send_status_to_client(newsockfd, OUTPUT_ERROR_MSG, false);
                send_response_to_client_from_file(newsockfd, string(diff_file));
            } else {
                send_status_to_client(newsockfd, PASS_MSG, true);
            }
            system(remove_diff_file_command.c_str());
        }

        system(remove_source_file_command.c_str());
        system(remove_executable_command.c_str());
        system(remove_output_file_command.c_str());
        system(remove_runtime_error_file_command.c_str());
        system(remove_compiler_error_file_command.c_str());
    }
    
    close(newsockfd);
    delete (int*)newsockfd_ptr;
    return NULL;
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    if (argc != 2) {
        cerr << "USAGE: ./server port\n";
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    cout << "1" << endl;
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    portno = atoi(argv[1]);
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, reinterpret_cast<struct sockaddr *>(&serv_addr), sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 3000);
    clilen = sizeof(cli_addr);
cout << "2" << endl;
    while (1) {
        cout << "6" << endl;
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if (newsockfd < 0){
            cerr << "ERROR on accept" << endl;
            continue;
        }
cout << "3" << endl;
        // Create a new thread using pthread_create
        pthread_t worker_thread;
        int* newsockfd_ptr = new int; // Dynamically allocate memory to pass the socket descriptor
        *newsockfd_ptr = newsockfd;
        int thread_create_result = pthread_create(&worker_thread, NULL, handle_client, newsockfd_ptr);
cout << "4" << endl;
        if (thread_create_result != 0) {
            cerr << "ERROR creating thread: " << thread_create_result << endl;
            delete newsockfd_ptr; // Free the allocated memory if thread creation fails
        }
        cout << "5" << endl;
    }

    return 0;
}
