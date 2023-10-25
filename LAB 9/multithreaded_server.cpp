#include "declarations.h"
#include "utilities.cpp"

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, thread_num; // P
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    if (argc != 3)
    {
        cerr << "USAGE: ./server port threads\n"; // P
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    portno = atoi(argv[1]);
    thread_num = atoi(argv[2]); // P
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, reinterpret_cast<struct sockaddr *>(&serv_addr), sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 3000);
    clilen = sizeof(cli_addr);

    // P ... start
    //  Create all thread_num threads using pthread_create

    pthread_t worker_thread[thread_num], avg_queue_thread;
    for (int i = 0; i < thread_num; i++)
    {
        if (pthread_create(&worker_thread[i], NULL, &handle_client, NULL) != 0) // P
        {
            cerr << "ERROR creating thread: " << i << endl;
        }
    }
    // cout << "here" << endl;
    pthread_create(&avg_queue_thread,NULL,&average_queue_size,NULL);

    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0){
            error("ERROR on accept");
        }
        // push newsockfd to queue
        pthread_mutex_lock(&queue_lock);
        newsockfd_queue.push(newsockfd);
        // cout << "Inside main: Queue size is " << newsockfd_queue.size() << endl; 
        pthread_cond_signal(&queue_empty);
        pthread_mutex_unlock(&queue_lock);
    }
    // P ... done

    return 0;
}
