#include "declarations.h"
#include "utilities.cpp"

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, thread_num; 
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    if (argc != 3)
    {
        cerr << "USAGE: ./server port threads\n"; 
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "ERROR opening socket";
        exit(EXIT_FAILURE);
    }

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    portno = atoi(argv[1]);
    thread_num = atoi(argv[2]); 
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "ERROR on binding";
        exit(EXIT_FAILURE);
    }
    listen(sockfd, 1000);
    clilen = sizeof(cli_addr);

    //  Creating all thread_num threads using pthread_create

    pthread_t worker_thread[thread_num], avg_queue_thread;
    for (int i = 0; i < thread_num; i++)
    {
        if (pthread_create(&worker_thread[i], NULL, &handle_client, NULL) != 0) // P
        {
            cerr << "ERROR creating thread: " << i << endl;
        }
    }
    if(pthread_create(&avg_queue_thread,NULL,&average_queue_size,NULL) != 0) {
        cerr << "Error creating queue thread." << endl;
    }

   while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            cerr << "ERROR on accept: " << strerror(errno) << endl;
            continue; 
        }
        // push newsockfd to queue
        if (pthread_mutex_lock(&queue_lock) != 0)
        {
            cerr << "ERROR locking mutex" << endl;
        }
        newsockfd_queue.push(newsockfd);
        if (pthread_cond_signal(&queue_empty) != 0)
        {
            cerr << "ERROR signaling condition variable" << endl;
        }
        if (pthread_mutex_unlock(&queue_lock) != 0)
        {
            cerr << "ERROR unlocking mutex" << endl;
        }
    }

    return 0;
}
