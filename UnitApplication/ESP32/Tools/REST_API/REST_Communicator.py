import requests
import json
import os

# Configuration
BASE_URL = "http://192.168.4.1/api/v1"
TIMEOUT = 5

def clear_screen():
    """Clears the terminal screen."""
    os.system('cls' if os.name == 'nt' else 'clear')

def print_header():
    """Prints the application header."""
    print("=" * 40)
    print(" ESP32 REST API Communicator")
    print("=" * 40)

def print_status(message):
    """Prints a status message."""
    print(f"\n[Status] {message}\n")

def print_request_details(method, url, headers=None, data=None):
    """Prints the details of an HTTP request."""
    print("\n--- Request ---")
    print(f"Method: {method}")
    print(f"URL: {url}")
    if headers:
        print("Headers:")
        for key, value in headers.items():
            print(f"  {key}: {value}")
    if data:
        print("Body:")
        print(data)
    print("---------------")

def get_status():
    """Handles the GET request to /status."""
    try:
        url = f"{BASE_URL}/status"
        print_request_details("GET", url)
        response = requests.get(url, timeout=TIMEOUT)
        response.raise_for_status()  # Raise an exception for bad status codes
        data = response.json()
        print_status(f"Current message: {data.get('message', 'N/A')}")
    except requests.exceptions.RequestException as e:
        print_status(f"Error getting status: {e}")

def update_status():
    """Handles the PUT request to /status."""
    try:
        new_message = input("Enter the new message: ")
        payload = {"message": new_message}
        headers = {'Content-Type': 'application/json'}
        url = f"{BASE_URL}/status"
        data = json.dumps(payload)
        print_request_details("PUT", url, headers, data)
        response = requests.put(url, data=data, headers=headers, timeout=TIMEOUT)
        response.raise_for_status()
        data = response.json()
        print_status(f"Status updated. New message: {data.get('message', 'N/A')}")
    except requests.exceptions.RequestException as e:
        print_status(f"Error updating status: {e}")

def main_menu():
    """Displays the main menu and handles user input."""
    while True:
        clear_screen()
        print_header()
        print("1. Get Status")
        print("2. Update Status")
        print("3. Exit")
        choice = input("\nEnter your choice: ")

        if choice == '1':
            get_status()
            input("Press Enter to continue...")
        elif choice == '2':
            update_status()
            input("Press Enter to continue...")
        elif choice == '3':
            break
        else:
            print_status("Invalid choice. Please try again.")
            input("Press Enter to continue...")

if __name__ == "__main__":
    main_menu()
