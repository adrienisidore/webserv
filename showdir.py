#!/usr/bin/env python3
import os

script_dir = os.path.dirname(os.path.abspath(__file__))

print("The script is acting in directory:", script_dir)

test_file = os.path.join(script_dir, "test_directory.txt")

with open(test_file, "w") as f:
    f.write(f"This is a test. The script is running in: {script_dir}\n")

print(f"File created: {test_file}")