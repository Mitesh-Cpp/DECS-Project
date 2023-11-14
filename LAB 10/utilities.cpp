// Contains all the functions that server needs to send and receive data to and from client

int find_position_in_queue(int element, queue<int> &q)
{
    pthread_mutex_lock(&request_queue_lock);
    queue<int> tempQueue = q; // Create a copy of the queue
    pthread_mutex_unlock(&request_queue_lock);
    int position = 0;
    while (!tempQueue.empty())
    {
        if (tempQueue.front() == element)
        {
            return position; // Element found, return its position
        }
        tempQueue.pop();
        ++position;
    }
    return -1; // Element not found
}

int send_response_to_client_from_file(int sockfd, string filename)
{
    request_response_packet packet;
    int bytesRead;
    cout<<filename<<endl;
    int fileDescriptor = open(filename.c_str(), O_RDONLY);
    if (fileDescriptor == -1)
    {
        cerr << "Error opening file." << endl;
        close(sockfd);
        return -1;
    }
    bzero(packet.packet_buffer, size(packet.packet_buffer));
    while ((bytesRead = read(fileDescriptor, packet.packet_buffer, sizeof(packet.packet_buffer))) > 0)
    {
        packet.bytes_to_read = bytesRead;
        packet.is_last_packet = false;
        if (bytesRead < MAX_PACKET_BUFFER_SIZE)
            packet.is_last_packet = true;
        cout << packet.packet_buffer << endl;
        int n = send(sockfd, &packet, sizeof(packet), 0);
        if (n <= 0)
        {
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
    if (n <= 0)
    {
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
        if (n <= 0)
        {
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
void append_msg_to_file(const char *msg, const char *filename)
{
    ofstream file(filename, ios::app); // Open the file in append mode
    if (file.is_open())
    {
        file << msg;
        file.close();
    }
    else
    {
        cerr << "Failed to open file for appending: " << filename << endl;
    }
}

void append_file_to_file(const char *source_filename, const char *target_filename)
{
    ifstream source_file(source_filename);
    if (source_file.is_open())
    {
        ofstream target_file(target_filename, ios::app); // Open the target file in append mode
        if (target_file.is_open())
        {
            target_file << source_file.rdbuf(); // Append the contents of the source file to the target file
            target_file.close();
        }
        else
        {
            cerr << "Failed to open target file for appending: " << target_filename << endl;
        }
        source_file.close();
    }
    else
    {
        cerr << "Failed to open source file for reading: " << source_filename << endl;
    }
}

void *handle_request(void *arg)
{
    while (1)
    {
        sleep(30);
        cout << "Hello" << endl;
        int requestID;
        pthread_mutex_lock(&request_queue_lock);
        while (request_queue.empty())
        {
            pthread_cond_wait(&request_queue_empty, &request_queue_lock); // wait if queue empty
        }
        requestID = request_queue.front();
        request_queue.pop();
        status_hashmap[requestID] = 1;
        pthread_mutex_unlock(&request_queue_lock);

        cout << "RequestID : " << requestID << endl;

        // sleep(20);
        //  Generate unique filenames based on the thread ID
        char final_output_file[100], source_file[100], executable[100], output_file[100], compiler_error_file[100], runtime_error_file[100], diff_file[100];
        sprintf(final_output_file, "./final_outputs/%d_otpt.txt", requestID);
        sprintf(source_file, "./submissions/gradeFileAtServer_%d.cpp", requestID);
        sprintf(executable, "./executables/gradeThisFile_%d", requestID);
        sprintf(output_file, "./outputs/output_%d.txt", requestID);
        sprintf(compiler_error_file, "./compiler_error/Cerror_%d.txt", requestID);
        sprintf(runtime_error_file, "./runtime_error/runtime_error_%d.txt", requestID);
        sprintf(diff_file, "./diffs/diff_%d.txt", requestID);

        string compile_command = "g++ " + string(source_file) + " -o " + string(executable) + " > /dev/null 2> " + string(compiler_error_file);
        string run_command = "./" + string(executable) + " > " + string(output_file) + " 2> " + string(runtime_error_file);
        string compare_command = "diff " + string(output_file) + " " + string(EXPECTED_OUTPUT) + " > " + string(diff_file);

        string remove_source_file_command = "rm " + string(source_file);
        string remove_executable_command = "rm " + string(executable);
        string remove_output_file_command = "rm " + string(output_file);
        string remove_compiler_error_file_command = "rm " + string(compiler_error_file);
        string remove_runtime_error_file_command = "rm " + string(runtime_error_file);
        string remove_diff_file_command = "rm " + string(diff_file);

        int compiling = system(compile_command.c_str());
        if (compiling != 0)
        {
            // copy compiler error file to output file
            append_msg_to_file(COMPILER_ERROR_MSG, final_output_file);
            append_file_to_file(compiler_error_file, final_output_file);
            // system(remove_source_file_command.c_str());
            system(remove_compiler_error_file_command.c_str());
        }
        else
        {
            int runTheFile = system(run_command.c_str());
            if (runTheFile != 0)
            {
                append_msg_to_file(RUNTIME_ERROR_MSG, final_output_file);
            }
            else
            {
                int difference = system(compare_command.c_str());
                if (difference != 0)
                {
                    append_msg_to_file(OUTPUT_ERROR_MSG, final_output_file);
                    append_file_to_file(output_file, final_output_file);
                    append_msg_to_file("\n\nOUTPUT OF DIFF COMMAND\n", final_output_file);
                    append_file_to_file(diff_file, final_output_file);
                }
                else
                {
                    append_msg_to_file(PASS_MSG, final_output_file);
                }
                system(remove_diff_file_command.c_str());
            }

            system(remove_source_file_command.c_str());
            system(remove_executable_command.c_str());
            system(remove_output_file_command.c_str());
            system(remove_runtime_error_file_command.c_str());
            system(remove_compiler_error_file_command.c_str());
        }

        status_hashmap[requestID] = 2;
    }

    return NULL;
}

void *receive_file(void *arg)
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

        // get status from server
        status_packet status_p;
        int bytes_read = read(newsockfd, &status_p, sizeof(status_p));
        if (bytes_read <= 0)
        {
            cerr << "Error reading status from client";
        }

        cout << "Status : " << status_p.type << endl;

        if (!status_p.type) // new request so generate reqID and accept file from client
        {
            // Generate requestID for this new request
            int requestID;
            pthread_mutex_lock(&set_lock);
            while (requestID_set.empty())
            {
                pthread_cond_wait(&set_empty,&set_lock);
            }
            requestID = *requestID_set.begin();
            requestID_set.erase(requestID_set.begin());
            pthread_mutex_unlock(&set_lock);

            status_p.request_id = requestID;
            cout << "RequestID : " << requestID << endl;

            char final_output_file[100], source_file[100];
            sprintf(final_output_file, "./final_outputs/%d_otpt.txt", requestID);
            sprintf(source_file, "./submissions/gradeFileAtServer_%d.cpp", requestID);

            cout << "Now receiving file from client" << endl;
            if (receive_file_from_client_into_file(newsockfd, string(source_file)) < 0)
                continue;

            cout << "File received successfully now sending status" << endl;
            pthread_mutex_lock(&request_queue_lock);
            request_queue.push(requestID);
            pthread_cond_signal(&request_queue_empty);
            pthread_mutex_unlock(&request_queue_lock);
            status_hashmap[requestID] = 0;

            status_p.type = 4;
            status_p.position_in_queue = find_position_in_queue(status_p.request_id, request_queue);
            if (write(newsockfd, &status_p, sizeof(status_p)) < 0)
                continue;

            cout << "status send successfully" << endl;
        }
        else // checking old requests status
        {
            status_packet request_status;

            status_p.type = 6;
            status_p.position_in_queue = -1;
            status_p.request_id = 0;
            
            if(newsockfd,&status_p,sizeof(status_p)<=0);
                cerr << "Sending acknowledgement to the server";
            
            //Reading Request ID from client
            bytes_read = read(newsockfd, &request_status, sizeof(request_status));
            if (bytes_read <= 0)
            {
                cerr << "Error reading Request ID from client";
            }

            if (status_hashmap.find(request_status.request_id) == status_hashmap.end()) // request id not in hashmap
            {
                request_status.type = -1;
                request_status.position_in_queue = -1;
                if (write(newsockfd, &request_status, sizeof(request_status)) < 0)
                    continue;
            }
            else
            {
                cout << "request ID : " << request_status.request_id << endl;
                int status = status_hashmap[request_status.request_id];

                if (status == 0)
                {
                    request_status.type = 0;
                    request_status.position_in_queue = find_position_in_queue(request_status.request_id, request_queue);
                    if (write(newsockfd, &request_status, sizeof(request_status)) < 0)
                        continue;
                }
                else if (status == 1)
                {
                    request_status.type = 1;
                    request_status.position_in_queue = -1;
                    if (write(newsockfd, &request_status, sizeof(request_status)) < 0)
                        continue;
                }
                else if (status == 2)
                {
                    request_status.type = 2;
                    request_status.position_in_queue = -1;
                    if (write(newsockfd, &request_status, sizeof(request_status)) < 0)
                        continue;
                    char final_output_file[100];
                    sprintf(final_output_file, "./final_outputs/%d_otpt.txt", request_status.request_id);
                    if (send_response_to_client_from_file(newsockfd, final_output_file) < 0)
                        continue;
                    string remove_output_file_command = "rm " + string(final_output_file);
                    system(remove_output_file_command.c_str());
                    status_hashmap.erase(request_status.request_id);
                    requestID_set.insert(request_status.request_id);
                    pthread_cond_signal(&set_empty);
                }
            }
        }
        close(newsockfd);
    }
}

void initialize(){
    for (int i = 0; i < MAX_REQUESTID_SET_SIZE; i++)
    {
        requestID_set.insert(i);
    }
    
}

void *average_queue_size(void *arg)
{
    while (1)
    {
        sleep(2);
        cout << "Queue Size: " << newsockfd_queue.size() << endl;
    }
}