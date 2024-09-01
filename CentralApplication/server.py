import socket

def start_server():
    # Define the IP address and port for the server
    server_ip = "192.168.1.194"  # My PC's IP address assigned by the Tenda WiFi router DHCP server (may change)
    server_port = 12345          # Port number on which the server will listen for incoming connections (uint16 range)

    # Create a new socket using IPv4 addressing and TCP
    # socket.AF_INET: IPv4 addressing
    # socket.SOCK_STREAM: TCP (stream-oriented) socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Bind the socket to the specified IP address and port
    # The server will listen for incoming connections on this IP and port
    server_socket.bind((server_ip, server_port))

    # Enable the server to accept connections. The argument `1` specifies the maximum number of queued connections
    server_socket.listen(1)

    # Inform the user that the server is up and running
    print(f"Server listening on port {server_port}")

    # Continuously accept and handle incoming connections
    while True:
        # Accept a new connection. This call blocks until a connection is made
        client_socket, client_address = server_socket.accept()

        # Print the address of the connected client
        print(f"Connection from {client_address}")

        # Receive a message from the client
        # The buffer size is 1024 bytes (1 KB)
        message = client_socket.recv(1024).decode()

        # Print the received message
        print(f"Received message: {message}")

        # Close the connection with the client
        client_socket.close()

# Ensure that the `start_server` function is called only when the script is executed directly
if __name__ == "__main__":
    start_server()
