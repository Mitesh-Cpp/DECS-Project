#include "declarations.h"
#include "utilities.cpp"

int main(int argc, char *argv[])
{

    // ---------------------------- SOCKET SET UP ----------------------------

    int sockfd, newsockfd, portno, thread_num1, thread_num2; 
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    if (argc != 5)
    {
        cerr << "USAGE: ./server port file_receiving_threads handler_threads restart/take_backup(0/1)\n"; 
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
    thread_num1 = atoi(argv[2]); 
    thread_num2 = atoi(argv[3]); 
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "ERROR on binding";
        exit(EXIT_FAILURE);
    }
    listen(sockfd, 3000);
    clilen = sizeof(cli_addr);

    // ---------------------------- SOCKET SET UP FINISH ----------------------------


    //setting up requestID set
    if(atoi(argv[4])==0){
        cout<<"Restarted"<<endl;
        initialize(false);
        cout<<"Initialization Done"<<endl;
    }
    else{
        cout<<"Taking backup"<<endl;
        initialize(true);
        cout<<"Backup Done"<<endl;
    }
    


    // ---------------------------- SET ALL THREADS (BOTH THREAD POOLS) --------------------------

    pthread_t worker_thread[thread_num1], avg_queue_thread;
    pthread_t request_handler_thread[thread_num2];
    pthread_t backup_thread;
    for (int i = 0; i < thread_num1; i++)
    {
        if (pthread_create(&worker_thread[i], NULL, &receive_file, NULL) != 0) 
        {
            cerr << "ERROR creating 1st thread pool" << i << endl;
        }
    }
    for (int i = 0; i < thread_num2; i++)
    {
        if (pthread_create(&request_handler_thread[i], NULL, &handle_request, NULL) != 0) 
        {
            cerr << "ERROR creating 2nd thread pool" << i << endl;
        }
    }
    if (pthread_create(&backup_thread, NULL, &backup_function, NULL) != 0) 
    {
        cerr << "ERROR creating Backup thread"<<endl;
    }



    // ---------------------------- CONTINOUSLY RECEIVE CLIENT REQUEST (SOCKFD's) --------------------------

    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            cerr << "ERROR on accept: " << strerror(errno) << endl;
            continue; 
        }
        
        // Push newsockfd to queue
        pthread_mutex_lock(&queue_lock);
        newsockfd_queue.push(newsockfd);
        pthread_cond_signal(&queue_empty);
        pthread_mutex_unlock(&queue_lock);
    }

    return 0;
}
