/**** dropbox_server.c ****/
#include "dropbox_server.h"

int term_flag = 0;
client_tuple server_tuple = {0};

void sig_h(int signo);
void notify_clients_USER_ON(client_list *list, client_tuple *ct);
void notify_clients_USER_OFF(client_list *list, client_tuple *ct);

int main(int argc, char **argv) {
    in_port_t portNum;
    struct sockaddr_in addr;
    if (argc != 3 || strcmp(argv[1], "-p") || (portNum = (in_port_t) strtol(argv[2], NULL, 10)) < 0) {
        fprintf(stderr, "Usage: dropbox_server -p portNum\n");
        exit(1);
    }

    addr.sin_port = htons(portNum);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;

    char hb[256];
    gethostname(hb, sizeof(hb));
    struct hostent *h_ent;
    h_ent = gethostbyname(hb);
    char *serverIP = inet_ntoa(*((struct in_addr*) h_ent->h_addr_list[0]));
    server_tuple.IP_Address = inet_addr(serverIP);
    server_tuple.portNum = addr.sin_port;

    int sock, enable;
    socklen_t socklen = sizeof(addr);

    if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation");
        exit(1);
    }

    enable = 1;

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        perror("setsockopt");
        close(sock);
        exit(1);
    }

    if (bind(sock, (struct sockaddr *) &addr, socklen)) {
        perror("Bind");
        exit(1);
    }

    if (listen(sock, 100)) {
        perror("Listen");
        exit(1);
    }

    signal(SIGINT, sig_h);
    signal(SIGPIPE, sig_h);

    int sock_d, clients_logged_on = 0;
    struct sockaddr_in client = {0};
    socklen_t client_socklen = (socklen_t) sizeof(struct sockaddr_in);
    short msg;
    client_list *c_list = NULL;
    client_tuple c_tuple;
    struct pollfd fds;

    while (term_flag != 1) {

        fds.events = POLLIN;
        fds.revents = 0;
        fds.fd = sock;

        while (fds.revents != POLLIN && term_flag == 0) {
            poll(&fds, 1, 100);
        }

        if (term_flag == 1) {
            break;
        }

        sock_d = accept(sock, (struct sockaddr *) &client, &client_socklen);
        if (read(sock_d, &msg, sizeof(msg)) <= 0) {
            perror("Read message from client");
            continue;
        }
        if (read(sock_d, &c_tuple, sizeof(client_tuple)) <= 0) {
            perror("Read client tuple after a message");
            break;
        }
        switch (msg) {
            case _LOG_ON:
                if (!(in_client_list(c_list, &c_tuple))) {
                    client_list_add(&c_list, &c_tuple);
                    notify_clients_USER_ON(c_list, &c_tuple);
                    clients_logged_on++;
                    fprintf(stdout, "Client @%s:%hu logged on.\n", inet_ntoa(*((struct in_addr *) &c_tuple.IP_Address)), ntohs(c_tuple.portNum));

                    fds.events = POLLOUT;
                    fds.revents = 0;
                    fds.fd = sock_d;

                    while (fds.revents != POLLOUT && term_flag == 0) {
                        poll(&fds, 1, 100);
                    }

                    if (term_flag == 1) {
                        break;
                    }

                    if (write(sock_d, &server_tuple, sizeof(client_tuple)) <= 0) {
                        perror("Send server's tuple to client at LOG_ON");
                        break;
                    }
                }
                else {
                    fprintf(stderr, "Client trying to log on has not logged off.\n");
                    close(sock_d);  /* Forcefully */
                }
                break;
            case _GET_CLIENTS:
                if (!(in_client_list(c_list, &c_tuple))) {
                    if (write(sock_d, &IP_PORT_NOT_FOUND, sizeof(IP_PORT_NOT_FOUND)) <= 0) {
                        perror("Write ERROR_IP_PORT_NOT_FOUND_IN_LIST");
                    }
                    fprintf(stderr, "The client that requested for the client list is not logged on.\n");
                    break;
                }
                if (write(sock_d, &CLIENT_LIST, sizeof(CLIENT_LIST)) <= 0) {
                    perror("Write CLIENT_LIST");
                    break;
                }
                if (write(sock_d, &clients_logged_on, sizeof(clients_logged_on)) <= 0) {
                    perror("Write number of logged on clients");
                    break;
                }
                client_list *curr = c_list;
                while (curr->next != NULL) {
                    if (write(sock_d, curr->this_tuple, sizeof(client_tuple)) <= 0) {
                        perror("Write CLIENT_LIST client_tuple");
                        break;
                    }
                    curr = curr->next;
                }
                if (write(sock_d, curr->this_tuple, sizeof(client_tuple)) <= 0) {
                    perror("Write CLIENT_LIST client_tuple");
                    break;
                }
                break;
            case _LOG_OFF:
                if (!(in_client_list(c_list, &c_tuple))) {
                    if (write(sock_d, &IP_PORT_NOT_FOUND, sizeof(IP_PORT_NOT_FOUND)) <= 0) {
                        perror("Write ERROR_IP_PORT_NOT_FOUND_IN_LIST");
                    }
                    fprintf(stderr, "The client trying to log off has not logged on.\n");
                    break;
                }
                delete_from_client_list(&c_list, &c_tuple);
                clients_logged_on--;
                if (write(sock_d, &SUCC_LOG_OFF, sizeof(SUCC_LOG_OFF)) <= 0) {
                    perror("Write SUCCESSFUL_LOG_OFF");
                }
                fprintf(stdout, "Client @%s:%hu logged off.\n", inet_ntoa(*((struct in_addr *) &c_tuple.IP_Address)), ntohs(c_tuple.portNum));
                notify_clients_USER_OFF(c_list, &c_tuple);
                break;
            default:
                fprintf(stderr, "A client tried to communicate wrongly.\n");
                break;
        }
        /* The code below makes sure that the other end of the socket has read everything and closed its end */
        fds.events = POLLOUT;
        fds.revents = 0;
        fds.fd = sock_d;
        while (fds.revents == POLLOUT && term_flag == 0) {
            poll(&fds, 1, 100);
        }

        close(sock_d);
    }

    free_list(c_list);
    close(sock);
    fprintf(stdout, "The server exits.\n");

    exit(0);
}

/* Signal handler */
void sig_h(int signo) {
    if (signo == SIGINT) {
        term_flag = 1;
    }
}

/* Sends a USER_ON message for a new client to the clients already logged on */
void notify_clients_USER_ON(client_list *list, client_tuple *ct) {
    client_list *curr = list;
    int sock;
    short msg;
    struct pollfd fds;

    if (curr == NULL) {
        return;
    }

    struct sockaddr_in client = {0};

    while (curr->next != NULL) {
        client.sin_addr.s_addr = curr->this_tuple->IP_Address;
        client.sin_port = curr->this_tuple->portNum;
        client.sin_family = AF_INET;

        sock = socket(AF_INET, SOCK_STREAM, 0);

        if (connect(sock, (struct sockaddr *) &client, sizeof(client)) < 0) {
            perror("Connect to notify USER_ON");
            curr = curr->next;
            continue;
        }

        fds.events = POLLOUT;
        fds.revents = 0;
        fds.fd = sock;

        while (fds.revents != POLLOUT && term_flag == 0) {
            poll(&fds, 1, 100);
        }

        if (term_flag == 1) {
            close(sock);
            return;
        }

        if (write(sock, &USER_ON, sizeof(USER_ON)) <= 0) {
            perror("Write USER_ON");
            close(sock);
            curr = curr->next;
            continue;
        }

        if (write(sock, &server_tuple, sizeof(client_tuple)) <= 0) {
            perror("Write server's tuple");
            close(sock);
            curr = curr->next;
            continue;
        }

        fds.events = POLLIN;
        fds.revents = 0;
        fds.fd = sock;

        while (fds.revents != POLLIN && term_flag == 0) {
            if (poll(&fds, 1, 100) > 0) {
                break;
            }
        }

        if (term_flag == 1) {
            close(sock);
            return;
        }

        if (read(sock, &msg, sizeof(msg)) <= 0) {
            perror("Read approval of client");
            close(sock);
            return;
        }

        if (msg != OK) {
            close(sock);
            fprintf(stderr, "Sent notification to wrong client.\n");
            return;
        }

        fds.events = POLLOUT;
        fds.revents = 0;
        fds.fd = sock;

        while (fds.revents != POLLOUT && term_flag == 0) {
            poll(&fds, 1, 100);
        }

        if (term_flag == 1) {
            close(sock);
            return;
        }

        if (write(sock, ct, sizeof(client_tuple)) <= 0) {
            perror("Write client tuple");
            close(sock);
            curr = curr->next;
            continue;
        }

        fds.events = POLLOUT;
        fds.revents = 0;
        fds.fd = sock;
        while (fds.revents == POLLOUT && term_flag == 0) {
            poll(&fds, 1, 100);
        }
        close(sock);
        curr = curr->next;
    }

    /* The last client_list node is the one we notify the others about */
    return;
}

void notify_clients_USER_OFF(client_list *list, client_tuple *ct) {
    client_list *curr = list;
    int sock;
    short msg;
    struct pollfd fds;

    if (curr == NULL) {
        return;
    }

    struct sockaddr_in client;
    client.sin_family = AF_INET;
    socklen_t socklen = sizeof(struct sockaddr_in);

    while (curr->next != NULL) {
        client.sin_addr.s_addr = curr->this_tuple->IP_Address;
        client.sin_port = curr->this_tuple->portNum;

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(sock, (struct sockaddr *) &client, socklen) < 0) {
            perror("Connect to notify USER_OFF");
            curr = curr->next;
            continue;
        }

        fds.events = POLLOUT;
        fds.revents = 0;
        fds.fd = sock;

        while (fds.revents != POLLOUT && term_flag == 0) {
            poll(&fds, 1, 100);
        }

        if (term_flag == 1) {
            close(sock);
            return;
        }

        if (write(sock, &USER_OFF, sizeof(USER_OFF)) <= 0) {
            perror("Write USER_ON");
            close(sock);
            curr = curr->next;
            continue;
        }

        if (write(sock, &server_tuple, sizeof(client_tuple)) <= 0) {
            perror("Write server's tuple");
            close(sock);
            curr = curr->next;
            continue;
        }

        fds.events = POLLIN;
        fds.revents = 0;
        fds.fd = sock;

        while (fds.revents != POLLIN && term_flag == 0) {
            poll(&fds, 1, 100);
        }

        if (term_flag == 1) {
            close(sock);
            return;
        }

        if (read(sock, &msg, sizeof(msg)) <= 0) {
            perror("Read approval of client");
            close(sock);
            return;
        }

        if (msg != OK) {
            close(sock);
            fprintf(stderr, "Sent notification to wrong client.\n");
            return;
        }

        fds.events = POLLOUT;
        fds.revents = 0;
        fds.fd = sock;

        while (fds.revents != POLLOUT && term_flag == 0) {
            poll(&fds, 1, 100);
        }

        if (term_flag == 1) {
            close(sock);
            return;
        }

        if (write(sock, ct, sizeof(client_tuple)) <= 0) {
            perror("Write client tuple");
            close(sock);
            curr = curr->next;
            continue;
        }

        fds.events = POLLOUT;
        fds.revents = 0;
        fds.fd = sock;
        while (fds.revents == POLLOUT && term_flag == 0) {
            poll(&fds, 1, 100);
        }
        close(sock);
        curr = curr->next;
    }

    client.sin_addr.s_addr = curr->this_tuple->IP_Address;
    client.sin_port = curr->this_tuple->portNum;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sock, (struct sockaddr *) &client, socklen) < 0) {
        perror("Connect to notify USER_OFF");
        return;
    }

    fds.events = POLLOUT;
    fds.revents = 0;
    fds.fd = sock;

    while (fds.revents != POLLOUT && term_flag == 0) {
        poll(&fds, 1, 100);
    }

    if (term_flag == 1) {
        close(sock);
        return;
    }

    if (write(sock, &USER_OFF, sizeof(USER_OFF)) <= 0) {
        perror("Write USER_ON");
        close(sock);
        return;
    }

    if (write(sock, &server_tuple, sizeof(client_tuple)) <= 0) {
        perror("Write server's tuple");
        close(sock);
        curr = curr->next;
        return;
    }

    fds.events = POLLIN;
    fds.revents = 0;
    fds.fd = sock;

    while (fds.revents != POLLIN && term_flag == 0) {
        poll(&fds, 1, 100);
    }

    if (term_flag == 1) {
        close(sock);
        return;
    }

    if (read(sock, &msg, sizeof(msg)) <= 0) {
        perror("Read approval of client");
        close(sock);
        return;
    }

    if (msg != OK) {
        close(sock);
        fprintf(stderr, "Sent notification to wrong client.\n");
        return;
    }

    fds.events = POLLOUT;
    fds.revents = 0;
    fds.fd = sock;

    while (fds.revents != POLLOUT && term_flag == 0) {
        poll(&fds, 1, 100);
    }

    if (term_flag == 1) {
        close(sock);
        return;
    }

    if (write(sock, ct, sizeof(client_tuple)) <= 0) {
        perror("Write client tuple");
        close(sock);
        return;
    }

    fds.events = POLLOUT;
    fds.revents = 0;
    fds.fd = sock;
    while (fds.revents == POLLOUT && term_flag == 0) {
        poll(&fds, 1, 100);
    }

    close(sock);

    return;
}
