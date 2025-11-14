#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

char story[10000] = {0};
pthread_mutex_t story_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct session
{
    char name[64];
    char genre[32];
    char story[20000];
    pthread_mutex_t lock;
    FILE *log_fp;
    int participant_count;
    struct session *next;
} session_t;

session_t *sessions_head = NULL;
pthread_mutex_t sessions_lock = PTHREAD_MUTEX_INITIALIZER;

session_t *create_session(const char *name, const char *genre)
{
    pthread_mutex_lock(&sessions_lock);

    // Check if session name already exists
    session_t *current = sessions_head;

    while (current != NULL) 
    {
        if (strcmp(current->name, name) == 0) 
        {
            pthread_mutex_unlock(&sessions_lock);
            return NULL;
        }

        current = current->next;
    }

    // Allocate new session
    session_t *new_session = malloc(sizeof(session_t));

    if (!new_session) 
    {
        pthread_mutex_unlock(&sessions_lock);
        return NULL;
    }

    // Initialize fields
    strncpy(new_session->name, name, sizeof(new_session->name) - 1);
    new_session->name[sizeof(new_session->name) - 1] = '\0';

    strncpy(new_session->genre, genre, sizeof(new_session->genre) - 1);
    new_session->genre[sizeof(new_session->genre) - 1] = '\0';

    new_session->story[0] = '\0';
    new_session->participant_count = 0;
    new_session->log_fp = NULL;
    pthread_mutex_init(&new_session->lock, NULL);

    // Insert into linked list (head insertion)
    new_session->next = sessions_head;
    sessions_head = new_session;

    pthread_mutex_unlock(&sessions_lock);
    return new_session;
}

session_t *find_session(const char *name) 
{
    pthread_mutex_lock(&sessions_lock);
    session_t *current = sessions_head;

    while(current != NULL) 
    {
        if(strcmp(current->name, name) == 0) 
        {
            pthread_mutex_unlock(&sessions_lock);
            return current;
        }

        current = current->next;
    }

    pthread_mutex_unlock(&sessions_lock);
    return NULL;
}

void *handle_client(void *arg) 
{
    int client_fd = *(int*)arg;
    char buffer[BUFFER_SIZE];

    while(1)
    {
        memset(buffer, 0, BUFFER_SIZE);

        if(read(client_fd, buffer, BUFFER_SIZE) <= 0)
        {
            break;
        }

        else
        {
            if(strncmp(buffer, "VIEW", 4) == 0)
            {
                pthread_mutex_lock(&story_lock);
                send(client_fd, story, strlen(story), 0);
                pthread_mutex_unlock(&story_lock);
            }

            else if(strncmp(buffer, "WRITE ", 6) == 0)
            {
                char story_copy[10000] = {0};
                strcpy(story_copy, buffer + 6);

                pthread_mutex_lock(&story_lock);
                strcat(story, story_copy);
                pthread_mutex_unlock(&story_lock);

                char *reply = "Added to story!\n";
                send(client_fd, reply, strlen(reply), 0);
            }

            else if(strncmp(buffer, "QUIT", 4) == 0)
            {
                char *reply = "Exiting current story.";
                send(client_fd, reply, strlen(reply), 0);
                break;
            }
        }
    }

    close(client_fd);
    return NULL;
}


int main() 
{
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    // 1. Create socket
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;   // Listen on all interfaces
    address.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 3. Listen for connections
    if(listen(server_fd, 1) < 0) 
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server running on port %d... waiting for connection.\n", PORT);

    while(1) 
    {
        int client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

        if (client_fd < 0) 
        {
            perror("accept");
            continue;
        }

        printf("New client connected!\n");

        pthread_t thread;
        int *pclient = malloc(sizeof(int));  // allocate memory so thread gets unique socket value
        *pclient = client_fd;

        pthread_create(&thread, NULL, handle_client, pclient);
        pthread_detach(thread); // don't require join, thread cleans up after itself
    }
}
