import socket
import time

SERVER_HOST = '127.0.0.1' # localhost
SERVER_PORT = 8080

def send_order(order_string):
    """Sends an order string to the TCP server and prints the response."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((SERVER_HOST, SERVER_PORT))
            print(f"Sending: '{order_string}'")
            s.sendall(order_string.encode('utf-8'))
            
            response_data = b""
            while True: # Loop to receive all parts of the response
                chunk = s.recv(1024)
                if not chunk:
                    break
                response_data += chunk
            
            response_str = response_data.decode('utf-8')
            print(f"Received:\n{response_str.strip()}")
            print("-" * 30)

    except ConnectionRefusedError:
        print(f"Connection refused. Is the server running on {SERVER_HOST}:{SERVER_PORT}?")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    print("HFT Test Client")
    print(f"Connecting to server at {SERVER_HOST}:{SERVER_PORT}")
    print("Order format: SYMBOL,SIDE,PRICE,QUANTITY,CLIENT_ORDER_ID")
    print("Example: AAPL,BUY,150.50,10,C001")
    print("-" * 30)

    # Test scenarios
    send_order("MSFT,BUY,300.00,50,B001\n") # Add a buy order
    time.sleep(0.1) # Small delay
    
    send_order("MSFT,BUY,300.50,20,B002\n") # Add another buy order (higher price)
    time.sleep(0.1)

    send_order("MSFT,SELL,301.00,30,S001\n") # Add a sell order (no match yet)
    time.sleep(0.1)

    send_order("MSFT,SELL,300.50,10,S002\n") # This sell should match B002 partially or fully
    time.sleep(0.1)
    
    send_order("MSFT,SELL,300.00,60,S003\n") # This sell should match remaining B002 and B001
    time.sleep(0.1)

    send_order("GOOG,BUY,2500.00,5,B101\n")
    time.sleep(0.1)
    
    send_order("GOOG,SELL,2499.00,3,S101\n") # Match with B101
    time.sleep(0.1)

    send_order("FAKE,SELL,10.00,INVALID,SXXX\n") # Test invalid quantity (server should reject)
    time.sleep(0.1)

    send_order("AAPL,BUY,150,10\n") # Test invalid format (server should reject)
    time.sleep(0.1)
    
    print("Test sequence finished.")