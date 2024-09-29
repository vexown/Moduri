import socket
import time

SERVER_IP = "192.168.1.50"  # Pico W's static IP address
SERVER_PORT = 12345          # Pico W's socket port
MESSAGE = "Yo from PC!"

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Print start message
print(f"Sending messages to {SERVER_IP}, port {SERVER_PORT}")

try:
    # Send message every 100 ms in a loop
    while True:
        sock.sendto(MESSAGE.encode(), (SERVER_IP, SERVER_PORT))
        time.sleep(0.1)  # 100 ms delay
except KeyboardInterrupt:
    print("\nStopped sending messages.")

finally:
    # Close the socket when done
    sock.close()

