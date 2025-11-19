import sys
import os

if __name__ == "__main__":

    content_length = int(os.environ.get('CONTENT_LENGTH',0))

    if content_length > 0:
        body = sys.stdin.read(content_length)
    else:
        body = "no content length directive"


    print("Content-Type: text/html\r")
    print("\r")
    print(f"<html><body>Received (uppercase): {body.upper()}</body></html>")
