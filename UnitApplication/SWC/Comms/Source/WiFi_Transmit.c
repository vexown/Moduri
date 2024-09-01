#include <stdio.h>
#include <string.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>

/**
 * Sends a message over a TCP socket to a specified IP address and port.
 *
 * @param message The message to send.
 */
void send_message(const char *message) 
{
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int bytes_sent;

    // Create a TCP socket
    // AF_INET: IPv4 protocol, SOCK_STREAM: TCP (stream) socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return;
    }

    // Resolve the server's hostname (IP address in this case)
    server = gethostbyname("192.168.1.194");
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        close(sockfd);  // Clean up the socket before returning
        return;
    }

    // Set up the server address structure
    memset((char *)&serv_addr, 0, sizeof(serv_addr));  // Zero out the structure
    serv_addr.sin_family = AF_INET;  // Set address family to IPv4
    serv_addr.sin_port = htons(12345);  // Set the server port, converting to network byte order

    // Copy the server's IP address to the server address structure
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);

    // Connect the socket to the server's address
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        close(sockfd);  // Clean up the socket before returning
        return;
    }

    // Send the message to the server
    bytes_sent = send(sockfd, message, strlen(message), 0);
    if (bytes_sent < 0) {
        perror("ERROR writing to socket");
        close(sockfd);  // Clean up the socket before returning
        return;
    }

    // Close the socket after the message is sent
    close(sockfd);
}