#!/usr/bin/python3
import time
import os

print("Content-Type: text/html")
print("")

print("<html>")
print("<head><title>Slow CGI Test</title></head>")
print("<body>")
print("<h1>Slow CGI Script</h1>")
print("<p>This script sleeps for 5 seconds...</p>")

time.sleep(20)

print("<p>Done sleeping!</p>")
print("</body>")
print("</html>")

