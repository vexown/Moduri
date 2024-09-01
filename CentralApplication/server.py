# Currently this only works if I run it directly on a Windows machine, does not work when run within WSL

import socket

def start_server():
    server_ip = ""  # Bind to all available IP addresses
    server_port = 12345

    # Create a socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((server_ip, server_port))
    server_socket.listen(1)

    print(f"Server listening on port {server_port}")

    while True:
        client_socket, client_address = server_socket.accept()
        print(f"Connection from {client_address}")

        message = client_socket.recv(1024).decode()
        print(f"Received message: {message}")

        client_socket.close()

if __name__ == "__main__":
    start_server()
