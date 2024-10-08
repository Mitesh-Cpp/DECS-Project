#include "declarations.h"

// Contains all the functions that server needs to send and receive data to and from client

int send_response_to_client_from_file(int sockfd, string filename)
{
    request_response_packet packet;
    int bytesRead;
    int fileDescriptor = open(filename.c_str(), O_RDONLY);
    if(fileDescriptor == -1) {
        cerr << "Error opening file." << endl;
        close(sockfd);
        return -1;
    }
    
    while ((bytesRead = read(fileDescriptor, packet.packet_buffer, sizeof(packet.packet_buffer))) > 0)
    {
        packet.bytes_to_read = bytesRead;
        packet.is_last_packet = false;
        if (bytesRead < MAX_PACKET_BUFFER_SIZE)
            packet.is_last_packet = true;
        int n = send(sockfd, &packet, sizeof(packet), 0);
        if (n <= 0) {
            perror("ERROR writing to socket");
            close(sockfd);  
            return -1;  
        }
        if (n < 0)
        {
            perror("ERROR writing to socket");
            close(sockfd);
            return -1;
        }
    }
    close(fileDescriptor);
    return 1;
}

int send_status_to_client(int sockfd, const char *msg, bool is_last_packet)
{
    request_response_packet packet;
    strncpy(packet.packet_buffer, msg, sizeof(packet.packet_buffer));
    packet.bytes_to_read = sizeof(packet.packet_buffer);
    packet.is_last_packet = is_last_packet;
    int n = write(sockfd, &packet, sizeof(packet));
    if(n <= 0) {
        cerr << "Error writing status.";
        close(sockfd);
        return -1;
    }
    return 1;
}

int receive_file_from_client_into_file(int sockfd, string filename)
{
    request_response_packet packet;
    int fileDescriptor = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);
    while (true)
    {
        int n = recv(sockfd, &packet, sizeof(packet), 0);
        if(n <= 0) {
            cerr << "Error receiving file.";
            close(sockfd);
            return -1;
        }
        int wroteBytes = write(fileDescriptor, packet.packet_buffer, packet.bytes_to_read);
        if (packet.is_last_packet)
            break;
        memset(packet.packet_buffer, 0, sizeof(packet.packet_buffer));
    }
    close(fileDescriptor);
    return 1;
}

void *handle_client(void *arg)
{

    while (1)
    {
        int newsockfd;
        pthread_mutex_lock(&queue_lock);
        while (newsockfd_queue.empty())
        {
            pthread_cond_wait(&queue_empty, &queue_lock); // wait if queue empty
        }
        newsockfd = newsockfd_queue.front();
        newsockfd_queue.pop();
        pthread_mutex_unlock(&queue_lock);
        
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

        if(receive_file_from_client_into_file(newsockfd, string(source_file)) < 0)
            continue;

        int compiling = system(compile_command.c_str());
        if (compiling != 0)
        {
            if(send_status_to_client(newsockfd, COMPILER_ERROR_MSG, false) < 0)
                continue;
            if(send_response_to_client_from_file(newsockfd, string(compiler_error_file)) < 0)
                continue;
            system(remove_source_file_command.c_str());
            system(remove_compiler_error_file_command.c_str());
        }
        else
        {
            int runTheFile = system(run_command.c_str());
            if (runTheFile != 0)
            {
                send_status_to_client(newsockfd, RUNTIME_ERROR_MSG, true);
            }
            else
            {
                int difference = system(compare_command.c_str());
                if (difference != 0)
                {
                    send_status_to_client(newsockfd, OUTPUT_ERROR_MSG, false);
                    send_response_to_client_from_file(newsockfd, string(diff_file));
                }
                else
                {
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

    return NULL;
}

void *average_queue_size(void *arg) {
    while (1) {
        sleep(2);
        cout << "Queue Size: " << newsockfd_queue.size() << endl;
    }
}