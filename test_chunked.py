import socket
import time

HOST = '127.0.0.1'
PORT = 8080

def test_chunked_upload():
    try:
        # 1. Create a raw socket
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((HOST, PORT))
        print(f"[+] Connected to {HOST}:{PORT}")

        # 2. Send Headers
        # Note: We do NOT send Content-Length. We send Transfer-Encoding: chunked
        headers = (
            "POST /chunked_test HTTP/1.1\r\n"
            "Host: localhost:8080\r\n"
            "Transfer-Encoding: chunked\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
        )
        client.send(headers.encode())
        print("[+] Headers sent. Waiting 1 second...")
        time.sleep(1)

        # 3. Send Chunk 1
        # Format: HexSize\r\nData\r\n
        # "Hello " is 6 bytes -> Hex is 6
        chunk1 = "6\r\nHello \r\n"
        client.send(chunk1.encode())
        print(f"[+] Chunk 1 sent: {repr(chunk1)}")
        
        print("[+] Sleeping for 2 seconds (testing your timeout logic)...")
        time.sleep(2)

        # 4. Send Chunk 2
        # "World!" is 6 bytes -> Hex is 6
        chunk2 = "6\r\nWorld!\r\n"
        client.send(chunk2.encode())
        print(f"[+] Chunk 2 sent: {repr(chunk2)}")
        time.sleep(1)

        # 5. Send End Chunk
        # Format: 0\r\n\r\n
        end_chunk = "0\r\n\r\n"
        client.send(end_chunk.encode())
        print("[+] End chunk sent.")

        # 6. Receive Response
        response = client.recv(4096)
        print("\n[Server Response]:")
        print(response.decode())

        client.close()

    except Exception as e:
        print(f"[-] Error: {e}")

if __name__ == "__main__":
    test_chunked_upload()
