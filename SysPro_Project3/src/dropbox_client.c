/**** dropbox_client.c ****/
#include "dropbox_client.h"

void sig_h(int signo);
int send_file_list(int sock_d);
void free_file_list(file_list *f_list);
int send_file(int sock_d);
void free_list(client_list *list);
void *work(void *argp);
void perror_t(const char *description, int error);
void destroy_mtx();

buffer_t buffer = {0};
char *dir_path = NULL;
client_list *list = NULL;
client_tuple this_client = {0};
int term_flag = 0;
int listening_socket;
pthread_mutex_t client_list_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buffer_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_nonempty = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_nonfull = PTHREAD_COND_INITIALIZER;

const char not_in_list[] = "The client trying to communicate is not in the client list.\n";
const char error_comm_client[] = "An error occured while communicating with another client.\n";
const char serv_not_reached[] = "The server could not be reached.\n";
const char error_comm_server[] = "An error occured while communicating with the server.\n";
const char sth_wrong_client[] = "Something wrong came up from another client.\n";
const char error_file_client[] = "An error occurred while sending a file to another client.\n";
const char error_list_client[] = "An error occurred while sending file list to a client.\n";
const char error_file_rec_client[] = "An error occured while getting a file from another client.\n";
const char wrong_server[] = "A from-server kind of message was received, but not from this client's server.\n";
const char problem_notifying[] = "There was a problem notifying another client that it is not in the client list.\n";

int main(int argc, char **argv) {
    //parameters parsing
    CmdParams args = use(argc, argv);

    /**
     * 0 = dirname
     * 1 = portNum
     * 2 = workerThreads
     * 3 = bufferSize
     * 4 = serverPort
     * 5 = serverIP
    **/

    if (!(args.rightUsage)) {
        destroy_mtx();
        exit(1);
    }

    /**** Start catching SIGINT and SIGPIPE signals ****/
    signal(SIGINT, sig_h);
    signal(SIGPIPE, sig_h);

    if ( (dir_path = realpath(args.params[0], NULL)) == NULL){        //if dir_path does not exist, exit
        perror("dir_path - realpath()");
        free(dir_path);
        freeCmdParams(&args);
        destroy_mtx();

        exit(1);
    }

    if (args.params[1][0] == '-') {
        fprintf(stderr, "The client's port number cannot be a negative number.\n");
        free(dir_path);
        freeCmdParams(&args);
        destroy_mtx();

        exit(1);
    }

    uint16_t portNum = (uint16_t) strtol(args.params[1], NULL, 10);
    if (portNum < 1024) {
        fprintf(stderr, "The port number is less than 1024.\n");
        free(dir_path);
        freeCmdParams(&args);
        destroy_mtx();

        exit(1);
    }

    int workerThreads = (int) strtol(args.params[2], NULL, 10);

    if (workerThreads <= 0) {
        fprintf(stderr, "Invalid number of worker threads.\n");
        free(dir_path);
        freeCmdParams(&args);
        destroy_mtx();

        exit(1);
    }

    size_t bufferSize = (size_t) strtol(args.params[3], NULL, 10);

    if (bufferSize == 0) {
        fprintf(stderr, "The buffer's size cannot be 0.\n");
        free(dir_path);
        freeCmdParams(&args);
        destroy_mtx();

        exit(1);
    }

    if (args.params[4][0] == '-') {
        fprintf(stderr, "The server's port number cannot be a negative number.\n");
        free(dir_path);
        freeCmdParams(&args);
        destroy_mtx();

        exit(1);
    }

    struct sockaddr_in serverSocketAddress = {0};
    int sock_with_server = -1;
    struct hostent *rem;

    if ( (rem = gethostbyname(args.params[5])) == NULL) {
        herror("gethostbyname");
        free(dir_path);
        freeCmdParams(&args);
        destroy_mtx();

        exit(1);
    }

    memcpy(&serverSocketAddress.sin_addr, rem->h_addr, rem->h_length);
    serverSocketAddress.sin_family = AF_INET;
    serverSocketAddress.sin_port = htons((uint16_t) strtol(args.params[4], NULL, 10));

    freeCmdParams(&args);       //Finished parameters parsing

    if (term_flag == 1) {
        free(dir_path);
        fprintf(stdout, "The client is exiting.\n");
        destroy_mtx();

        exit(0);
    }

    /**** Create client's sockets ****/
    if ( (sock_with_server = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
        perror("Client socket with server could not be created");
        free(dir_path);
        destroy_mtx();

        exit(1);
    }

    if ( (listening_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
        perror("Client socket with other clients could not be created");
        free(dir_path);
        close(sock_with_server);
        destroy_mtx();

        exit(1);
    }

    struct sockaddr_in client;
    client.sin_port = htons(portNum);
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = htonl(INADDR_ANY);

    char hb[256];
    gethostname(hb, sizeof(hb));
    struct hostent *h_ent;
    h_ent = gethostbyname(hb);
    char *this_clientIP = inet_ntoa(*((struct in_addr*) h_ent->h_addr_list[0]));
    this_client.IP_Address = inet_addr(this_clientIP);
    this_client.portNum = client.sin_port;

    int enable = 1;

    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        perror("Setsockopt");
        free(dir_path);
        close(listening_socket);
        close(sock_with_server);
        destroy_mtx();

        exit(1);
    }

    if ( bind(listening_socket, (struct sockaddr *) &client, sizeof(client)) ) {
        perror("Bind socket with other client");
        free(dir_path);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    if ( listen(listening_socket, 100) == -1 ) {
        perror("Listen");
        free(dir_path);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    /**** Connect to server's socket ****/
    if ( (connect(sock_with_server, (struct sockaddr *)&serverSocketAddress, sizeof(serverSocketAddress))) < 0 ) {
        perror("Connection to server");
        free(dir_path);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    client_tuple server_tuple;      /* Needed to check if the messages supposed to be coming from a server, are from this client's server */
    struct pollfd fds;
    fds.events = POLLOUT;
    fds.revents = 0;
    fds.fd = sock_with_server;

    while (fds.revents != POLLOUT && term_flag == 0) {
        poll(&fds, 1, 100);
    }

    if (term_flag == 1) {
        free(dir_path);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    if ( (write(sock_with_server, &LOG_ON, sizeof(LOG_ON))) <= 0 ) {
        perror("Write LOG_ON");
        free(dir_path);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    if ( (write(sock_with_server, &this_client, sizeof(client_tuple))) <= 0) {
        perror("Write LOG_ON client_tuple");
        free(dir_path);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    fds.events = POLLIN;
    fds.revents = 0;
    fds.fd = sock_with_server;

    while (fds.revents != POLLIN && term_flag == 0) {
        poll(&fds, 1, 100);
    }

    if (term_flag == 1) {
        free(dir_path);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    if (read(sock_with_server, &server_tuple, sizeof(client_tuple)) <= 0) {
        perror("Read server's tuple");
        free(dir_path);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    close(sock_with_server);
    sock_with_server = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_with_server == -1) {
        perror("Socket");
        free(dir_path);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    /**** Initialize circular buffer to put client list inside ****/
    buffer_init(&buffer, bufferSize);

    if ( (connect(sock_with_server, (struct sockaddr *)&serverSocketAddress, sizeof(serverSocketAddress))) < 0 ) {
        perror("Connection to server");
        free(dir_path);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    fds.events = POLLOUT;
    fds.revents = 0;
    fds.fd = sock_with_server;

    while (fds.revents != POLLOUT && term_flag == 0) {
        poll(&fds, 1, 100);
    }

    if (term_flag == 1) {
        free(dir_path);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    if ( (write(sock_with_server, &GET_CLIENTS, sizeof(GET_CLIENTS))) <= 0 ) {
        perror("write GET_CLIENTS");
        free(dir_path);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    if ((write(sock_with_server, &this_client, sizeof(client_tuple))) <= 0) {
        perror("write this_client tuple");
        free(dir_path);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    if (term_flag == 1) {
        free(dir_path);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        fprintf(stdout, "The client is exiting.\n");
        exit(0);
    }

    /**** Commence receival of client addresses tuples ****/
    short received_msg;
    int number_of_clients;

    fds.events = POLLIN;
    fds.revents = 0;
    fds.fd = sock_with_server;

    while (fds.revents != POLLIN && term_flag == 0) {
        poll(&fds, 1, 100);
    }

    if (term_flag == 1) {
        free(dir_path);
        free(buffer.circ_buffer);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    if (read(sock_with_server, &received_msg, sizeof(short)) <= 0) {
        fprintf(stderr, "The server could not be reached.\n");
        free(dir_path);
        free(buffer.circ_buffer);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    if (received_msg != CLIENT_LIST) {
        fprintf(stderr, "A problem occured while communicating with the server.\n");
        free(dir_path);
        free(buffer.circ_buffer);
        close(listening_socket);
        close(sock_with_server);
        destroy_mtx();

        exit(1);
    }

    if (read(sock_with_server, &number_of_clients, sizeof(number_of_clients)) <= 0) {
        fprintf(stderr, "The server could not be reached.\n");
        free(dir_path);
        free(buffer.circ_buffer);
        close(sock_with_server);
        close(listening_socket);
        destroy_mtx();

        exit(1);
    }

    /* Creation of worker threads */
    pthread_t *thread_IDs = malloc(sizeof(pthread_t) * workerThreads);
    int create_error;

    for (int i = 0; i < workerThreads; i++) {
        if ( (create_error = pthread_create(&thread_IDs[i], NULL, work, NULL)) != 0 ) {
            perror_t("pthread_create", create_error);
            term_flag = 1;
            for (int j = 0; j < i; j++) {
                pthread_join(thread_IDs[j], NULL);
            }

            close(sock_with_server);
            close(listening_socket);
            free(dir_path);
            free_list(list);
            free(buffer.circ_buffer);
            free(thread_IDs);

            exit(1);
        }
    }

    /* Receival of client tuples from server */
    client_tuple c_tuple = {0};

    for (int i = 0; i < number_of_clients; i++) {
        fds.events = POLLIN;
        fds.revents = 0;
        fds.fd = sock_with_server;

        while (fds.revents != POLLIN && term_flag == 0) {
            poll(&fds, 1, 100);
        }

        if (term_flag == 1) {
            term_flag = 1;
            for (int j = 0; j <= i; j++) {
                pthread_join(thread_IDs[j], NULL);
            }
            free(dir_path);
            free(thread_IDs);
            free(buffer.circ_buffer);
            close(sock_with_server);
            close(listening_socket);
            destroy_mtx();

            exit(1);
        }

        if ( read(sock_with_server, &c_tuple, sizeof(client_tuple)) <= 0) {         
            write(2, serv_not_reached, strlen(serv_not_reached));
            term_flag = 1;
            for (int j = 0; j <= i; j++) {
                pthread_join(thread_IDs[j], NULL);
            }
            free(thread_IDs);
            free(dir_path);
            free(buffer.circ_buffer);
            close(sock_with_server);
            close(listening_socket);
            destroy_mtx();

            exit(1);
        }

        // The server sends this client's address too
        if (c_tuple.IP_Address == this_client.IP_Address && c_tuple.portNum == this_client.portNum) {
            continue;
        }

        pthread_mutex_lock(&client_list_mtx);
        client_list_add(&list, &c_tuple);
        pthread_mutex_unlock(&client_list_mtx);

        element_t el = {0}; memcpy(&(el.c_tuple), &c_tuple, sizeof(client_tuple));

        pthread_mutex_lock(&buffer_mtx);
        while (buffer_is_full(&buffer) && term_flag == 0) {
            pthread_cond_wait(&buffer_nonfull, &buffer_mtx);
        }
        if (term_flag == 1) {
            pthread_mutex_unlock(&buffer_mtx);
            for (int j = 0; j <= i; j++) {
                pthread_join(thread_IDs[j], NULL);
            }
            free(thread_IDs);
            free(dir_path);
            free(buffer.circ_buffer);
            close(sock_with_server);
            close(listening_socket);
            destroy_mtx();

            exit(1);
        }
        buffer_push(&buffer, &el);
        pthread_mutex_unlock(&buffer_mtx);
        pthread_cond_broadcast(&buffer_nonempty);
    }

    close(sock_with_server);
    chdir(dir_path);        //all jobs from now on happen in client's directory
    mkdir("__RSRVD", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

    /**** Finished starting client ****/
    struct sockaddr_in addr = {0};
    int sock_d, _ret;
    socklen_t socksize = sizeof(struct sockaddr_in);

    while (term_flag == 0) {
        fds.events = POLLIN;
        fds.revents = 0;
        fds.fd = listening_socket;

        while (fds.revents != POLLIN && term_flag == 0) {
            poll(&fds, 1, 100);
        }

        if (term_flag == 1) {
            break;
        }

        sock_d = accept(listening_socket, (struct sockaddr *) &addr, &socksize);

        if (term_flag == 1) {
            close(sock_d);
            break;
        }

        fds.events = POLLIN;
        fds.revents = 0;
        fds.fd = sock_d;

        while (fds.revents != POLLIN && term_flag == 0) {
            poll(&fds, 1, 100);
        }

        if (term_flag == 1) {
            close(sock_d);
            break;
        }

        received_msg = 0;

        if (read(sock_d, &received_msg, sizeof(short)) <= 0) {
            write(2, error_comm_client, strlen(error_comm_client));
            close(sock_d);
            continue;
        }

        if (read(sock_d, &c_tuple, sizeof(client_tuple)) <= 0) {
            write(2, error_comm_client, strlen(error_comm_client));
            close(sock_d);
            continue;
        }

        fds.events = POLLOUT;
        fds.revents = 0;
        fds.fd = sock_d;

        while (fds.revents != POLLOUT && term_flag == 0) {
            poll(&fds, 1, 100);
        }

        if (term_flag == 1) {
            close(sock_d);
            break;
        }

        /* No need to lock the client list mutex when searching in it, since only the main thread can modify it */
        switch (received_msg) {
            case _GET_FILE_LIST:
                if (!(in_client_list(list, &c_tuple))) {
                    if (write(sock_d, &IP_PORT_NOT_FOUND, sizeof(IP_PORT_NOT_FOUND)) <= 0) {
                        write(2, problem_notifying, strlen(problem_notifying));
                        break;
                    }
                    else {
                        write(2, not_in_list, strlen(not_in_list));
                        break;
                    }
                }
                else if (write(sock_d, &OK, sizeof(OK)) <= 0) {
                    write(2, error_comm_client, strlen(error_comm_client));
                    break;
                }

                _ret = send_file_list(sock_d);

                if (_ret != 0) {
                    write(2, error_list_client, strlen(error_list_client));
                    close(sock_d);
                    continue;
                }

                /* This code makes sure that the other client has closed its end of the socket before closing this end */
                fds.events = POLLOUT;
                fds.revents = 0;
                fds.fd = sock_d;

                while (fds.revents == POLLOUT && term_flag == 0) {                       
                    poll(&fds, 1, 100);
                }
                break;
            case _GET_FILE:
                if (!(in_client_list(list, &c_tuple))) {
                    if (write(sock_d, &IP_PORT_NOT_FOUND, sizeof(IP_PORT_NOT_FOUND)) <= 0) {
                        write(2, problem_notifying, strlen(problem_notifying));
                        break;
                    }
                    else {
                        write(2, not_in_list, strlen(not_in_list));
                        break;
                    }
                }
                else if (write(sock_d, &OK, sizeof(OK)) <= 0) {
                    write(2, error_comm_client, strlen(error_comm_client));
                    break;
                }

                _ret = send_file(sock_d);

                if (_ret != 0) {
                    write(2, error_file_client, strlen(error_file_client));
                    close(sock_d);
                    continue;
                }
                
                fds.events = POLLIN;
                fds.revents = 0;
                fds.fd = sock_d;

                while (fds.revents == POLLOUT && term_flag == 0) {
                    poll(&fds, 1, 100);
                }
                break;
            case _USER_ON:
                if (!(server_tuple.IP_Address == c_tuple.IP_Address && server_tuple.portNum == c_tuple.portNum)) {
                    write(sock_d, &IP_PORT_NOT_FOUND, sizeof(IP_PORT_NOT_FOUND));
                    write(2, wrong_server, strlen(wrong_server));
                    break;
                }
                else if (write(sock_d, &OK, sizeof(OK)) <= 0) {
                    write(2, error_comm_server, strlen(error_comm_server));
                    break;
                }
                fds.events = POLLIN;
                fds.revents = 0;
                fds.fd = sock_d;

                while (fds.revents != POLLIN && term_flag == 0) {
                    poll(&fds, 1, 100);
                }

                if (term_flag == 1) {
                    close(sock_d);
                    break;
                }
                if ( read(sock_d, &c_tuple, sizeof(c_tuple)) <= 0) {
                    write(2, error_comm_server, strlen(error_comm_server));
                    close(sock_d);
                    continue;
                }               
                pthread_mutex_lock(&client_list_mtx);
                client_list_add(&list, &c_tuple);
                pthread_mutex_unlock(&client_list_mtx);

                element_t el = {0}; memcpy(&(el.c_tuple), &c_tuple, sizeof(client_tuple));

                pthread_mutex_lock(&buffer_mtx);
                while(buffer_is_full(&buffer) && term_flag == 0) {
                    pthread_cond_wait(&buffer_nonfull, &buffer_mtx);
                }
                if (term_flag == 1) {
                    pthread_mutex_unlock(&buffer_mtx);
                    break;
                }
                buffer_push(&buffer, &el);
                pthread_mutex_unlock(&buffer_mtx);
                pthread_cond_broadcast(&buffer_nonempty);
                break;
            case _USER_OFF:
                if (!(server_tuple.IP_Address == c_tuple.IP_Address && server_tuple.portNum == c_tuple.portNum)) {
                    write(sock_d, &IP_PORT_NOT_FOUND, sizeof(IP_PORT_NOT_FOUND));
                    write(2, wrong_server, strlen(wrong_server));
                    break;
                }
                else if (write(sock_d, &OK, sizeof(OK)) <= 0) {
                    write(2, error_comm_server, strlen(error_comm_server));
                    break;
                }
                if ( read(sock_d, &c_tuple, sizeof(c_tuple)) <= 0) {
                    write(2, error_comm_server, strlen(error_comm_server));
                    close(sock_d);
                    continue;
                }
                pthread_mutex_lock(&client_list_mtx);
                delete_from_client_list(&list, &c_tuple);
                pthread_mutex_unlock(&client_list_mtx);
                break;                 
            default:
                write(2, sth_wrong_client, strlen(sth_wrong_client));
                break;
        }
        close(sock_d);
    }

    for (int i = 0; i < workerThreads; i++) {
        pthread_join(thread_IDs[i], NULL);
    }

    while ( (sock_with_server = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        /* retry */
    }

    /* The term_flag is reset in order to exit the client forcefully, in case the server has exited before all the clients had logged off */
    term_flag = 0;
    /* Need to make sure that the server gets the log off message */
    while (connect(sock_with_server, (struct sockaddr *) &serverSocketAddress, sizeof(serverSocketAddress)) < 0) {
        if (term_flag == 1) {
            fprintf(stderr, "The client exits without logging off.\n");
            close(sock_with_server);
            close(listening_socket);
            free(dir_path);
            free_list(list);
            free(buffer.circ_buffer);
            free(thread_IDs);
            destroy_mtx();

            exit(1);
        }
    }

    fds.events = POLLOUT;
    fds.revents = 0;
    fds.fd = sock_with_server;

    while (fds.revents != POLLOUT && term_flag == 0) {
        poll(&fds, 1, 100);
    }

    if (term_flag == 1) {
        fprintf(stderr, "The client could not log off normally.\n");
        close(sock_with_server);
        close(listening_socket);
        free(dir_path);
        free_list(list);
        free(buffer.circ_buffer);
        free(thread_IDs);
        destroy_mtx();

        exit(1);
    }

    if (write(sock_with_server, &LOG_OFF, sizeof(LOG_OFF)) <= 0) {
        fprintf(stderr, "The client could not log off normally.\n");
        close(sock_with_server);
        close(listening_socket);
        free(dir_path);
        free_list(list);
        free(buffer.circ_buffer);
        free(thread_IDs);
        destroy_mtx();

        exit(1);
    }

    if (write(sock_with_server, &this_client, sizeof(client_tuple)) <= 0) {
        fprintf(stderr, "The client could not log off normally.\n");
        close(sock_with_server);
        close(listening_socket);
        free(dir_path);
        free_list(list);
        free(buffer.circ_buffer);
        free(thread_IDs);
        destroy_mtx();

        exit(1);
    }

    fds.events = POLLIN;
    fds.revents = 0;
    fds.fd = sock_with_server;

    while (fds.revents != POLLIN && term_flag == 0) {
        poll(&fds, 1, 100);
    }

    if (read(sock_with_server, &received_msg, sizeof(received_msg)) <= 0) {
        fprintf(stderr, "The client could not log off normally.\n");
    }

    if (received_msg == IP_PORT_NOT_FOUND) {
        fprintf(stderr, "The client could not log off normally.\n");
    }

    close(sock_with_server);
    close(listening_socket);
    free(dir_path);
    free_list(list);
    free(buffer.circ_buffer);
    free(thread_IDs);
    fprintf(stdout, "The client exits.\n");
    destroy_mtx();

    exit(0);
}

void sig_h(int signo) {
    if (signo == SIGINT) {
        term_flag = 1;
        pthread_cond_broadcast(&buffer_nonempty);
        pthread_cond_broadcast(&buffer_nonfull);
    }
}

int send_file_list(int sock_d) {
    int num_of_files = 0;
    file_list *f_list = NULL, *last = NULL, *curr_node;

    while ( get_file_list(dir_path, &f_list, &last, &num_of_files, 1) > 0 ) {
        //in case it fails for some strange reason
    }

    if ( write(sock_d, &FILE_LIST, sizeof(FILE_LIST)) <= 0) {
        write(2, error_comm_client, strlen(error_comm_client));
        free_file_list(f_list);
        return 1;
    }
    if ( write(sock_d, &num_of_files, sizeof(num_of_files)) <= 0) {
        write(2, error_comm_client, strlen(error_comm_client));
        free_file_list(f_list);
        return 1;
    }

    if (num_of_files == 0) {
        return 0;
    }

    curr_node = f_list;

    while (curr_node->next != NULL) {
        if ( write(sock_d, curr_node->this_tuple, sizeof(file_tuple)) <= 0 ) {
            write(2, error_comm_client, strlen(error_comm_client));
            free_file_list(f_list);
            return 1;
        }

        curr_node = curr_node->next;
    }

    if ( write(sock_d, curr_node->this_tuple, sizeof(file_tuple)) <= 0 ) {
        write(2, error_comm_client, strlen(error_comm_client));
        free_file_list(f_list);
        return 1;
    }

    free_file_list(f_list);

    return 0;
}

void free_file_list(file_list *f_list) {
    file_list *curr_node = f_list;
    if (curr_node == NULL) {
        return;
    }

    while (curr_node->next != NULL) {
        curr_node = curr_node->next;
        free(curr_node->previous->this_tuple);
        free(curr_node->previous);
    }
    free(curr_node->this_tuple);
    free(curr_node);

    return;
}

int send_file(int sock_d) {
    file_tuple ft = {0};
    int file_fd;
    char *version = NULL, buffer[129];
    struct pollfd fds;
    struct stat stat_buf = {0};
    long file_size, remaining;

    if ( read(sock_d, &ft, sizeof(file_tuple)) <= 0) {
        write(2, error_comm_client, strlen(error_comm_client));
        return 1;
    }

    file_fd = open(ft.pathname, O_RDONLY);
    if (file_fd == -1) {        //the file does not exist
        if ( write(sock_d, &FILE_NOT_FOUND, sizeof(FILE_NOT_FOUND)) <= 0 ) {
            write(2, error_comm_client, strlen(error_comm_client));
            return 1;
        }

        return 0;
    }
    version = find_version(file_fd);
    close(file_fd);
    file_fd = open(ft.pathname, O_RDONLY);

    if (!(strcmp(version, ft.version))) {      //if the local version is same as the remote
        close(file_fd);
        free(version);
        if ( write(sock_d, &FILE_UP_TO_DATE, sizeof(FILE_UP_TO_DATE)) <= 0) {
            write(2, error_comm_client, strlen(error_comm_client));
            return 1;
        }
        return 0;
    }

    stat(ft.pathname, &stat_buf);       //find the file's size
    file_size = stat_buf.st_size;

    fds.events = POLLOUT;
    fds.revents = 0;
    fds.fd = sock_d;

    while (fds.revents != POLLOUT && term_flag == 0) {
        poll(&fds, 1, 100);
    }

    if (term_flag == 1) {
        return 1;
    }
        
    if ( write(sock_d, &FILE_SIZE, sizeof(FILE_SIZE)) <= 0) {
        write(2, error_comm_client, strlen(error_comm_client));
        free(version);
        close(file_fd);
        return 1;
    }

    if ( write(sock_d, version, 33) <= 0) {
        write(2, error_comm_client, strlen(error_comm_client));
        free(version);
        close(file_fd);
        return 1;
    }

    free(version);

    if ( write(sock_d, &file_size, sizeof(file_size)) <= 0) {
        write(2, error_comm_client, strlen(error_comm_client));
        close(file_fd);
        return 1;
    }

    if (file_size != 0) {
        remaining = file_size;
        while ( remaining >= 128 ) {
            read(file_fd, buffer, 128);

            fds.events = POLLOUT;
            fds.revents = 0;
            fds.fd = sock_d;

            while (fds.revents != POLLOUT && term_flag == 0) {
                poll(&fds, 1, 100);
            }

            if (term_flag == 1) {
                close(file_fd);
                return 1;
            }

            if ( write(sock_d, buffer, 128) <= 0) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(file_fd);
                return 1;
            }
            remaining -= 128;
        }
        read(file_fd, buffer, 128);             
        fds.events = POLLOUT;
        fds.revents = 0;
        fds.fd = sock_d;

        while (fds.revents != POLLOUT && term_flag == 0) {
            poll(&fds, 1, 100);
        }
        if (term_flag == 1) {
            close(file_fd);
            return 1;
        }
        if ( write(sock_d, buffer, remaining) < 0) {
            write(2, error_comm_client, strlen(error_comm_client));
            close(file_fd);
            return 1;
        }
    }
    close(file_fd);
    return 0;
}

/* Threads function */
void *work(void *argp) {
    int sock_d, file_fd;
    struct sockaddr_in addr = {0};
    element_t *elem;
    file_tuple ft;
    struct pollfd fds;
    short msg;

    addr.sin_family = AF_INET;

    while (term_flag != 1) {
        pthread_mutex_lock(&buffer_mtx);
        while (buffer_is_empty(&buffer) && term_flag == 0) {
            pthread_cond_wait(&buffer_nonempty, &buffer_mtx);
        }
        if (term_flag == 1) {
            pthread_mutex_unlock(&buffer_mtx);
            break;
        }
        elem = buffer_pop(&buffer);
        pthread_mutex_unlock(&buffer_mtx);
        pthread_cond_broadcast(&buffer_nonfull);

        /* Need to lock the client list because the main thread can modify it while this thread reads */
        pthread_mutex_lock(&client_list_mtx);
        if (!(in_client_list(list, &elem->c_tuple))) {
            write(2, not_in_list, strlen(not_in_list));
            pthread_mutex_unlock(&client_list_mtx);
            continue;
        }
        pthread_mutex_unlock(&client_list_mtx);

        sock_d = socket(AF_INET, SOCK_STREAM, 0);

        addr.sin_addr.s_addr = elem->c_tuple.IP_Address;
        addr.sin_port = elem->c_tuple.portNum;

        if (elem->f_tuple.pathname[0] == 0) {       //it is only a client tuple
            if ( (connect(sock_d, (struct sockaddr *) &addr, sizeof(addr))) < 0 ) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(sock_d);
                continue;
            }
            fds.events = POLLOUT;
            fds.revents = 0;
            fds.fd = sock_d;

            while (fds.revents != POLLOUT && term_flag == 0) {
                poll(&fds, 1, 100);
            }

            if (term_flag == 1) {
                close(sock_d);
                break;
            }

            if ( write(sock_d, &GET_FILE_LIST, sizeof(GET_FILE_LIST)) <= 0) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(sock_d);
                continue;
            }
            if (write(sock_d, &this_client, sizeof(client_tuple)) <= 0) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(sock_d);
                continue;
            }

            fds.events = POLLIN;
            fds.revents = 0;
            fds.fd = sock_d;

            while (fds.revents != POLLIN && term_flag == 0) {
                poll(&fds, 1, 100);
            }

            if (term_flag == 1) {
                close(sock_d);
                break;
            }

            if (read(sock_d, &msg, sizeof(msg)) <= 0) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(sock_d);
                continue;
            }

            if (msg != OK) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(sock_d);
                continue;
            }

            if (read(sock_d, &msg, sizeof(msg)) <= 0) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(sock_d);
                continue;
            } 

            if (msg != FILE_LIST) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(sock_d);
                continue;
            }

            int num_of_files;

            if (read(sock_d, &num_of_files, sizeof(num_of_files)) <= 0) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(sock_d);
                continue;
            }

            element_t *tmp_array = malloc(sizeof(element_t) * num_of_files);

            for (int i = 0; i < num_of_files; i++) {
                memcpy(&(tmp_array[i].c_tuple), &elem->c_tuple, sizeof(client_tuple));

                if (read(sock_d, &ft, sizeof(file_tuple)) <= 0) {
                    write(2, error_comm_client, strlen(error_comm_client));
                    close(sock_d);
                    continue;
                }

                memcpy(&(tmp_array[i].f_tuple), &ft, sizeof(file_tuple));
            }

            close(sock_d);
            for (int i = 0; i < num_of_files; i++) {
                pthread_mutex_lock(&buffer_mtx);
                while (buffer_is_full(&buffer) && term_flag == 0) {
                    pthread_cond_wait(&buffer_nonempty, &buffer_mtx);
                }
                if (term_flag == 1) {
                    pthread_mutex_unlock(&buffer_mtx);
                    break;
                }
                buffer_push(&buffer, &(tmp_array[i]));
                pthread_mutex_unlock(&buffer_mtx);
                pthread_cond_broadcast(&buffer_nonempty);
            }

            free(tmp_array);
        }
        else {          //it is a file to be received
            long file_size, remaining;
            char portNumStr[6], clientName[22], version[33], buffer[129];
            version[0] = 0;
            char *path_buf, *tokenized, *token, *tree, *target, *get_vers = NULL;
            sprintf(portNumStr, "%hu", ntohs(addr.sin_port));
            strcpy(clientName, inet_ntoa(addr.sin_addr));
            strcat(clientName, "@");
            strcat(clientName, portNumStr);

            path_buf = malloc(sizeof(char) * (strlen(dir_path) + strlen("/__RSRVD/") + strlen(clientName) + strlen(elem->f_tuple.pathname) + 2));
            strcpy(path_buf, dir_path);
            strcat(path_buf, "/__RSRVD/");
            strcat(path_buf, clientName);
            strcat(path_buf, "/");

            /* Create the client's directory if it does not exist */
            mkdir(path_buf, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

            tokenized = malloc( sizeof(char) * (strlen(elem->f_tuple.pathname) + 1 ) );
            int tokenized_len = strlen(elem->f_tuple.pathname);
            strcpy(tokenized, elem->f_tuple.pathname);
            tree = NULL;
            target = malloc(sizeof(char) * (strlen(path_buf) + strlen(elem->f_tuple.pathname) + 1 ) );
            strcpy(target, path_buf);
            strcat(target, elem->f_tuple.pathname);

            int i = 0;
            while ( tokenized[i] != '/' && i < tokenized_len) {
                i++;
            }
            if (i < tokenized_len) {
                tokenized[i] = 0;
            }
            token = tokenized;

            if ( (strcmp(tokenized, elem->f_tuple.pathname)) ){
                tree = malloc(sizeof(char) * (strlen(path_buf) + strlen(elem->f_tuple.pathname) + 1 ) );
                strcpy(tree, path_buf);
                strcat(tree, token);

                while (i < tokenized_len) {
                    mkdir(tree, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
                    token = &tokenized[++i];
                    while (tokenized[i] != '/' && i < tokenized_len) {
                        i++;
                    }
                    if (i < tokenized_len) {
                        tokenized[i] = 0;
                        strcat(tree, "/");
                        strcat(tree, token);
                    }
                }

                if (!(strcmp(elem->f_tuple.version, "DIR"))) {
                    strcat(tree, "/");
                    strcat(tree, token);
                    mkdir(tree, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

                    free(tree);
                    free(path_buf);
                    free(tokenized);
                    free(target);

                    continue;
                }
            }
            else {
                if (!(strcmp(elem->f_tuple.version, "DIR"))) {
                    mkdir(target, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

                    free(tree);
                    free(path_buf);
                    free(tokenized);
                    free(target);

                    continue;
                }
            }

            free(tree);
            free(path_buf);
            free(tokenized);

            get_vers = get_version(target);
            if (get_vers != NULL) {
                strcpy(version, get_vers);
                free(get_vers);
            }

            memcpy(&ft, &(elem->f_tuple), sizeof(file_tuple));

            if ( (connect(sock_d, (struct sockaddr *) &addr, sizeof(addr))) < 0 ) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(sock_d);
                continue;
            }

            fds.events = POLLOUT;
            fds.revents = 0;
            fds.fd = sock_d;
            while (fds.revents != POLLOUT && term_flag == 0) {
                poll(&fds, 1, 100);
            }
            if (term_flag == 1) {
                close(sock_d);
                free(target);
                break;
            }
            if ( write(sock_d, &GET_FILE, sizeof(GET_FILE)) <= 0 ) {
                write(2, error_comm_client, strlen(error_comm_client));
                free(target);
                close(sock_d);
                continue;
            }
            if ( write(sock_d, &this_client, sizeof(client_tuple)) <= 0) {
                write(2, error_comm_client, sizeof(error_comm_client));
                free(target);
                close(sock_d);
                continue;
            }
            fds.events = POLLIN;
            fds.revents = 0;
            fds.fd = sock_d;
            while (fds.revents != POLLIN && term_flag == 0) {
                poll(&fds, 1, 100);
            }
            if (term_flag == 1) {
                close(sock_d);
                free(target);
                break;
            }
            if (read(sock_d, &msg, sizeof(msg)) <= 0) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(sock_d);
                free(target);
                break;
            }
            if (msg != OK) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(sock_d);
                free(target);
                continue;
            }
            fds.events = POLLOUT;
            fds.revents = 0;
            fds.fd = sock_d;
            while (fds.revents != POLLOUT && term_flag == 0) {
                poll(&fds, 1, 100);
            }
            if (term_flag == 1) {
                close(sock_d);
                free(target);
                break;
            }


            if (version[0] == 0) {      /* If the file does not exist */
                ft.version[0] = 0;
            }
            else {                      /* If it exists */
                strcpy(ft.version, version);
            }


            if ( write(sock_d, &ft, sizeof(file_tuple)) <= 0) {
                write(2, error_comm_client, strlen(error_comm_client));
                free(target);
                close(sock_d);
                continue;
            }
            fds.events = POLLIN;
            fds.revents = 0;
            fds.fd = sock_d;
            while (fds.revents != POLLIN && term_flag == 0) {
                poll(&fds, 1, 100);
            }
            if (term_flag == 1) {
                close(sock_d);
                free(target);
                break;
            }
            if ( read(sock_d, &msg, sizeof(msg)) <= 0 ) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(sock_d);
                free(target);
                continue;
            }
            if (msg == FILE_NOT_FOUND || msg == FILE_UP_TO_DATE) {
                close(sock_d);
                free(target);
                continue;
            }
            else if (msg != FILE_SIZE) {
                write(2, &error_comm_client, sizeof(error_comm_client));
                close(sock_d);
                free(target);
                continue;
            }
            if ( read(sock_d, version, 33) <= 0) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(sock_d);
                close(file_fd);
                continue;
            }
            if ( read(sock_d, &file_size, sizeof(file_size)) <= 0 ) {
                write(2, error_comm_client, strlen(error_comm_client));
                close(sock_d);
                close(file_fd);
                continue;
            }
            file_fd = open(target, O_CREAT | O_RDWR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
            if (file_fd == -1) {                                              
                write(2, error_file_rec_client, strlen(error_file_rec_client));
                free(target);
                close(sock_d);
                continue;
            }
            free(target);
            if (file_size != 0) {
                remaining = file_size;
                while ( remaining >= 128 ) {
                    fds.events = POLLIN;
                    fds.revents = 0;
                    fds.fd = sock_d;
                    while (fds.revents != POLLIN && term_flag == 0) {
                        poll(&fds, 1, 100);
                    }
                    if (term_flag == 1) {
                        close(sock_d);
                        close(file_fd);
                        break;
                    }
                    if ( read(sock_d, buffer, 128) <= 0) {
                        write(2, error_file_rec_client, strlen(error_file_rec_client));
                        close(file_fd);
                        close(sock_d);
                        continue;
                    }
                    write(file_fd, buffer, 128);
                    remaining -= 128;
                }
                fds.events = POLLIN;
                fds.revents = 0;
                fds.fd = sock_d;
                while (fds.revents != POLLIN && term_flag == 0) {
                    poll(&fds, 1, 100);
                }
                if (term_flag == 1) {
                    close(sock_d);
                    close(file_fd);
                }
                if ( read(sock_d, buffer, remaining) < 0) {
                    write(2, error_file_rec_client, strlen(error_file_rec_client));
                    close(file_fd);
                    close(sock_d);
                    continue;
                }
                write(file_fd, buffer, remaining);
            }
            close(file_fd);
            close(sock_d);
        }
    }

    /* return is used instead of pthread_exit because pthread_exit gives memory leaks 
    in valgrind and is not necessary anyway */
    return NULL;
}

void perror_t(const char *description, int error) {
    char *err_str = strerror(error);
    char *toPrint = malloc(sizeof(char) * (strlen(description) + strlen(err_str) + 3) );
    strcpy(toPrint, description);
    strcat(toPrint, ":");
    strcat(toPrint, err_str);
    strcat(toPrint, "\n");

    write(2, toPrint, strlen(toPrint));

    free(toPrint);

    return;
}

void destroy_mtx() {
    pthread_mutex_destroy(&client_list_mtx);
    pthread_mutex_destroy(&buffer_mtx);
    pthread_cond_destroy(&buffer_nonempty);
    pthread_cond_destroy(&buffer_nonfull);
}