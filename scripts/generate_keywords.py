#!/opt/homebrew/bin/python3
# Save this as generate_keywords.py in your library root
import os
import re

src_dir = "src"
keywords = set()
methods = set()

for root, _, files in os.walk(src_dir):
    for file in files:
        if file.endswith(".h"):
            with open(os.path.join(root, file), "r") as f:
                lines = f.readlines()
                for line in lines:
                    # Find class names
                    match_class = re.match(r'\s*class\s+(\w+)', line)
                    if match_class:
                        keywords.add(match_class.group(1))
                    # Find public method names
                    match_method = re.match(r'\s*(?:virtual\s+)?(?:[\w:<>]+)\s+(\w+)\s*\(', line)
                    if match_method and not match_method.group(1) in ["if", "for", "while", "switch"]:
                        methods.add(match_method.group(1))

with open("keywords.txt", "w") as out:
    out.write("#######################################\n# Datatypes (KEYWORD1)\n#######################################\n\n")
    for k in sorted(keywords):
        out.write(f"{k}\tKEYWORD1\n")
    out.write("\n#######################################\n# Methods and Functions (KEYWORD2)\n#######################################\n\n")
    for m in sorted(methods):
        out.write(f"{m}\tKEYWORD2\n")

print("keywords.txt generated!")
