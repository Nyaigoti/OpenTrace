import socket
import argparse
import sys

def run_udp_server(host, port):
    """
    Runs a simple UDP server to print incoming packets.
    """
    try:
        # Create a UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
        # Bind the socket to the address and port
        server_address = (host, port)
        print(f"Starting UDP server on {host} port {port}...")
        try:
            sock.bind(server_address)
        except PermissionError:
            print(f"Error: Permission denied. Try using a port > 1024 or run with elevated privileges.")
            return
        except OSError as e:
            print(f"Error binding to {host}:{port}: {e}")
            return

        while True:
            print("\nWaiting to receive message...")
            data, address = sock.recvfrom(4096)
            
            print(f"Received {len(data)} bytes from {address}")
            
            # Parse 'seal_payload_t': <16s (ID) B (Status)
            # Size: 16 + 1 = 17 bytes
            try:
                import struct
                if len(data) == 17:
                    device_id, status = struct.unpack('<16sB', data)
                    
                    # Clean up Device ID (bytes to hex or string)
                    dev_id_str = device_id.hex()
                    
                    print(f"  [Parsed Payload]")
                    print(f"  Device ID  : {dev_id_str}")
                    print(f"  Status Code: 0x{status:02X} ({'OPENED' if status == 0x01 else 'UNKNOWN'})")
                else:
                     print(f"  [Raw Data]: {data.hex()} (Length mismatch, expected 17)")

            except Exception as e:
                print(f"  Parsing Error: {e}")
                print(f"  Raw Data: {data.hex()}")

    except KeyboardInterrupt:
        print("\nServer stopping...")
    except Exception as e:
        print(f"\nUnexpected error: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Simple UDP Server for IoT Testing')
    parser.add_argument('--host', default='0.0.0.0', help='Host to bind to (default: 0.0.0.0)')
    parser.add_argument('--port', type=int, default=5000, help='Port to bind to (default: 5000)')
    
    args = parser.parse_args()
    run_udp_server(args.host, args.port)
