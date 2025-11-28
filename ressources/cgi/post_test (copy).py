#!/usr/bin/env python3
import sys

print("Content-Type: text/plain\r\n")
print(sys.stdin.read())

#curl -X POST -d "name=Bob&age=42" http://localhost:8080/cgi-bin/post_test.py
