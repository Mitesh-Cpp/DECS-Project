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
  perror(msg);
  exit(EXIT_FAILURE);
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

void send_status_to_client(int sockfd, const char* msg, bool is_last_packet) {
    request_response_packet packet;
    strncpy(packet.packet_buffer, msg, sizeof(packet.packet_buffer));
    packet.bytes_to_read = strlen(msg);
    packet.is_last_packet = is_last_packet;
    int n = write(sockfd, &packet, sizeof(packet));
}

void receive_file_from_client_into_file(int sockfd, string filename) {
    request_response_packet packet;
    int fileDescriptor = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);
    while (true) {
        int n = read(sockfd, &packet, sizeof(packet));
        int wroteBytes = write(fileDescriptor, packet.packet_buffer, packet.bytes_to_read);
        if (packet.is_last_packet)
            break;
        memset(packet.packet_buffer, 0, sizeof(packet.packet_buffer));
    }
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
    /* create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    portno = atoi(argv[1]);
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, reinterpret_cast<struct sockaddr *>(&serv_addr), sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 1000);
    clilen = sizeof(cli_addr);

    while (1) {
        try {
            if (mkdir(SUBMISSIONS_DIR, S_IRWXU) != 0 && errno != EEXIST) {
                error("Error creating submissions directory");
            }
            if (mkdir(EXECUTABLES_DIR, S_IRWXU) != 0 && errno != EEXIST) {
                error("Error creating executables directory");
            }
            if (mkdir(OUTPUTS_DIR, S_IRWXU) != 0 && errno != EEXIST) {
                error("Error creating outputs directory");
            }
            if (mkdir(COMPILER_ERROR_DIR, S_IRWXU) != 0 && errno != EEXIST) {
                error("Error creating compiler error directory");
            }
            if (mkdir(RUNTIME_ERROR_DIR, S_IRWXU) != 0 && errno != EEXIST) {
                error("Error creating runtime error directory");
            }
            if (mkdir(DIFF_DIR, S_IRWXU) != 0 && errno != EEXIST) {
                error("Error creating diffs directory");
            }
        } catch (const std::exception& e) {
            cerr << "Error creating directories: " << e.what() << "\n";
            close(sockfd);
            return -1;
        }

        string source_file = SUBMISSIONS_DIR + string("gradeFileAtServer.cpp");
        string executable = EXECUTABLES_DIR + string("gradeThisFile");
        string output_file = OUTPUTS_DIR + string("output.txt");
        string compiler_error_file = COMPILER_ERROR_DIR + string("Cerror.txt");
        string runtime_error_file = RUNTIME_ERROR_DIR + string("runtime_error.txt");
        string diff_file = DIFF_DIR + string("diff.txt");

        string compile_command = "g++ " + source_file + " -o " + executable + " > /dev/null 2> " + compiler_error_file;
        string run_command = "./" + executable + " > " + output_file + " 2> " + runtime_error_file;
        string compare_command = "diff " + output_file + " " + EXPECTED_OUTPUT + " > " + diff_file;

        string remove_source_file_command = "rm " + source_file;
        string remove_executable_command = "rm " + executable;
        string remove_output_file_command = "rm " + output_file;
        string remove_compiler_error_file_command = "rm " + compiler_error_file;
        string remove_runtime_error_file_command = "rm " + runtime_error_file;
        string remove_diff_file_command = "rm " + diff_file;

        newsockfd = accept(sockfd, reinterpret_cast<struct sockaddr *>(&cli_addr), &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        receive_file_from_client_into_file(newsockfd, source_file);

        int compiling = system(compile_command.c_str());
        if (compiling != 0) {
            send_status_to_client(newsockfd, COMPILER_ERROR_MSG, false);
            send_response_to_client_from_file(newsockfd, compiler_error_file);
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
                    send_response_to_client_from_file(newsockfd, diff_file);
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
    }

    if (n < 0)
        error("ERROR writing to socket");

    return 0;
}