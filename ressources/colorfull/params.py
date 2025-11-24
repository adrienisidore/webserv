import os
import urllib.parse

# Get query string from environment
query_string = os.environ.get('QUERY_STRING', '')

# Parse it
params = urllib.parse.parse_qs(query_string)
# params = {'name': ['john'], 'age': ['25']}

print("Content-Type: text/html")
print()
print("<!DOCTYPE html>")
print("<html><body>")
print(f"<h1>Search Results</h1>")
print(f"<p>Name: {params.get('name', [''])[0]}</p>")
print(f"<p>Age: {params.get('age', [''])[0]}</p>")
print("</body></html>")

