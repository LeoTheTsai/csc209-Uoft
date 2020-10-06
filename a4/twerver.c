#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include "socket.h"

#ifndef PORT
    #define PORT 56590
#endif

#define LISTEN_SIZE 5
#define WELCOME_MSG "Welcome to CSC209 Twitter! Enter your username: "
#define SEND_MSG "send"
#define SHOW_MSG "show"
#define FOLLOW_MSG "follow"
#define UNFOLLOW_MSG "unfollow"
#define BUF_SIZE 256
#define MSG_LIMIT 8
#define FOLLOW_LIMIT 5

struct client {
    int fd;
    struct in_addr ipaddr;
    char username[BUF_SIZE];
    char message[MSG_LIMIT][BUF_SIZE];
    struct client *following[FOLLOW_LIMIT]; // Clients this user is following
    struct client *followers[FOLLOW_LIMIT]; // Clients who follow this user
    char inbuf[BUF_SIZE]; // Used to hold input from the client
    char *in_ptr; // A pointer into inbuf to help with partial reads
    struct client *next;
};


// Provided functions. 
void add_client(struct client **clients, int fd, struct in_addr addr);
void remove_client(struct client **clients, int fd);

// These are some of the function prototypes that we used in our solution 
// You are not required to write functions that match these prototypes, but
// you may find them helpful when thinking about operations in your program.

// Send the message in s to all clients in active_clients. 
void announce(struct client *active_clients, char *s);

// Move client c from new_clients list to active_clients list. 
void activate_client(struct client *c, 
    struct client **active_clients_ptr, struct client **new_clients_ptr);

// Find the network newline in string buf with size n.
int find_network_newline(const char *buf, int n);

// Compare two strings s1 and s2, return 1 if equal, 0 otherwise.
int compare_not_equal(char *s1, char *s2);

// Read in user's command, and perform the command.
void command(struct client *p, struct client **active_clients);

// When the user follows username, add username to this user's following list 
// and add this user to username's list of followers. Once this user follows username, 
// they are sent any messages that username sends.
void follow(struct client *p, struct client *other_client, struct client *active_clients);

// When the user unfollow username, remove the username from user's following list,
// and remove the user from username's follower list.
void unfollow(struct client *p, struct client *other_client, struct client *active_clients);

// Displays the previously sent messages of those users this user is following.
void show(struct client *p, struct client *active_clients);

// Send a message that is up to 140 characters long (not including the command send) 
// to all of this user's followers.
void send_msg(struct client *p, char *argument, struct client *active_clients);

// Close the socket connection and terminate.
void quit(struct client *p, struct client **active_clients);

// A error checking function for write. When client disconnect, the write call will occur
// error and called quit().
void _write_wrapper(char *s, struct client *p, struct client *active_clients);

// A error checking function for read. When client disconnect, the read call will occur
// error and called quit().
int _read_wrapper(char *s,int size, struct client *p, struct client *active_clients);

// A helper function for command(). Help to partial read the command message that the user send.
void command_partial_read(char *message, struct client *p, struct client *active_clients, char *error_message);

// Return 1 if the string s is repeated among active_clients' username. Return 0 otherwise.
struct client *find_equal_username(char *s, struct client *active_clients);

//void err(int sig);

// The set of socket descriptors for select to monitor.
// This is a global variable because we need to remove socket descriptors
// from allset when a write to a socket fails. 
fd_set allset;

/* 
 * Create a new client, initialize it, and add it to the head of the linked
 * list.
 */
void add_client(struct client **clients, int fd, struct in_addr addr) {
    struct client *p = malloc(sizeof(struct client));
    if (!p) {
        perror("malloc");
        exit(1);
    }

    printf("Adding client %s\n", inet_ntoa(addr));
    p->fd = fd;
    p->ipaddr = addr;
    p->username[0] = '\0';
    p->in_ptr = p->inbuf;
    p->inbuf[0] = '\0';
    p->next = *clients;

    for (int i = 0; i < FOLLOW_LIMIT; i++) {
		p->followers[i] = NULL;
		p->following[i] = NULL;
	}

    // initialize messages to empty strings
    for (int i = 0; i < MSG_LIMIT; i++) {
        p->message[i][0] = '\0';
    }

    *clients = p;
}

/* 
 * Remove client from the linked list and close its socket.
 * Also, remove socket descriptor from allset.
 */
void remove_client(struct client **clients, int fd) {
    struct client **p;

    for (p = clients; *p && (*p)->fd != fd; p = &(*p)->next)
        ;
    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        // TODO: Remove the client from other clients' following/followers
        // lists
        for (int i = 0; i < FOLLOW_LIMIT; i++) {
            struct client *person_following = (*p)->following[i];
            if (person_following != NULL) {
                for (int j = 0; j < FOLLOW_LIMIT; j++) {
                    if (person_following->followers[j]->fd == (*p)->fd) {
                        person_following->followers[j] = NULL;
                        break;
                    }
                }
            }
        }
        for (int i = 0; i < FOLLOW_LIMIT; i++) {
            struct client *person_followers = (*p)->followers[i];
            if (person_followers != NULL) {
                for (int j = 0; j < FOLLOW_LIMIT; j++) {
                    if (person_followers->following[j]->fd == (*p)->fd) {
                        person_followers->following[j] = NULL;
                        break;
                    }
                }
            }
        }
        
        // Remove the client
        struct client *t = (*p)->next;
        printf("Removing client %d %s\n", fd, inet_ntoa((*p)->ipaddr));
        FD_CLR((*p)->fd, &allset);
        close((*p)->fd);
        free(*p);
        *p = t;
    } else {
        fprintf(stderr, 
            "Trying to remove fd %d, but I don't know about it\n", fd);
    }
}

void activate_client(struct client *c, 
    struct client **active_clients_ptr, struct client **new_clients_ptr) {

    struct client *p;
    if (c->fd == (*new_clients_ptr)->fd) {
        *new_clients_ptr = (*new_clients_ptr)->next;
    }else {
        for (p = *new_clients_ptr; p && p->next->fd != c->fd; p = p->next);
        p = p->next->next;
    }


    c->next = *active_clients_ptr;
    *active_clients_ptr = c;
}

void announce(struct client *active_clients, char *s) {
    struct client *p;
    for (p = active_clients; p; p = p->next){
        //_write_wrapper(s, p, active_clients);
        if (write(p->fd, s, strlen(s)) != strlen(s)) {
            //printf("%s\n", p->username);
            perror("write");
            close(p->fd);
            exit(1);
        }
    }
}

int find_network_newline(const char *buf, int n) {
    for (int i = 0; i < n - 1; i++){
        if (buf[i] == '\r' && buf[i + 1] == '\n'){
            return i + 2;
        }
    }
    return -1;
}



int compare_equal(char *s1, char *s2) {
    for (int i = 0; i < strlen(s1); i++) {
        if (s1[i] != s2[i]) {
            return 0;
        }
    }
    return 1;
}

void command_partial_read(char *message, struct client *p, struct client *active_clients, char *error_message) {
    int inbuf = 0;           // How many bytes currently in buffer?
    int room = BUF_SIZE;  // How many bytes remaining in buffer?
    //char *after = message;       // Pointer to position after the data in buf
    p->in_ptr = p->inbuf;

    int nbytes;
    if ((nbytes = _read_wrapper(p->in_ptr, room, p, active_clients)) > 0) {
        // if nbytes == 2, it means that the user input a blank line.
        if (nbytes == 2) {
            _write_wrapper(error_message, p, active_clients);
            //command_partial_read(message, p, active_clients, error_message);
            return;
        }
        inbuf += nbytes;
        int where;
        printf("Read %d bytes\n", inbuf);
        if ((where = find_network_newline(p->inbuf, inbuf)) > 0) {
            printf("Found newline\n");
            p->inbuf[where - 2] = '\0';
            strcpy(message, p->inbuf);
            inbuf -= where;
            memmove(p->inbuf, &p->inbuf[where],  inbuf);
            return;
        }
        room = sizeof(p->inbuf) - inbuf;
        p->in_ptr = &p->inbuf[inbuf];
    }
}

void command(struct client *p, struct client **active_clients) {
    char message[BUF_SIZE] = {'\0'};
    char command[BUF_SIZE] = {'\0'};
    char argument[BUF_SIZE] = {'\0'};
    struct client *other_client;
    command_partial_read(message, p, *active_clients, "Invalid command\r\n");
    char *white_space = strchr(message, ' ');
    if (message[0] == '\0' || (message[0] == '\r' && message[1] == '\n')) {
        return;
    }else if (white_space == NULL) {
        strncpy(command, message, strlen(message) -  2);
    }else {
        strncpy(command, message, white_space - message);
        strcpy(argument, white_space + 1);
        other_client = find_equal_username(argument, *active_clients);
    }
    if (compare_equal(command, FOLLOW_MSG)) {
        follow(p, other_client, *active_clients);
    } else if (compare_equal(command, UNFOLLOW_MSG)) {
        unfollow(p, other_client, *active_clients);
    } else if (compare_equal(command, SHOW_MSG)) {
        show(p, *active_clients);
    } else if (compare_equal(command, SEND_MSG)) {
        send_msg(p, argument, *active_clients);
    } else if (compare_equal(command, "quit")) {
        quit(p, active_clients);
    }else {
        char *error_message = "Invalid command\r\n";
        _write_wrapper(error_message, p,  *active_clients);
    }
    return;
}

void follow(struct client *p, struct client *other_client, struct client *active_clients) {
    int p_following = 0, other_follower = 0;
    if (other_client == NULL) {
        char *unseccessful = "Unsuccessful: follow name does not exits\r\n";
        _write_wrapper(unseccessful, p, active_clients);
        return;
    }
    while (p_following < FOLLOW_LIMIT) {
        if (p->following[p_following] != NULL) {
            p_following += 1;
        }else{
            break;
        }
    }
    while (other_follower < FOLLOW_LIMIT) {
        if (other_client->followers[other_follower] != NULL) {
            other_follower += 1;
        }else{
            break;
        }
    }
    if (p_following < FOLLOW_LIMIT && other_follower < FOLLOW_LIMIT) {
        p->following[p_following] = other_client;
        other_client->followers[other_follower] = p;
        printf("%s is now following %s\n", p->username, other_client->username);
    }else {
        char *unsuccessful = "Unsuccessful: follow\r\n";
        _write_wrapper(unsuccessful, p, active_clients);
    }
}

void unfollow(struct client *p, struct client *other_client, struct client *active_clients) {
    if (other_client == NULL) {
        char *unseccessful = "Unsuccessful: Unfollow name does not exits\r\n";
        _write_wrapper(unseccessful, p, active_clients);
        return;
    }
    for (int i = 0; i < FOLLOW_LIMIT; i++) {
        if (p->following[i]->username == other_client->username) {
            p->following[i] = NULL;
        }
    }
    for (int i = 0; i < FOLLOW_LIMIT; i++) {
        if (other_client->followers[i]->username == p->username) {
            other_client->followers[i] = NULL;
        }
    }
    printf("%s unfollow %s\n", p->username, other_client->username);
}

void show(struct client *p, struct client *active_clients) {
    struct client *c;
    int message_place = 0; // the index of the message
    for (int i = 0; i < FOLLOW_LIMIT; i++) {
        c = p->following[i];
        if (c != NULL) {
            while (message_place < MSG_LIMIT) {
                if (!compare_equal(c->message[message_place], "\0")) {
                    message_place += 1;
                }else{
                    break;
                }                
            }
            printf("%s : show\n", p->username);
            _write_wrapper(c->message[message_place - 1], p, active_clients);
        }
    }
}

void send_msg(struct client *p, char *argument, struct client *active_clients) {
    int i = 0;
    while (i < MSG_LIMIT) {
        if (!compare_equal(p->message[i], "\0")) {
            i += 1;
        }else {
            break;
        }
    }
    if (i < MSG_LIMIT) {
        strcpy(p->message[i], argument);
        printf("%s send %s\n", p->username, p->message[i]);
        strcat(p->message[i], "\r\n");
    }else {
        char *unsuccessful = "Unsuccessful: Number of send exceeds limit.";
        _write_wrapper(unsuccessful, p, active_clients);
    }
}


void quit(struct client *p, struct client **active_clients){
    char *username = p->username;
    char message[BUF_SIZE] = {'\0'};
    strcpy(message, username);
    strcat(message, " disconnected\r\n");
    remove_client(active_clients, p->fd);
    announce(*active_clients, message);

}


void _write_wrapper(char *s, struct client *p, struct client *active_clients){
    if (write(p->fd, s, strlen(s)) < 0) {
        quit(p, &active_clients);
    }
}


int _read_wrapper(char *s,int size, struct client *p, struct client *active_clients){
    int num_read;
    if ((num_read = read(p->fd, s, size)) <= 0) {
        quit(p, &active_clients);
    }
    return num_read;
}


// void control_c_handle(){
//     struct sigaction sa1;
//     sa1.sa_handler = err;
//     sa1.sa_flags = 0;
//     sigemptyset(&sa1.sa_mask);
//     if (sigaction(SIGKILL, &sa1, NULL) == -1) {
//         perror("sigaction");
//         exit(1);
//     }
// }

struct client *find_equal_username(char *s, struct client *active_clients) {
    struct client *active;
    for (active = active_clients; active != NULL; active = active->next) {
        if (compare_equal(s, active->username)) {
            return active;
        }
    }
    return NULL;
}

// void err(int sig){
// 	printf("Removing client %d\n", getpid());
// 	exit(0);
// }


int main (int argc, char **argv) {
    int clientfd, maxfd, nready;
    struct client *p;
    struct sockaddr_in q;
    fd_set rset;

    // If the server writes to a socket that has been closed, the SIGPIPE
    // signal is sent and the process is terminated. To prevent the server
    // from terminating, ignore the SIGPIPE signal. 
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // struct sigaction sa1;
    // sa1.sa_handler = err;
    // sa1.sa_flags = 0;
    // sigemptyset(&sa1.sa_mask);
    // if (sigaction(SIGKILL, &sa1, NULL) == -1) {
    //     perror("sigaction");
    //     exit(1);
    // }


    // A list of active clients (who have already entered their names). 
    struct client *active_clients = NULL;

    // A list of clients who have not yet entered their names. This list is
    // kept separate from the list of active clients, because until a client
    // has entered their name, they should not issue commands or 
    // or receive announcements. 
    struct client *new_clients = NULL;

    struct sockaddr_in *server = init_server_addr(PORT);
    int listenfd = set_up_server_socket(server, LISTEN_SIZE);
    free(server);

    // Initialize allset and add listenfd to the set of file descriptors
    // passed into select 
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    // maxfd identifies how far into the set to search
    maxfd = listenfd;

    while (1) {
        // make a copy of the set before we pass it into select
        rset = allset;

        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready == -1) {
            perror("select");
            exit(1);
        } else if (nready == 0) {
            continue;
        }

        // check if a new client is connecting
        if (FD_ISSET(listenfd, &rset)) {
            printf("A new client is connecting\n");
            clientfd = accept_connection(listenfd, &q);

            FD_SET(clientfd, &allset);
            if (clientfd > maxfd) {
                maxfd = clientfd;
            }
            printf("Connection from %s\n", inet_ntoa(q.sin_addr));
            add_client(&new_clients, clientfd, q.sin_addr);
            char *greeting = WELCOME_MSG;
            if (write(clientfd, greeting, strlen(greeting)) == -1) {
                fprintf(stderr, 
                    "Write to client %s failed\n", inet_ntoa(q.sin_addr));
                remove_client(&new_clients, clientfd);
            }
        }

        // Check which other socket descriptors have something ready to read.
        // The reason we iterate over the rset descriptors at the top level and
        // search through the two lists of clients each time is that it is
        // possible that a client will be removed in the middle of one of the
        // operations. This is also why we call break after handling the input.
        // If a client has been removed, the loop variables may no longer be 
        // valid.
        int cur_fd, handled;
        for (cur_fd = 0; cur_fd <= maxfd; cur_fd++) {
            if (FD_ISSET(cur_fd, &rset)) {
                handled = 0;
                //printf("77\n");

                // Check if any new clients are entering their names
                for (p = new_clients; p != NULL; p = p->next) {
                    if (cur_fd == p->fd) {
                        // TODO: handle input from a new client who has not yet
                        // entered an acceptable name
                        command_partial_read(p->username, p, new_clients, "Please enter a valid name\r\n");
                        if (p->username[0] == '\0') {
                            //_write_wrapper("Please enter a valid name\r\n", p, new_clients);
                        }else if (find_equal_username(p->username, active_clients)) {
                            p->username[0] = '\0';
                            _write_wrapper("Please enter a valid name\r\n", p, new_clients);
                        }else {
                            //Username is valid, activate the client and announce        
                            char username[BUF_SIZE] = {'\0'};
                            strcpy(username, p->username);
                            printf("%s has just joined\n", p->username);
                            activate_client(p, &active_clients, &new_clients);
                            announce(active_clients, strcat(username, " has just joined\r\n"));
                        }
                        handled = 1;
                        break;
                    }
                }
                if (!handled) {
                    // Check if this socket descriptor is an active client
                    for (p = active_clients; p != NULL; p = p->next) {
                        if (cur_fd == p->fd) {
                            // TODO: handle input from an active client
                            command(p, &active_clients);
                            break;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
