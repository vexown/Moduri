#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>

/* Server defines */
#define SERVER_IP_ADDRESS   "192.168.1.194"  //My PC's IP address assigned by the Tenda WiFi router DHCP server (may change)
#define SERVER_PORT         (uint16_t)12345  //TODO - server port for testing, later you can think about which one to use permanently

/* Misc defines */
#define E_OK                0
#define E_NOT_OK            -1
#define NO_FLAG             0

/**
 * Sends a message over a TCP socket to a specified IP address and port.
 *
 * @param message The message to send.
 */
void send_message_TCP(const char* message) 
{
    int bytes_sent;
    int client_socket;
    struct sockaddr_in server_socket;
    
    /* Create a socket for Pico W (client) */
    /* This is a wrapper around the netconn_new function that creates a new network connection 
       and associates it with a socket descriptor. It takes three arguments: 
            the domain (usually AF_INET for IPv4), 
            the type of socket (such as SOCK_STREAM for TCP or SOCK_DGRAM for UDP), 
            and the protocol (e.g., IPPROTO_TCP or IPPROTO_UDP) - in our case when we use SOCK_STREAM this parameter is unused (TCP implied). 
    The function then creates a netconn object based on the specified parameters, allocates a socket descriptor, and associates the netconn with the socket */
    if (client_socket = socket(AF_INET, SOCK_STREAM, 0 /*unused*/) != E_OK) 
    {
        printf("ERROR opening socket \n");
        return;
    }

    /* Set up the server socket configuration */
    memset((char *)&server_socket, 0, sizeof(server_socket)); // start with 0'd out parameters
    server_socket.sin_family = AF_INET; //server uses IPv4
    server_socket.sin_port = htons(SERVER_PORT);
    server_socket.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);

    /* Establish a connection between the client and the server TCP socket */ 
    /* It takes three arguments: the socket descriptor, the server address structure, and the length of the server address structure. 
        The function first checks if the socket is valid and if the remote address matches the socket type (IPv4 or IPv6). 
        It then extracts the remote IP address and port from the address structure and calls the netconn_connect function to establish the connection. */
    if (connect(client_socket, (struct sockaddr *)&server_socket, sizeof(server_socket)) != E_OK) 
    {
        printf("ERROR connecting \n");
    }

    /* Send the message */
    /* The lwip_send function in LwIP is a wrapper around the netconn_write_partly function that sends data over a TCP socket. 
       It takes four arguments: the socket descriptor, the data to be sent, the size of the data, and flags that specify additional options. */
    bytes_sent = send(client_socket, message, strlen(message), NO_FLAG);
    if (bytes_sent == E_NOT_OK) //send function returns either number of bytes sent or -1 if something went wrong
    {
        printf("ERROR writing to socket \n");
    }
    else if(bytes_sent != strlen(message))
    {
        printf("ERROR - not all bytes of the message were sent \n");
    }

    /* Close the socket */
    close(client_socket);
}