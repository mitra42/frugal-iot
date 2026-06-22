#!/usr/bin/env python3
"""
Convert platformio.ini to platform.h for Arduino C++
"""

import re
import sys
from pathlib import Path
from typing import List, Tuple, Optional

class PlatformIOConverter:
    def __init__(self, input_file: str):
        self.input_file = input_file
        self.lines = []
        self.board_defines = {
            "lolin_c3_mini": "ARDUINO_LOLIN_C3_PICO",
            "lolin_s2_mini": "ARDUINO_LOLIN_S2_MINI",
            "nodemcu-32s": "ARDUINO_NodeMCU_32S",
            "d1_mini_pro": "ARDUINO_ESP8266_WEMOS_D1MINIPRO",
            "d1_mini": "ARDUINO_ESP8266_WEMOS_D1MINI",
            "ttgo-lora32-v21": "ARDUINO_TTGO_LoRa32_v21new",
            "lilygo-t3-s3": "ARDUINO_LILYGO_T3_S3_V1_X",
            "esp32-c3-devkitm-1": "ARDUINO_ESP32C3_DEV",
        }

    def read_file(self):
        """Read the platformio.ini file"""
        with open(self.input_file, 'r') as f:
            self.lines = f.readlines()

    def convert_comment(self, line: str) -> str:
        """Convert ; comments to // but preserve other content"""
        result = []
        i = 0
        in_double_quotes = False
        in_single_quotes = False
        
        while i < len(line):
            char = line[i]
            
            # Track quote state
            if char == '"' and (i == 0 or line[i-1] != '\\'):
                in_double_quotes = not in_double_quotes
                result.append(char)
            elif char == "'" and (i == 0 or line[i-1] != '\\'):
                in_single_quotes = not in_single_quotes
                result.append(char)
            elif char == ';' and not in_double_quotes and not in_single_quotes:
                # Found a comment marker
                result.append('//')
                result.append(line[i+1:])
                break
            else:
                result.append(char)
            i += 1
        
        return ''.join(result)

    def extract_define(self, line_content: str) -> Optional[str]:
        """Extract and convert a single -D flag from line content"""
        # Remove leading/trailing whitespace and quotes
        content = line_content.strip()
        
        # Strip outer single quotes if present
        if content.startswith("'") and content.endswith("'"):
            content = content[1:-1]
        
        # Look for -D pattern
        match = re.search(r"-D\s+([A-Za-z_][A-Za-z0-9_]*)(?:=(.+))?", content)
        
        if match:
            name = match.group(1)
            value = match.group(2)
            
            if value:
                value = value.strip()
                # Remove trailing quotes or other junk
                if value.endswith("'"):
                    value = value[:-1]
                
                # Check if it's a number (including floats with F suffix)
                if re.match(r'^[\d.]+[FfLl]?$', value):
                    return f"#define {name} {value}"
                else:
                    # It's a string, keep quotes
                    return f"#define {name} {value}"
            else:
                return f"#define {name}"
        
        return None

    def ensure_newline(self, line: str) -> str:
        """Ensure line ends with newline"""
        if line and not line.endswith('\n'):
            return line + '\n'
        return line

    def process_single_line(self, line: str) -> str:
        """Process a single line from the file"""
        stripped = line.lstrip()
        
        # Empty lines stay empty
        if not stripped:
            return line
        
        # Section headers - comment them out
        if stripped.startswith('[') and stripped.endswith(']\n'):
            return f"// {line}"
        if stripped.startswith('[') and stripped.endswith(']'):
            return f"// {line}\n"
        
        # Lines that are already comments (start with ;)
        if stripped.startswith(';'):
            # Convert the leading ; to //
            line = self.convert_comment(line)
            # Check if this comment contains a -D flag
            define = self.extract_define(line)
            if define:
                # It's a commented-out define - output it as a comment
                return f"// {define}\n"
            return self.ensure_newline(line)
        
        # Lines with -D flags (not commented)
        if '-D' in line:
            define = self.extract_define(line)
            if define:
                # Convert any trailing comment on the same line
                trailing_comment = ""
                if "//" in line or ";" in line:
                    # Extract the comment part
                    match = re.search(r'[;](.+)$', line)
                    if match:
                        trailing_comment = self.convert_comment(";" + match.group(1))
                
                result = define
                if trailing_comment:
                    result += " " + trailing_comment
                else:
                    result += '\n'
                return result
            # If we couldn't extract a define, fall through to comment it
            line = self.convert_comment(line)
            return f"// {self.ensure_newline(line)}"
        
        # Lines with ${...} variable references - comment them out
        if '${' in line:
            return f"// {self.ensure_newline(line)}"
        
        # Lines with = that are config lines - comment them out
        if '=' in stripped and not stripped.startswith('//'):
            return f"// {self.ensure_newline(line)}"
        
        # Lines that are already comments
        if stripped.startswith('//'):
            return self.ensure_newline(line)
        
        # Indented continuation lines (like library deps) - comment them out
        if line.startswith(' ') or line.startswith('\t'):
            if not stripped.startswith('//'):
                return f"// {self.ensure_newline(line)}"
            return self.ensure_newline(line)
        
        # Other non-section lines - comment them out
        if stripped and not stripped.startswith('['):
            return f"// {self.ensure_newline(line)}"
        
        return self.ensure_newline(line)

    def get_board_define(self, board_name: str) -> str:
        """Determine the #ifdef value for a board"""
        if board_name in self.board_defines:
            return self.board_defines[board_name]
        
        # Generate from board name
        return f"TODO_{board_name.upper().replace('-', '_')}"

    def process_nonenv_content(self) -> List[str]:
        """Extract and process non-[env:xxx] content"""
        output = []
        in_env_section = False
        
        for i, line in enumerate(self.lines):
            stripped = line.strip()
            
            # Check if we're entering an env section
            if stripped.startswith('[env:'):
                in_env_section = True
                continue
            
            # Check if we're entering a non-env section
            if stripped.startswith('[') and not stripped.startswith('[env:'):
                in_env_section = False
                # Process this non-env section header
                output.append(self.process_single_line(line))
                continue
            
            # Skip content inside env sections
            if in_env_section:
                continue
            
            # Process the line (non-env content)
            output.append(self.process_single_line(line))
        
        return output

    def process_env_sections(self) -> List[Tuple[str, List[str], Optional[str]]]:
        """Extract all [env:xxx] sections with their content"""
        sections = []
        current_env = None
        current_content = []
        
        for line in self.lines:
            stripped = line.strip()
            
            if stripped.startswith('[env:'):
                # New env section
                if current_env is not None:
                    sections.append((current_env, current_content))
                    current_content = []
                current_env = stripped[5:-1]  # Extract name from [env:name]
            elif current_env is not None:
                if stripped.startswith('['):
                    # End of current env section (new section started)
                    sections.append((current_env, current_content))
                    current_env = None
                    current_content = []
                else:
                    # Still in current env section
                    current_content.append(line)
        
        # Don't forget the last section
        if current_env is not None:
            sections.append((current_env, current_content))
        
        return sections

    def convert(self) -> str:
        """Main conversion logic"""
        self.read_file()
        
        output = []
        
        # Header
        output.append("// Autogenerated by generate_platform_h.py and not yet checked by a human\n")
        output.append("\n")
        output.append("/*\n")
        output.append("  This file is auto converted. And possibly manually edited, from platformio.ini so that it can be included by those using Arduino.ini\n")
        output.append("*/\n")
        output.append("\n")
        
        # Process non-env content
        nonenv_output = self.process_nonenv_content()
        output.extend(nonenv_output)
        output.append("\n")
        
        # Process env sections
        env_sections = self.process_env_sections()
        
        for env_name, section_lines in env_sections:
            board_name = None
            converted_lines = []
            
            # First pass: extract board name
            for line in section_lines:
                stripped = line.strip()
                if stripped.startswith('board') and '=' in stripped:
                    match = re.search(r'board\s*=\s*(.+?)(?:\s*;|$)', stripped)
                    if match:
                        board_name = match.group(1).strip()
            
            # Second pass: convert all lines
            for line in section_lines:
                converted_lines.append(self.process_single_line(line))
            
            # Output the env section
            if board_name:
                board_define = self.get_board_define(board_name)
                output.append(f"#ifdef {board_define}\n")
                output.extend(converted_lines)
                output.append(f"#endif // {board_define}\n")
                output.append("\n")
    
        return ''.join(output)

    def write_output(self, output_file: str):
        """Write the converted content to output file"""
        content = self.convert()
        with open(output_file, 'w') as f:
            f.write(content)
        print(f"✓ Converted to {output_file}")

def main():
    if len(sys.argv) < 2:
        input_file = "platformio.ini"
        output_file = "platform.h"
    else:
        input_file = sys.argv[1]
        output_file = sys.argv[2] if len(sys.argv) > 2 else "platform.h"
    
    if not Path(input_file).exists():
        print(f"Error: {input_file} not found")
        sys.exit(1)
    
    converter = PlatformIOConverter(input_file)
    converter.write_output(output_file)

if __name__ == "__main__":
    main()