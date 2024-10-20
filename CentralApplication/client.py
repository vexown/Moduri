import socket
import time

# Constants
SERVER_IP = "192.168.1.50"  # Pico W's static IP address
SERVER_PORT = 4242          # Pico W's socket port
MESSAGE = "Yo from PC!"

# Function to send messages via UDP
def send_udp():
    # Create a UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    print(f"Sending messages via UDP to {SERVER_IP}, port {SERVER_PORT}")

    try:
        # Send message periodically in a loop
        while True:
            sock.sendto(MESSAGE.encode(), (SERVER_IP, SERVER_PORT))
            time.sleep(1) 
    except KeyboardInterrupt:
        print("\nStopped sending messages via UDP.")
    finally:
        sock.close()

# Function to send messages via TCP
def send_tcp():
    # Create a TCP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((SERVER_IP, SERVER_PORT))  # Connect to the server
    print(f"Sending messages via TCP to {SERVER_IP}, port {SERVER_PORT}")

    try:
        # Send message periodically in a loop
        while True:
            sock.sendall(MESSAGE.encode())  # Use sendall for TCP
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nStopped sending messages via TCP.")
    finally:
        sock.close()

# Main function
def main():
    # User input to select protocol
    protocol = input("Choose protocol (UDP/TCP): ").strip().lower()

    if protocol == "udp":
        send_udp()
    elif protocol == "tcp":
        send_tcp()
    else:
        print("Invalid protocol. Please enter 'UDP' or 'TCP'.")

if __name__ == "__main__":
    main()
