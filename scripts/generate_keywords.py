#!/usr/bin/env python3
# Generate keywords.txt, for the Arduino IDE's syntax highlighting, from the headers under a source
# directory. Run from the directory keywords.txt should be written to.
#
#   scripts/generate_keywords.py          the library: headers under src/
#   scripts/generate_keywords.py .        an application whose sketch is at the top of the repo
#
# Hidden directories, lib/ and build/ are skipped, which only matters for "." - there they hold
# PlatformIO's downloaded libraries, the library symlink and Arduino's build output, whose classes
# are not this project's.
import os
import re
import sys

src_dir = sys.argv[1] if len(sys.argv) > 1 else "src"
SKIP = {"lib", "build"}
keywords = set()
methods = set()

for root, dirs, files in os.walk(src_dir):
    dirs[:] = sorted(d for d in dirs if not d.startswith(".") and d not in SKIP)
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
