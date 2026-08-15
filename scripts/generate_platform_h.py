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
        # platform.h is read ONLY by the Arduino IDE (_settings.h includes it when PLATFORMIO is
        # undefined). The guards below are PlatformIO's spellings, which is safe because
        # _settings.h normalises the Arduino board macros to PlatformIO's names *before* it
        # includes platform.h - see the TO_ADD_BOARD block at the top of _settings.h, which is
        # the one place that knows about both spellings.
        #
        # Using PlatformIO's names also matters for LilyGo T3-S3 and T-Beam: in the IDE those
        # boards have a "Revision" radio menu where every option sets a different build.board
        # (LILYGO_T3S3_SX1262, _SX1276, ...), so no single Arduino macro would cover them.
        # Normalisation collapses all the variants, letting one guard per env work.
        #
        # If adding a board whose Arduino macro differs, add the alias to _settings.h too.
        # Find Arduino's spelling with:
        #   arduino-cli board details -b <fqbn> --show-properties | grep '^build.board='

        # Some envs need a different board in Arduino than in PlatformIO, so they cannot be
        # keyed on the platformio.ini board name. Checked before board_defines.
        self.env_defines = {
            # PlatformIO has no lolin_c3_pico board, so [env:c3_pico] uses lolin_c3_mini and
            # passes -D ARDUINO_LOLIN_C3_PICO to tell itself apart from a real c3_mini.
            # Arduino does have a LOLIN C3 Pico board, and it defines ARDUINO_LOLIN_C3_PICO
            # natively - so select that board (esp32:esp32:lolin_c3_pico) when testing.
            "c3_pico": "ARDUINO_LOLIN_C3_PICO",
        }
        self.board_defines = {
            "lolin_c3_mini": "ARDUINO_LOLIN_C3_MINI",
            "lolin_s2_mini": "ARDUINO_LOLIN_S2_MINI",
            "nodemcu-32s": "ARDUINO_NodeMCU_32S",           # Arduino: ARDUINO_NODEMCU_32S
            "d1_mini_pro": "ARDUINO_ESP8266_WEMOS_D1MINIPRO",
            "d1_mini": "ARDUINO_ESP8266_WEMOS_D1MINI",
            # Arduino's "TTGO LoRa32-OLED" board has a Board Revision menu; its v21new option
            # (Revision=TTGO_LoRa32_v21new) defines exactly this, matching PlatformIO
            "ttgo-lora32-v21": "ARDUINO_TTGO_LoRa32_v21new",
            "lilygo-t3-s3": "ARDUINO_LILYGO_T3_S3_V1_X",    # Arduino: ARDUINO_LILYGO_T3S3_<radio>
            "ttgo-t-beam": "ARDUINO_T_Beam",                # Arduino: ARDUINO_TBEAM_USE_RADIO_<radio>
            "heltec_wifi_lora_32_V3": "ARDUINO_heltec_wifi_lora_32_V3", # Arduino: uppercase HELTEC
            "esp32-c3-devkitm-1": "ARDUINO_ESP32C3_DEV",
            # esp32dev - PlatformIO and Arduino ("ESP32 Dev Module") agree on this one
            "esp32dev": "ARDUINO_ESP32_DEV",
            # C3 SuperMini - DELIBERATELY Arduino's macro rather than PlatformIO's, the one place
            # this table breaks its own convention. PlatformIO's nologo_esp32c3_super_mini board
            # defines ARDUINO_ESP32C3_DEV via extra_flags - the SAME macro as esp32-c3-devkitm-1 -
            # so using it would make an example containing both a devkit env and a supermini env
            # fire both blocks at once and fight over pin defines. Arduino gives the board its own
            # ARDUINO_NOLOGO_ESP32C3_SUPER_MINI, which is unambiguous, so use that and add no
            # alias in _settings.h (an alias would reintroduce exactly the collision).
            "nologo_esp32c3_super_mini": "ARDUINO_NOLOGO_ESP32C3_SUPER_MINI",
            # sonoff_basic intentionally left as PlatformIO's macro with NO alias: Arduino's only
            # Sonoff board is "ITEAD Sonoff" = ARDUINO_ESP8266_SONOFF_SV, and SV is different
            # hardware from the Basic this code targets. So the sonoff ESP8266 env cannot
            # currently be configured from the Arduino IDE - see notes in _settings.h.
            "sonoff_basic": "ARDUINO_ESP8266_SONOFF_BASIC",
        }

        # TO_ADD_BOARD - the label the board carries in the IDE's Tools > Board menu, which is the
        # only name a user of the Arduino IDE ever sees. The "no board configured" #error is aimed
        # at that user, so it lists these rather than the ARDUINO_* macros above: being told
        # "ARDUINO_NodeMCU_32S" does not tell you what to click.
        #
        # Keyed the same way as the two tables above - env first, then board. Get a label with:
        #   arduino-cli board listall | grep <fqbn>
        # and, for a board whose variant is chosen by a menu, the option's label with:
        #   arduino-cli board details -b <fqbn>
        # arduino_compile.bash checks these against the core, the same way it checks the macros,
        # so a label that goes stale is caught rather than quietly misdirecting someone.
        self.env_names = {
            "c3_pico": "LOLIN C3 Pico",
            "ttgo-lora32-v21": "TTGO LoRa32-OLED, with Board Revision = TTGO LoRa32 V2.1 (1.6.1)",
        }
        self.board_names = {
            "lolin_c3_mini": "LOLIN C3 Mini",
            "lolin_s2_mini": "LOLIN S2 Mini",
            "nodemcu-32s": "NodeMCU-32S",
            "d1_mini_pro": "LOLIN(WEMOS) D1 mini Pro",
            "d1_mini": "LOLIN(WEMOS) D1 R2 & mini",
            "ttgo-lora32-v21": "TTGO LoRa32-OLED",
            # Both of these pick their radio from a Board Revision menu, and _settings.h collapses
            # every option to one macro - so name the board and leave the revision to the user.
            "lilygo-t3-s3": "LilyGo T3-S3",
            "ttgo-t-beam": "T-Beam",
            "heltec_wifi_lora_32_V3": "Heltec WiFi LoRa 32(V3)",
            "esp32-c3-devkitm-1": "ESP32C3 Dev Module",
            "esp32dev": "ESP32 Dev Module",
            "nologo_esp32c3_super_mini": "Nologo ESP32C3 Super Mini",
            # No Arduino board matches sonoff_basic - see board_defines above
            "sonoff_basic": "(no matching Arduino board)",
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

    def strip_trailing_comment(self, text: str) -> str:
        """Return text up to (not including) the first ';' comment marker outside quotes"""
        in_double_quotes = False
        in_single_quotes = False
        for i, char in enumerate(text):
            if char == '"' and (i == 0 or text[i-1] != '\\'):
                in_double_quotes = not in_double_quotes
            elif char == "'" and (i == 0 or text[i-1] != '\\'):
                in_single_quotes = not in_single_quotes
            elif char == ';' and not in_double_quotes and not in_single_quotes:
                return text[:i]
        return text

    def extract_define(self, line_content: str) -> Optional[str]:
        """Extract and convert a single -D flag from line content"""
        # Remove leading/trailing whitespace and quotes
        content = line_content.strip()
        # Drop any inline "; comment" - it's converted separately - so it doesn't end up in the value
        content = self.strip_trailing_comment(content).strip()

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

    def extract_trailing_comment(self, line: str) -> str:
        """Return the //-converted trailing "; comment" on a -D line, if any"""
        match = re.search(r'[;](.+)$', line)
        if match:
            return self.convert_comment(";" + match.group(1))
        return ""

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
            # Drop just the leading ; marker - keep the rest raw so a -D flag's OWN trailing
            # "; comment" (if any) is still found by extract_define/extract_trailing_comment,
            # rather than being consumed as part of converting this leading marker.
            marker_index = line.index(';')
            rest = line[:marker_index] + line[marker_index + 1:]
            define = self.extract_define(rest)
            if define:
                # It's a commented-out define - output it as a comment, plus its own comment if any
                result = f"// {define}"
                trailing_comment = self.extract_trailing_comment(rest)
                if trailing_comment:
                    result += " " + trailing_comment
                return self.ensure_newline(result)
            line = self.convert_comment(line)
            return self.ensure_newline(line)

        # Lines with -D flags (not commented)
        if '-D' in line:
            define = self.extract_define(line)
            if define:
                result = define
                trailing_comment = self.extract_trailing_comment(line)
                if trailing_comment:
                    result += " " + trailing_comment
                return self.ensure_newline(result)
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

    def get_board_define(self, board_name: str, env_name: Optional[str] = None) -> str:
        """Determine the #ifdef value for an env, preferring an env-specific override"""
        if env_name and env_name in self.env_defines:
            return self.env_defines[env_name]
        if board_name in self.board_defines:
            return self.board_defines[board_name]

        # Generate from board name
        return f"TODO_{board_name.upper().replace('-', '_')}"

    def get_board_name(self, board_name: str, env_name: Optional[str] = None) -> str:
        """The Tools > Board label for an env, preferring an env-specific override.

        Falls back to the platformio.ini board name, which is at least recognisable, rather than
        inventing a label that would not be found in the menu.
        """
        if env_name and env_name in self.env_names:
            return self.env_names[env_name]
        if board_name in self.board_names:
            return self.board_names[board_name]
        return board_name

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
        self.env_platforms = {}  # env name -> 'espressif8266' | 'espressif32' | ''
        self.env_arduino_default = set()  # envs marked custom_arduino_default

        for line in self.lines:
            stripped = line.strip()

            # PlatformIO ignores options prefixed custom_ silently; a plain "arduino_default"
            # would print "Warning! Ignore unknown configuration option" on every single build.
            if current_env is not None and re.match(r'\s*custom_arduino_default\s*=', line):
                if re.search(r'=\s*(true|yes|1|on)\b', line, re.I):
                    self.env_arduino_default.add(current_env)

            if current_env is not None and re.match(r'\s*platform\s*=', line):
                self.env_platforms[current_env] = (
                    'espressif8266' if 'espressif8266' in line else 'espressif32')

            if stripped.startswith('[env:'):
                # New env section
                if current_env is not None:
                    sections.append((current_env, current_content))
                    current_content = []
                hdr = re.sub(r'\s*;.*$', '', stripped).strip()
                current_env = hdr[5:-1]  # Extract name from [env:name]
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

    def convert(self, family: Optional[str] = None, guard: str = "PLATFORM_H",
                output_name: str = "platform.h", marker: Optional[str] = None) -> str:
        """Main conversion logic.

        family: 'espressif8266' or 'espressif32' to emit only that chip's [env:] blocks, or
                None for all of them. platform.h is read only by the Arduino IDE on ESP32 and
                <sketch>.ino.globals.h only on ESP8266, so each carries just its own boards -
                the #ifdef guards already prevent cross-firing, this is for readability.
        marker: a macro to #define unconditionally, so the library can tell this file reached the
                build at all - as opposed to the settings being absent. Only meaningful for the
                ESP8266 file, which the user has to put in place by hand; see main().
        """
        self.read_file()

        self.env_blocks_emitted = 0
        self.guards_used = []
        self.names_used = []
        output = []

        # Header
        output.append("// Autogenerated by generate_platform_h.py and not yet checked by a human\n")
        output.append("\n")
        output.append("/*\n")
        output.append("  This file is auto converted. And possibly manually edited, from platformio.ini so that it can be included by those using Arduino.ini\n")
        output.append("*/\n")
        output.append("\n")
        output.append(f"#ifndef {guard}\n#define {guard}\n\n")

        if marker:
            output.append("// Tells the library this file made it into the build. Deliberately outside\n")
            output.append("// every #ifdef below: it says \"the file is here\", not \"your board is covered\",\n")
            output.append("// which is a separate failure with its own #error at the end.\n")
            output.append(f"#define {marker}\n\n")

        # Process non-env content
        nonenv_output = self.process_nonenv_content()
        output.extend(nonenv_output)
        output.append("\n")
        
        # Process env sections
        env_sections = self.process_env_sections()
        
        # The Arduino IDE has no concept of an environment, so only ONE env per board can be
        # active. Group by the guard macro, emit the env marked custom_arduino_default (or the
        # first one if none is marked), and keep the others visible but disabled behind #if 0.
        groups = []          # [(board_define, [(env_name, converted_lines), ...])]
        index_of = {}
        for env_name, section_lines in env_sections:
            if family and self.env_platforms.get(env_name, 'espressif32') != family:
                continue
            board_name = None
            for line in section_lines:
                stripped = line.strip()
                if stripped.startswith('board') and '=' in stripped:
                    match = re.search(r'board\s*=\s*(.+?)(?:\s*;|$)', stripped)
                    if match:
                        board_name = match.group(1).strip()
            if not board_name:
                continue
            board_define = self.get_board_define(board_name, env_name)
            converted_lines = [self.process_single_line(l) for l in section_lines]
            if board_define not in index_of:
                index_of[board_define] = len(groups)
                groups.append((board_define, []))
            groups[index_of[board_define]][1].append((env_name, converted_lines, board_name))

        for board_define, members in groups:
            # Which env wins for this board
            marked = [m for m in members if m[0] in self.env_arduino_default]
            if len(marked) > 1:
                names = ", ".join(m[0] for m in marked)
                print(f"! {board_define}: several envs set custom_arduino_default ({names}) - using the first")
            active = marked[0] if marked else members[0]
            others = [m for m in members if m[0] != active[0]]

            output.append(f"// ===== [env:{active[0]}] -> {board_define}\n")
            output.append(f"#ifdef {board_define}\n")
            # Marker for the catch-all at the end of the file. Namespaced because on ESP8266
            # this file is force-included into EVERY translation unit, the core's own
            # sources included, so a name like BOARD_FOUND could collide.
            output.append("#define FRUGAL_IOT_BOARD_CONFIGURED\n")
            output.extend(active[1])
            output.append(f"#endif // {board_define}\n")
            output.append("\n")
            self.env_blocks_emitted += 1
            if board_define not in self.guards_used:
                self.guards_used.append(board_define)
                self.names_used.append(self.get_board_name(active[2], active[0]))

            for env_name, converted_lines, _board_name in others:
                output.append(f"// ----- [env:{env_name}] also targets {board_define}, DISABLED\n")
                output.append(f"// Only one env per board can be active in the Arduino IDE, and\n")
                output.append(f"// [env:{active[0]}] is the one in effect. To use this one instead, set\n")
                output.append(f"//   custom_arduino_default = yes\n")
                output.append(f"// on [env:{env_name}] in platformio.ini (and remove it from any other env for\n")
                output.append(f"// this board), then re-run scripts/generate_platform_h.py.\n")
                output.append("#if 0\n")
                output.append(f"#ifdef {board_define}\n")
                output.extend(converted_lines)
                output.append(f"#endif // {board_define}\n")
                output.append("#endif // 0\n")
                output.append("\n")

        # Catch-all: if none of the blocks above matched, the selected board has no settings in
        # this file and the sketch would otherwise compile with library defaults only - a build
        # that looks fine and is not what anyone intended. Fail loudly instead.
        if self.guards_used:
            # Board menu labels, not the ARDUINO_* macros - whoever reads this error is sitting in
            # front of Tools > Board and needs to know what to pick there. The macros are still on
            # every #ifdef above for anyone debugging the file itself.
            # Separated by ' / ' and NOT quoted: the whole message is one C string literal, so an
            # inner " would end it early and the file would not even parse.
            supported = " / ".join(self.names_used)
            output.append("#ifndef FRUGAL_IOT_BOARD_CONFIGURED\n")
            output.append(f'  #error "This board has no settings in {output_name}. Under Tools > Board, '
                          f'select one of the boards this example supports, or add a section for yours to its '
                          f'platformio.ini and re-run scripts/generate_platform_h.py. Supported here: '
                          f'{supported}"\n')
            output.append("#endif\n\n")

        output.append(f"#endif // {guard}\n")
        return ''.join(output)

    def write_output(self, output_file: str, family: Optional[str] = None,
                     guard: str = "PLATFORM_H", skip_if_no_envs: bool = False,
                     marker: Optional[str] = None):
        """Write the converted content to output file"""
        content = self.convert(family, guard, output_file, marker)
        if skip_if_no_envs and self.env_blocks_emitted == 0:
            print(f"- {output_file} not needed (no {family} environments)")
            return
        # Write only if the content actually changed. Rewriting an identical file still bumps its
        # mtime, and since _settings.h includes platform.h, EVERY library source depends on it -
        # so an unconditional rewrite made arduino-cli rebuild the whole library on every run,
        # which is what stopped the cached build directory from helping much.
        if Path(output_file).exists() and Path(output_file).read_text() == content:
            print(f"= {output_file} unchanged")
            return
        Path(output_file).parent.mkdir(parents=True, exist_ok=True)
        with open(output_file, 'w') as f:
            f.write(content)
        print(f"✓ Converted to {output_file}")

ESP8266_DIR = "esp8266"

def esp8266_readme(sketch: str) -> str:
    """The note that ships beside the generated ESP8266 file, explaining the manual step."""
    return f"""# ESP8266 in the Arduino IDE - one manual step

`{sketch}.ino.globals.h` in this folder holds the settings (pins, addresses, debug flags) that
PlatformIO takes from `platformio.ini`, for the ESP8266 boards this example supports.

**Using PlatformIO, or an ESP32 board? Ignore this folder entirely** - `platformio.ini` and
`platform.h` already cover you.

**Building for an ESP8266 in the Arduino IDE?** Move the file up one level, so it sits next to
`{sketch}.ino`:

    Sketch > Show Sketch Folder, then drag {ESP8266_DIR}/{sketch}.ino.globals.h into the folder above.

Then compile. Skip the step and the build stops with a message pointing back here - deliberately,
because the alternative was a build that succeeded on the library's built-in defaults rather than
this example's settings, which for most boards means the wrong pins. The check is in
`system/frugal.cpp`; the file carries a `FRUGAL_IOT_GLOBALS_FOUND` define that satisfies it.

## Why it is not just left there in the first place

The ESP8266 core only picks this file up under the exact name `<sketch>.ino.globals.h` - that is
`{{build.project_name}}.globals.h` in the core's `platform.txt`, and `build.project_name` includes
the `.ino`. It is also the only mechanism there is on ESP8266: unlike ESP32, that core does not put
the sketch directory on the include path, so `platform.h` cannot be reached from library sources -
nor, as it turns out, from the sketch itself.

But arduino-cli - and so the Arduino IDE, which embeds it - refuses to recognise a folder as a
sketch at all if it contains any file named `<sketch>.ino*` besides the sketch itself. The example
then disappears from **File > Examples** completely. Verified on arduino-cli 1.5.1: a folder is
listed with `foo.globals.h` or `bar.ino.globals.h` in it, and not listed with `foo.ino.globals.h`.

So the required name and a listed example are mutually exclusive, and the file is parked here.
"""

def main():
    """Emit the files an Arduino IDE build can pick up.

    platform.h                     - ESP32 boards. _settings.h #includes it; that only works on
                                     ESP32 because its core puts the sketch dir on the include path
                                     ("-I{build.source.path}" in compiler.cpreprocessor.flags).
    esp8266/<sketch>.ino.globals.h - ESP8266 boards. The ESP8266 core copies this into the build and
                                     force-includes it (-include) into every translation unit,
                                     library sources included, so it does not need the include path.
                                     ESP32 has no equivalent (its build_opt.h is a flat compiler
                                     response file and cannot hold #ifdef), which is why both exist.

    The ESP8266 one goes in a subfolder, and an IDE user has to move it up next to the .ino, because
    the core insists on that exact filename and arduino-cli drops any folder containing a
    <sketch>.ino* file from File > Examples. See the generated esp8266/README.md for the detail.

    None of these are hand-edited - re-run this (or scripts/prerelease.bash) after changing
    platformio.ini.
    """
    input_file = sys.argv[1] if len(sys.argv) > 1 else "platformio.ini"

    if not Path(input_file).exists():
        print(f"Error: {input_file} not found")
        sys.exit(1)

    # Explicit output overrides everything and emits just that one file, as before
    if len(sys.argv) > 2:
        PlatformIOConverter(input_file).write_output(sys.argv[2])
        return

    PlatformIOConverter(input_file).write_output(
        "platform.h", family="espressif32", guard="PLATFORM_H")

    # The ESP8266 core requires the name to match the sketch
    sketches = sorted(Path(".").glob("*.ino"))
    if sketches:
        sketch = sketches[0].stem
        # A copy left in the sketch root by an older version of this script would hide the whole
        # example from the IDE's File > Examples, so clear it out rather than leave it shadowing
        # the one written below.
        stale = Path(f"{sketch}.ino.globals.h")
        if stale.exists():
            stale.unlink()
            print(f"✗ Removed {stale} (hid this example from Arduino's File > Examples)")
        out = f"{ESP8266_DIR}/{sketch}.ino.globals.h"
        PlatformIOConverter(input_file).write_output(
            out, family="espressif8266",
            guard=f"{sketch.upper()}_INO_GLOBALS_H", skip_if_no_envs=True,
            # system/frugal.cpp refuses to build an ESP8266 Arduino IDE sketch without this,
            # because the alternative is a green build on library defaults that nothing can detect.
            marker="FRUGAL_IOT_GLOBALS_FOUND")
        readme = Path(ESP8266_DIR) / "README.md"
        if Path(out).exists():
            content = esp8266_readme(sketch)
            if not readme.exists() or readme.read_text() != content:
                readme.write_text(content)
                print(f"✓ Wrote {readme}")
    else:
        print("- no .ino found, skipping <sketch>.ino.globals.h")

if __name__ == "__main__":
    main()
