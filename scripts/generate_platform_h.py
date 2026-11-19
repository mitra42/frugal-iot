#!/usr/bin/env python3
"""
Convert platformio.ini to platform.h (and the ESP8266 X_<sketch>.ino.globals.h shim) for the Arduino IDE.

What comes out is only what PlatformIO would pass as -D: each [env:] block's build_flags, with
every ${common.x} / ${env.x} / ${this.x} reference followed the way PlatformIO follows it and
`extends =` honoured, turned into #define lines - live ones as they are, commented-out ones as
"// #define" so a user can switch them on. Nothing else in the .ini (lib_deps, platform, board,
monitor_speed, the prose) reaches the output: a user of the Arduino IDE cannot act on it and it was
most of a 400-line file. Flags every env shares are written once, at the top, so a board's own
block holds only what makes it different.
"""

import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple, Optional

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
            "heltec_wifi_lora_32_V4": "ARDUINO_heltec_wifi_lora_32_V4", # Arduino: uppercase HELTEC
            "esp32-c3-devkitm-1": "ARDUINO_ESP32C3_DEV",
            "seeed_xiao_esp32c3": "ARDUINO_XIAO_ESP32C3",   # Same macro in PlatformIO and Arduino
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
            # Same macro in PlatformIO and Arduino, where it is "ITEAD Sonoff" with Model = Basic
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
            "heltec_wifi_lora_32_V4": "Heltec WiFi LoRa 32(V4)",
            "esp32-c3-devkitm-1": "ESP32C3 Dev Module",
            "seeed_xiao_esp32c3": "XIAO_ESP32C3",
            "esp32dev": "ESP32 Dev Module",
            "nologo_esp32c3_super_mini": "Nologo ESP32C3 Super Mini",
            "sonoff_basic": "ITEAD Sonoff, with Model = ITEAD Sonoff Basic",
        }

    # ---------------------------------------------------------------- reading platformio.ini

    def read_file(self):
        """Read and parse platformio.ini into self.sections: section -> key -> value lines.

        A value is the text after the = (or :) on the key's own line followed by every indented
        line under it, kept RAW - including indented ";" comment lines, which is where the
        commented-out -D flags live. A ";" line at the left margin is a comment and is dropped.
        """
        with open(self.input_file, 'r') as f:
            self.lines = f.readlines()
        self.sections: Dict[str, Dict[str, List[str]]] = {}
        self.section_order: List[str] = []
        # "[env:r4] ; PIO ONLY" - an env the Arduino IDE cannot build (it needs PlatformIO's
        # board_build.variants_dir or a board JSON of its own), so platform.h and the README leave it out
        self.pio_only_sections = set()
        section = None
        key = None
        for line in self.lines:
            stripped = line.rstrip('\n')
            bare = stripped.strip()
            if not bare:
                continue
            if bare.startswith('[') and not stripped[0].isspace():
                section = self.strip_trailing_comment(bare).strip()[1:-1].strip()
                if PIO_ONLY.search(bare):
                    self.pio_only_sections.add(section)
                if section not in self.sections:
                    self.sections[section] = {}
                    self.section_order.append(section)
                key = None
            elif stripped[0].isspace():
                if section is not None and key is not None:
                    self.sections[section][key].append(bare)
            elif bare.startswith(';') or bare.startswith('#'):
                continue
            else:
                m = re.match(r'([A-Za-z_][\w.\-]*)\s*[=:]\s*(.*)$', bare)
                if m and section is not None:
                    key = m.group(1)
                    self.sections[section][key] = [m.group(2)] if m.group(2).strip() else []

    def strip_trailing_comment(self, text: str) -> str:
        """Return text up to (not including) the first ';' comment marker outside quotes"""
        return self.split_comment(text)[0]

    def split_comment(self, text: str) -> Tuple[str, str]:
        """(text, comment) at the first ';' outside quotes - comment without the ';', stripped."""
        quote = None
        for i, char in enumerate(text):
            if quote:
                if char == quote:
                    quote = None
            elif char in ('"', "'"):
                quote = char
            elif char == ';':
                return text[:i], text[i + 1:].strip()
        return text, ""

    def env_lookup(self, env_name: str, key: str, seen=()) -> Optional[List[str]]:
        """An [env:x] key as PlatformIO resolves it: the env's own, else the env it extends, else
        the [env] base section. None when nowhere."""
        section = self.sections.get(f"env:{env_name}", {})
        if key in section:
            return section[key]
        ext = section.get('extends')
        if ext and env_name not in seen:
            for parent in self.strip_trailing_comment(" ".join(ext)).replace(",", " ").split():
                if parent.startswith('env:'):
                    found = self.env_lookup(parent[4:], key, seen + (env_name,))
                    if found is not None:
                        return found
        return self.sections.get('env', {}).get(key)

    def env_value(self, env_name: str, key: str) -> str:
        """A single-line env setting (board, platform) with its comment removed."""
        lines = self.env_lookup(env_name, key)
        return self.strip_trailing_comment(" ".join(lines)).strip() if lines else ""

    # ---------------------------------------------------------------- the flags

    def flags_of(self, section: str, key: str, env_name: Optional[str], seen=()) -> List["Flag"]:
        """Every -D in a value, live or commented out, with ${...} references followed.

        A commented line counts only when the comment IS a flag - starts with -D, possibly
        quoted - so prose that happens to mention -D is not mistaken for one. A live line that
        is not a -D (-fno-exceptions, -Wl,...) has no #define form and is dropped.
        """
        if section.startswith('env:'):
            lines = self.env_lookup(section[4:], key)
        else:
            lines = self.sections.get(section, {}).get(key)
        out: List[Flag] = []
        for raw in lines or []:
            text = raw.strip()
            commented = text.startswith(';')
            if commented:
                text = text[1:].strip()
            text, comment = self.split_comment(text)
            text = text.strip()
            ref = re.fullmatch(r'\$\{([\w.\-]+)\.([\w.\-]+)\}', text)
            if ref:
                if not commented:
                    target = {'env': section, 'this': section}.get(ref.group(1), ref.group(1))
                    if (target, ref.group(2)) not in seen and target != 'sysenv':
                        out.extend(self.flags_of(target, ref.group(2), env_name,
                                                 seen + ((target, ref.group(2)),)))
            else:
                if len(text) >= 2 and text[0] in ('"', "'") and text[-1] == text[0]:
                    text = text[1:-1].strip()
                if text.startswith('-D'):
                    out.extend(Flag.parse(text, comment, commented))
        return out

    def env_flags(self, env_name: str) -> List["Flag"]:
        """The env's build_flags as #defines, first occurrence of each kept."""
        flags = []
        keys = set()
        for flag in self.flags_of(f"env:{env_name}", 'build_flags', env_name):
            if flag.key not in keys:
                keys.add(flag.key)
                flags.append(flag)
        return flags

    # ---------------------------------------------------------------- boards and envs

    def env_alias(self, env_name: Optional[str], table: dict) -> Optional[str]:
        """The env to look up in an env-keyed table: itself, or the nearest env it extends that
        the table names. [env:c3_pico_no_battery] extends [env:c3_pico], whose Arduino board is
        not its PlatformIO one, and inherits that along with everything else."""
        seen = ()
        while env_name and env_name not in table and env_name not in seen:
            seen += (env_name,)
            ext = getattr(self, 'sections', {}).get(f"env:{env_name}", {}).get('extends')
            parents = [p[4:] for p in self.strip_trailing_comment(" ".join(ext)).replace(",", " ").split()
                       if p.startswith('env:')] if ext else []
            env_name = parents[0] if parents else None
        return env_name

    def get_board_define(self, board_name: str, env_name: Optional[str] = None) -> str:
        """Determine the #ifdef value for an env, preferring an env-specific override"""
        env_name = self.env_alias(env_name, self.env_defines)
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
        env_name = self.env_alias(env_name, self.env_names)
        if env_name and env_name in self.env_names:
            return self.env_names[env_name]
        if board_name in self.board_names:
            return self.board_names[board_name]
        return board_name

    def get_env_selector(self, env_name: str) -> str:
        """The macro that selects this env's settings by hand.

        Several envs can share one board, and the Arduino IDE has no concept of an environment,
        so the only way to ask for a non-default one is to name it. The name is the env name
        uppercased. It has to reach every translation unit, so it cannot be a #define in the
        sketch: on ESP32 it goes in build_opt.h beside the .ino (as -DNAME), on ESP8266 at the
        top of <sketch>.ino.globals.h.
        """
        return re.sub(r'[^A-Za-z0-9_]', '_', env_name).upper()

    def process_env_sections(self) -> List[Tuple[str, Dict[str, List[str]]]]:
        """Every [env:xxx] section in file order, and the two per-env facts the header needs:
        self.env_platforms (which chip) and self.env_arduino_default (custom_arduino_default)."""
        if not hasattr(self, 'sections'):
            self.read_file()
        sections = []
        self.env_platforms = {}  # env name -> 'espressif8266' | 'espressif32'
        self.env_arduino_default = set()  # envs marked custom_arduino_default
        self.pio_only_envs = []  # envs marked "; PIO ONLY" - left out of everything Arduino
        for name in self.section_order:
            if name.startswith('env:') and name in self.pio_only_sections:
                self.pio_only_envs.append(name[4:])
            elif name.startswith('env:'):
                env_name = name[4:]
                sections.append((env_name, self.sections[name]))
                platform = self.env_value(env_name, 'platform')
                ref = re.fullmatch(r'\$\{([\w.\-]+)\.([\w.\-]+)\}', platform)
                if ref:
                    platform = self.strip_trailing_comment(
                        " ".join(self.sections.get(ref.group(1), {}).get(ref.group(2), [])))
                self.env_platforms[env_name] = (
                    'espressif8266' if 'espressif8266' in platform else 'espressif32')
                # PlatformIO ignores options prefixed custom_ silently; a plain "arduino_default"
                # would print "Warning! Ignore unknown configuration option" on every build.
                if re.match(r'(true|yes|1|on)\b', self.env_value(env_name, 'custom_arduino_default'), re.I):
                    self.env_arduino_default.add(env_name)
        return sections

    def group_envs(self, env_sections, family: Optional[str] = None):
        """Group [env:] sections by the board macro they will be guarded with.

        Returns [(board_define, active, others)], where each member is
        (env_name, flags, board_name) and `active` is the env that gets the board to itself -
        the one marked custom_arduino_default, or the first in the file.

        Both platform.h and the example README are built from this, so that the README cannot
        end up naming a different env as the default than the header actually compiles.
        """
        groups = []
        index_of = {}
        for env_name, _section in env_sections:
            if family and self.env_platforms.get(env_name, 'espressif32') != family:
                continue
            board_name = self.env_value(env_name, 'board')
            if not board_name:
                continue
            board_define = self.get_board_define(board_name, env_name)
            if board_define not in index_of:
                index_of[board_define] = len(groups)
                groups.append((board_define, []))
            groups[index_of[board_define]][1].append((env_name, self.env_flags(env_name), board_name))

        out = []
        for board_define, members in groups:
            marked = [m for m in members if m[0] in self.env_arduino_default]
            if len(marked) > 1:
                names = ", ".join(m[0] for m in marked)
                print(f"! {board_define}: several envs set custom_arduino_default ({names}) - using the first")
            active = marked[0] if marked else members[0]
            others = [m for m in members if m[0] != active[0]]
            out.append((board_define, active, others))
        return out

    def project_meta(self) -> Tuple[str, str]:
        """The [platformio] name/description, which every example's ini carries.

        Written with a colon rather than an = in these files; PlatformIO accepts both.
        """
        if not hasattr(self, 'sections'):
            self.read_file()
        meta = self.sections.get('platformio', {})
        name = self.strip_trailing_comment(" ".join(meta.get('name', []))).strip()
        description = self.strip_trailing_comment(" ".join(meta.get('description', []))).strip()
        return name, description

    # ---------------------------------------------------------------- writing the header

    def convert(self, family: Optional[str] = None, guard: str = "PLATFORM_H",
                output_name: str = "platform.h", marker: Optional[str] = None,
                secrets: bool = False, sketch: str = "<sketch>") -> str:
        """Main conversion logic.

        family: 'espressif8266' or 'espressif32' to emit only that chip's [env:] blocks, or
                None for all of them. platform.h is read only by the Arduino IDE on ESP32 and
                <sketch>.ino.globals.h only on ESP8266, so each carries just its own boards -
                the #ifdef guards already prevent cross-firing, this is for readability.
        marker: a macro to #define unconditionally, so the library can tell this file reached the
                build at all - as opposed to the settings being absent.
        secrets: also pull in platform-secrets-local.h. Done here, not only in _settings.h, because
                on ESP8266 platform.h arrives through the globals shim from {build}/sketch/, and a
                quoted include searches the including file's own directory first - so this is the
                one place that finds the file on both chips.
        """
        self.read_file()
        env_sections = self.process_env_sections()
        groups = self.group_envs(env_sections, family)
        members = [m for _bd, active, others in groups for m in [active] + others]

        self.env_blocks_emitted = 0
        self.guards_used = []
        self.names_used = []
        self.selectors_used = {}   # selector macro -> env that owns it, to catch collisions
        out = []

        # Header. Short: whoever opens this has a board to configure, not a file to read.
        out.append(f"// {output_name} - this example's settings from platformio.ini, for the Arduino IDE.\n")
        out.append("// Generated by scripts/generate_platform_h.py: change platformio.ini and re-run it, do not edit here.\n")
        out.append("// PlatformIO reads platformio.ini itself and ignores this file.\n")
        if secrets and any(self.env_platforms.get(m[0]) == "espressif8266" for m in members):
            out.append(f"// ESP8266 reads it through {sketch}.ino.globals.h - see README.md.\n")
        out.append('// A "// #define" is an option that is off - remove the "// " to turn it on.\n')
        out.append(f"#ifndef {guard}\n#define {guard}\n\n")
        if marker:
            out.append(f"#define {marker} // Tells the library this file reached the build - leave it in\n\n")

        # What every board has in common goes once, at the top; a board's block holds the rest.
        common: List[Flag] = []
        if members:
            keys_in_all = set.intersection(*(set(f.key for f in m[1]) for m in members))
            common = [f for f in members[0][1] if f.key in keys_in_all]
        common_keys = set(f.key for f in common)
        if common:
            out.append("// Every board\n" if len(members) > 1 else "")
            out.extend(render_flag(f, "") for f in common)
            out.append("\n")

        for board_define, active, others in groups:
            group = [active] + others
            # One selector macro per env, so a non-default env is one #define away. EVERY env
            # gets one, including a board with only one - otherwise the name a user had defined
            # would stop working the day a second env for that board is added or removed.
            # Where there are siblings, the default's guard also has to stand down for each of
            # them: the board macro comes from Tools > Board and stays defined whichever env is
            # wanted, so without the !defined()s both blocks would fire.
            selector_of = {m[0]: self.get_env_selector(m[0]) for m in group}
            for env_name, selector in selector_of.items():
                if selector in self.selectors_used and self.selectors_used[selector] != env_name:
                    print(f"! selector {selector} is shared by [env:{self.selectors_used[selector]}] "
                          f"and [env:{env_name}] - rename one of the envs")
                self.selectors_used[selector] = env_name
            label = self.get_board_name(active[2], active[0])

            if others:
                # Defining two of a board's selectors would fire two blocks at once. defined() is
                # 1 or 0 in an #if expression, so summing them counts how many were asked for.
                all_selectors = [selector_of[m[0]] for m in group]
                joined = ", ".join(all_selectors)
                total = " + ".join(f"defined({sel})" for sel in all_selectors)
                out.append(f"#if ({total}) > 1\n")
                out.append(f'  #error "{joined} are alternative settings for one board - define at most one"\n')
                out.append("#endif\n")
                out.append(f"// [env:{active[0]}] - Tools > Board: {label}. The default of {len(group)} for this board\n")
                stand_down = "".join(f" && !defined({selector_of[m[0]]})" for m in others)
                out.append(f"#if (defined({board_define}){stand_down}) || defined({selector_of[active[0]]})\n")
            else:
                out.append(f"// [env:{active[0]}] - Tools > Board: {label}\n")
                out.append(f"#if defined({board_define}) || defined({selector_of[active[0]]})\n")
            self.emit_env_body(out, active[1], common_keys)
            out.append(f"#endif // {board_define}\n\n")
            self.env_blocks_emitted += 1
            if board_define not in self.guards_used:
                self.guards_used.append(board_define)
                self.names_used.append(label)

            for env_name, flags, _board_name in others:
                selector = selector_of[env_name]
                # The define has to reach the library's sources too, so not the .ino. Each core
                # has one place for that, and only the one that applies is named.
                where = (f"#define {selector} at the top of {sketch}.ino.globals.h"
                         if self.env_platforms.get(env_name) == "espressif8266" else
                         f"-D{selector} in a build_opt.h beside the .ino")
                out.append(f"// [env:{env_name}] - Tools > Board: {label}, then {where} (see README.md)\n")
                out.append(f"#if defined({selector})\n")
                self.emit_env_body(out, flags, common_keys)
                out.append(f"#endif // {selector}\n\n")

        if secrets:
            # Before the catch-all, in the same order _settings.h used, which still includes the
            # file for a sketch whose platform.h is not generated, unless this marker says it is done
            out.append("// Your enrolment secret, if you made platform-secrets-local.h (see README.md). Never commit it.\n")
            out.append('#if __has_include("platform-secrets-local.h")\n')
            out.append('  #include "platform-secrets-local.h"\n')
            out.append("#endif\n")
            out.append("#define FRUGAL_IOT_SECRETS_CHECKED // Tells _settings.h not to include it again\n\n")

        if self.pio_only_envs:
            names = ", ".join(f"[env:{e}]" for e in self.pio_only_envs)
            out.append(f"// No block for {names}: marked PIO ONLY in platformio.ini\n\n")
            if not self.guards_used:
                verb = "needs" if len(self.pio_only_envs) == 1 else "need"
                out.append(f'#error "This example is PlatformIO only: {", ".join(self.pio_only_envs)} '
                           f'{verb} board files the Arduino IDE cannot use - see README.md"\n\n')

        # Catch-all: if none of the blocks above matched, the selected board has no settings in
        # this file and the sketch would otherwise compile with library defaults only - a build
        # that looks fine and is not what anyone intended. Fail loudly instead.
        if self.guards_used:
            # Board menu labels, not the ARDUINO_* macros - whoever reads this error is sitting in
            # front of Tools > Board and needs to know what to pick there. Separated by ' / ' and
            # NOT quoted: the message is one C string literal, so an inner " would end it early.
            supported = " / ".join(self.names_used)
            out.append("#ifndef FRUGAL_IOT_BOARD_CONFIGURED\n")
            out.append(f'  #error "This board has no settings in {output_name}. Under Tools > Board, '
                       f'select one of the boards this example supports, or add a section for yours to its '
                       f'platformio.ini and re-run scripts/generate_platform_h.py. Supported here: '
                       f'{supported}"\n')
            out.append("#endif\n\n")

        out.append(f"#endif // {guard}\n")
        return ''.join(out)

    def emit_env_body(self, out: List[str], flags: List["Flag"], common_keys: set):
        # Marker for the catch-all at the end of the file. Namespaced because on ESP8266 this
        # file is force-included into EVERY translation unit, the core's own sources included.
        out.append("  #define FRUGAL_IOT_BOARD_CONFIGURED\n")
        out.extend(render_flag(f, "  ") for f in flags if f.key not in common_keys)

    def write_output(self, output_file: str, family: Optional[str] = None,
                     guard: str = "PLATFORM_H", marker: Optional[str] = None,
                     secrets: bool = False, sketch: str = "<sketch>"):
        """Write the converted content to output file"""
        write_if_changed(output_file, self.convert(family, guard, output_file, marker, secrets, sketch))


def write_if_changed(output_file: str, content: str):
    """Write only if the content actually changed. Rewriting an identical file still bumps its
    mtime, and since _settings.h includes platform.h, EVERY library source depends on it - so an
    unconditional rewrite made arduino-cli rebuild the whole library on every run, which is what
    stopped the cached build directory from helping much."""
    if Path(output_file).exists() and Path(output_file).read_text() == content:
        print(f"= {output_file} unchanged")
    else:
        Path(output_file).parent.mkdir(parents=True, exist_ok=True)
        with open(output_file, 'w') as f:
            f.write(content)
        print(f"✓ Wrote {output_file}")


class Flag:
    """One -D from a build_flags value, as a #define line. `commented` is a flag the .ini has
    switched off with a leading ; - it comes out as "// #define" so a user can switch it on."""

    def __init__(self, name: str, value: Optional[str], comment: str, commented: bool):
        self.name = name
        self.value = value
        self.comment = comment
        self.commented = commented

    @property
    def key(self):
        return (self.name, self.value, self.commented)

    def render(self) -> str:
        text = f"#define {self.name}" + (f" {self.value}" if self.value else "")
        if self.commented:
            text = "// " + text
        if self.comment:
            text += " // " + self.comment
        return text

    @staticmethod
    def parse(text: str, comment: str, commented: bool) -> List["Flag"]:
        """Every -D in one line - "-D A -D B=2" is two - split at a -D outside quotes."""
        starts = []
        quote = None
        for i, char in enumerate(text):
            if quote:
                if char == quote:
                    quote = None
            elif char in ('"', "'"):
                quote = char
            elif text.startswith('-D', i) and (i == 0 or text[i - 1].isspace()):
                starts.append(i)
        flags = []
        for n, start in enumerate(starts):
            end = starts[n + 1] if n + 1 < len(starts) else len(text)
            m = re.match(r'-D\s*([A-Za-z_]\w*)(?:=(.*))?$', text[start:end].strip())
            if m:
                value = (m.group(2) or "").strip()
                flags.append(Flag(m.group(1), value or None, comment if n == len(starts) - 1 else "", commented))
        return flags

ESP8266_DIR = "esp8266"   # Where older versions of this script put the ESP8266 file - cleaned up
SHIM_PREFIX = "X_"

def esp8266_shim(sketch: str) -> str:
    """X_<sketch>.ino.globals.h - the same few lines in every example; only the name differs.

    The ESP8266 core force-includes <sketch>.ino.globals.h, found by that exact name, into every
    translation unit - the one way to reach the library's sources on that chip. It copies the file
    to {build}/core/ first, and the IDE has copied the sketch folder to {build}/sketch/, hence
    ../sketch/platform.h. A file of that exact name beside the .ino also makes arduino-cli drop
    the example from File > Examples, so it ships with an X_ that an ESP8266 user removes. Left
    as it is, the build stops at the #error in system/frugal.cpp, which says to rename it.
    """
    return f"""// ESP8266 boards in the Arduino IDE only - PlatformIO, and ESP32 boards, ignore this file.
// Before compiling for an ESP8266, rename it to {sketch}.ino.globals.h - remove the X_. If you
// saved the sketch under a new name, use that name: mytank.ino needs mytank.ino.globals.h.
// It ships with the X_ because a file of the real name hides the example from File > Examples.
//
// It only passes platform.h on, which holds the settings. The ESP8266 core reads a copy of this
// file from the build folder, where the Arduino IDE has also copied the sketch into sketch/.
// To pick a non-default configuration for your board (see README.md), #define it above the #include.
// Generated by scripts/generate_platform_h.py.
#define FRUGAL_IOT_GLOBALS_FOUND // Tells the library this file reached the build
#include "../sketch/platform.h"
"""

def needs_cdc_on_boot(flags) -> bool:
    """An env whose Serial is the chip's own USB - in the Arduino IDE that is a Tools menu setting."""
    return any(f.name == "ARDUINO_USB_CDC_ON_BOOT" and f.value == "1" and not f.commented for f in flags)


def render_flag(f: "Flag", indent: str) -> str:
    """A flag as platform.h lines. ARDUINO_USB_CDC_ON_BOOT=1 is the exception: in the Arduino IDE it is
    the Tools > USB CDC On Boot menu, compiled into the core before this file is read - a #define
    here would change nothing - so check the menu was set instead."""
    if needs_cdc_on_boot([f]):
        return (f"{indent}#if !ARDUINO_USB_CDC_ON_BOOT // {f.comment or 'Serial = USB'}\n"
                f'{indent}  #error "Set Tools > USB CDC On Boot > Enabled - this board has no USB-serial chip, '
                f'so without it the Serial Monitor stays empty"\n'
                f"{indent}#endif\n")
    return indent + f.render() + "\n"


PIO_ONLY = re.compile(r";.*\bPIO ONLY\b", re.I)  # marks an [env:] the Arduino IDE cannot build

README_BEGIN = "<!-- BEGIN generated by scripts/generate_platform_h.py - do not edit inside -->"
README_END = "<!-- END generated -->"

LORAMESHER_ZIP = "https://github.com/loramesher/LoRaMesher/archive/refs/heads/main.zip"


def loramesher_board_defines() -> set:
    """The Arduino board macros that turn on SYSTEM_LORAMESHER_WANT, read from _settings.h.

    Read rather than copied, so a board added to that list gets the README's LoRaMesher step
    without anyone remembering this file. One level of indirection is followed, because the list
    names ARDUINO_TTGO_LoRa32, which _settings.h itself derives from the three real TTGO macros.
    """
    text = (Path(__file__).resolve().parent.parent / "src" / "_settings.h").read_text()
    derived = {}  # macro -> the macros on the #if that defines it
    wanted = set()
    for cond, name in re.findall(r"^#if\s+(.*?)\s*\n\s*#define\s+(\w+)", text, re.M):
        macros = set(re.findall(r"\b(ARDUINO_\w+)", cond))
        derived[name] = macros
        if name == "SYSTEM_LORAMESHER_WANT":
            wanted = macros
    return wanted.union(*(derived.get(w, set()) for w in wanted))


def example_readme_block(conv: "PlatformIOConverter", sketch: str, has_shim: bool,
                         has_min_spiffs: bool) -> str:
    """The generated part of an example's README.md - what an Arduino IDE user needs to compile it.

    Built from the same group_envs() the header is built from, so the board it calls the default
    is the board platform.h actually gives the settings to.
    """
    groups = conv.group_envs(conv.process_env_sections())
    multi = [g for g in groups if g[2]]
    default_env = groups[0][1][0] if groups else ""
    families = {conv.env_platforms.get(m[0], "espressif32")
                for _bd, a, o in groups for m in [a] + o}

    out = [README_BEGIN, ""]
    wiki = "https://github.com/mitra42/frugal-iot/wiki/Getting-Started"
    example_env = default_env or (conv.pio_only_envs[0] if conv.pio_only_envs else "<env>")

    out += ["## PlatformIO", "",
            f"1. Copy this folder (`examples/{sketch}` in [frugal-iot](https://github.com/mitra42/frugal-iot))"
            " to wherever you keep projects.",
            "2. In VS Code, **PlatformIO Home > Open Project**, and choose that folder.",
            "3. In the status bar, pick the environment for your board"
            + (" from the table below" if groups else "") + ", and plug the board in.",
            "4. **Upload** (→), then **Serial Monitor** (the plug icon) to watch it start.", "",
            f"Or from a terminal: `pio run -e {example_env} -t upload -t monitor`", ""]
    if groups and conv.pio_only_envs:
        out += [f"PlatformIO only: {', '.join(f'`{e}`' for e in conv.pio_only_envs)}.", ""]

    next_steps = ["## Next steps", "",
                  f"* Connect it to WiFi and see it on the dashboard: [Getting Started]({wiki}).",
                  "* Debug messages (PlatformIO): copy `templates/platformio-local.ini.template` from the"
                  " library to `platformio-local.ini` beside `platformio.ini`, and uncomment the flags you want.",
                  "* So the node joins your organization without an administrator approving it, add its"
                  " enrolment secret: `templates/platformio-secrets-local.ini.template` (PlatformIO) or"
                  " `platform-secrets-local.h.template` (Arduino IDE) - each says how.",
                  f"* Won't build? Run `python3 frugal_iot_doctor.py` (from the library's `scripts/`) in this folder, or see [Getting Started]({wiki}#if-it-does-not-work).", ""]

    if not groups:
        # Every env is PIO ONLY - nothing for the Arduino IDE to build
        out += ["## Arduino IDE", "",
                f"This example is PlatformIO only - it needs board files the Arduino IDE cannot use.", ""]
        out += next_steps + [README_END]
        return "\n".join(out) + "\n"

    # Named per family, so an ESP32-only example does not send people off to add a board URL
    cores = []
    if "espressif32" in families:
        cores.append("**esp32** by Espressif Systems")
    if "espressif8266" in families:
        cores.append("**esp8266** by ESP8266 Community (first add "
                     "`https://arduino.esp8266.com/stable/package_esp8266com_index.json` under "
                     "**File > Preferences > Additional boards manager URLs**)")
    steps = [
        "**Tools > Manage Libraries**, install **Frugal-IoT**, and choose **Install All** when it asks about dependencies.",
        "**Tools > Board > Boards Manager**, and install "
        + (cores[0] if len(cores) == 1 else "the one for your chip: " + ", or ".join(cores)) + ".",
        f"**File > Examples > Frugal-IoT > {sketch}**, then **File > Save As** to keep your own copy.",
    ]
    if has_shim:
        steps.append(f"ESP8266 only: **Sketch > Show Sketch Folder**, and rename `{SHIM_PREFIX}{sketch}.ino.globals.h`"
                     f" to match your sketch, e.g. `{sketch}.ino.globals.h`.")
    steps += ["**Tools > Board**: the row for your hardware in the table below. **Tools > Port**: your board.",
              "**Upload** (→), then **Tools > Serial Monitor** at 115200 baud to watch it start."]
    out += ["## Arduino IDE", ""] + [f"{i}. {t}" for i, t in enumerate(steps, 1)] + [""]

    out += ["| Tools > Board | configuration | also define |",
            "| --- | --- | --- |"]
    for _board_define, active, others in groups:
        for member in [active] + others:
            env_name, _lines, board_name = member
            label = conv.get_board_name(board_name, env_name)
            selector = "" if member is active else f"`{conv.get_env_selector(env_name)}`"
            out.append(f"| {label} | `{env_name}` | {selector} |")
    out.append("")
    cdc = [m[0] for _bd, a, o in groups for m in [a] + o if needs_cdc_on_boot(m[1])]
    if cdc:
        names = ", ".join(f"`{e}`" for e in cdc)
        out += [f"For {names}, also set **Tools > USB CDC On Boot > Enabled**, or the Serial Monitor stays empty.", ""]

    out += next_steps

    # Arduino IDE problems - after the success path, so they do not get in its way
    trouble = ["*Frugal-IoT.h: No such file or directory* - the library is not installed (step 1);"
               " opening the `.ino` from a download does not install it."]
    if has_min_spiffs:
        trouble.append("*text section exceeds available space* - **Tools > Partition Scheme > Minimal SPIFFS**"
                       " if offered. Otherwise rename `min_spiffs.csv` to `partitions.csv` in the sketch folder and"
                       " choose **No OTA (2MB APP/2MB SPIFFS)**; OTA still works, as `partitions.csv` sets the real layout.")

    lora_macros = loramesher_board_defines()
    # Up to the comma, as the TTGO label goes on to name a Board Revision; the table has that
    lora_boards = sorted({conv.get_board_name(m[2], m[0]).split(",")[0]
                          for _bd, a, o in groups for m in [a] + o
                          if conv.get_board_define(m[2], m[0]) in lora_macros})
    if lora_boards:
        # LoRaMesher is not in the Library Manager, so depends= cannot pull it in - and Add .ZIP
        # Library installs no dependencies, hence RadioLib named separately
        named = " and ".join(filter(None, [", ".join(lora_boards[:-1]), lora_boards[-1]]))
        trouble.append(f"*loramesher.hpp: No such file* ({named}) - download [LoRaMesher as a zip]({LORAMESHER_ZIP}),"
                       " **Sketch > Include Library > Add .ZIP Library**, then install **RadioLib** from the Library Manager.")

    if multi:
        # The example macro has to come from a board of the family the bullet is about - quoting an
        # ESP32 board's selector in the ESP8266 instruction is exactly the sort of thing someone copies verbatim.
        def sample_selector(want_family):
            for _bd, _active, siblings in multi:
                env_name = siblings[0][0]
                if conv.env_platforms.get(env_name, "espressif32") == want_family:
                    return conv.get_env_selector(env_name)
            return None

        esp32_sel = sample_selector("espressif32")
        esp8266_sel = sample_selector("espressif8266")
        how = []
        if esp32_sel:
            how.append(f"on ESP32 put the line `-D{esp32_sel}` in a file `build_opt.h` beside `{sketch}.ino`"
                       " (compiler flags, not a `#define`)")
        if esp8266_sel:
            how.append(f"on ESP8266 add `#define {esp8266_sel}` above the `#include` in `{sketch}.ino.globals.h`")
        trouble.append("A board listed twice - you get the row with an empty last column unless you define that macro: "
                       + "; ".join(how) + ".")

    if sketch != "all":  # examples/all is where the other boards' [env:] sections live
        trouble.append("Board not in the table - the build stops and lists the ones that are. `examples/all/platformio.ini`"
                       " has more boards: copy one into this `platformio.ini`, add this example's pins, and re-run"
                       " `scripts/generate_platform_h.py`.")

    out += ["## Arduino IDE problems", ""] + [f"* {t}" for t in trouble] + [""]
    out += [README_END]
    return "\n".join(out) + "\n"


def write_example_readme(path: str, title: str, description: str, block: str):
    """Write the block into README.md, leaving anything a human wrote around it alone.

    Replaces an existing marked block in place, appends one to a README that has none (which is
    how the hand-written loramesher README keeps its content), and otherwise starts a new file
    with a title and description above the markers - outside them, so they stay editable.
    """
    target = Path(path)
    if target.exists():
        old = target.read_text()
        if README_BEGIN in old and README_END in old:
            head, rest = old.split(README_BEGIN, 1)
            _stale, tail = rest.split(README_END, 1)
            content = head + block.rstrip("\n") + tail
        else:
            content = old.rstrip("\n") + "\n\n" + block
    else:
        content = f"# {title}\n\n{description}\n\n{block}"
    if target.exists() and target.read_text() == content:
        print(f"= {path} unchanged")
        return
    target.write_text(content)
    print(f"✓ Wrote {path}")


def main():
    """Emit the files an Arduino IDE build can pick up.

    platform.h                 - every board's settings. On ESP32 _settings.h #includes it, which
                                 works because that core puts the sketch dir on the include path
                                 ("-I{build.source.path}" in compiler.cpreprocessor.flags).
    X_<sketch>.ino.globals.h   - ESP8266 only: a few lines that pass platform.h on. The ESP8266
                                 core force-includes <sketch>.ino.globals.h into every translation
                                 unit, library sources included, but does not put the sketch dir on
                                 the include path. Ships with the X_ because arduino-cli drops any
                                 folder containing a <sketch>.ino* file from File > Examples; an
                                 ESP8266 IDE user removes it. See esp8266_shim().

    None of these are hand-edited - re-run this (or scripts/prerelease.bash) after changing
    platformio.ini.

    Two options, for a project that is not one of the library's examples - frugal-iot-irrigation
    is the case they were added for:
      --esp32      ESP32 boards only in platform.h, and no ESP8266 shim.
      --no-readme  leave README.md alone. An example's README is mostly this generated block; an
                   application's is written by hand and already says how to build it.
    """
    import argparse
    parser = argparse.ArgumentParser(description="Convert platformio.ini to platform.h")
    parser.add_argument("input", nargs="?", default="platformio.ini")
    parser.add_argument("output", nargs="?",
                        help="write just this one file, with every env in it, and nothing else")
    parser.add_argument("--esp32", action="store_true",
                        help="ESP32 only: platform.h without ESP8266 boards, and no ESP8266 shim")
    parser.add_argument("--no-readme", action="store_true",
                        help="do not write the generated block into README.md")
    args = parser.parse_args()
    input_file = args.input

    if not Path(input_file).exists():
        print(f"Error: {input_file} not found")
        sys.exit(1)

    # Explicit output overrides everything and emits just that one file, as before
    if args.output:
        PlatformIOConverter(input_file).write_output(args.output)
        return

    sketches = sorted(Path(".").glob("*.ino"))
    sketch = sketches[0].stem if sketches else "<sketch>"
    conv = PlatformIOConverter(input_file)
    conv.write_output("platform.h", family="espressif32" if args.esp32 else None,
                      guard="PLATFORM_H", secrets=True, sketch=sketch)
    has_8266 = (not args.esp32) and "espressif8266" in conv.env_platforms.values()

    # What older versions of this script wrote: esp8266/ with the settings in it, which a user had
    # to move up - and, before that, the same file beside the .ino, hiding the example from
    # File > Examples. Only this script's own files are removed, and the folder only if emptied.
    if sketches:
        for stale in [Path(f"{sketch}.ino.globals.h"), Path(ESP8266_DIR) / f"{sketch}.ino.globals.h",
                      Path(ESP8266_DIR) / "README.md"]:
            if stale.exists():
                stale.unlink()
                print(f"✗ Removed {stale}")
        if Path(ESP8266_DIR).is_dir() and not any(Path(ESP8266_DIR).iterdir()):
            Path(ESP8266_DIR).rmdir()

    shim = Path(f"{SHIM_PREFIX}{sketch}.ino.globals.h")
    if not sketches:
        print("- no .ino found, skipping the ESP8266 shim")
    elif has_8266:
        write_if_changed(str(shim), esp8266_shim(sketch))
    elif shim.exists():
        shim.unlink()
        print(f"✗ Removed {shim} (no espressif8266 env any more)")

    # README.md for whoever opens the example - which board to select, and how to ask for a
    # configuration the board menu cannot express.
    if sketches and not args.no_readme:
        conv = PlatformIOConverter(input_file)
        conv.read_file()
        name, description = conv.project_meta()
        block = example_readme_block(
            conv, sketch, has_shim=shim.exists(),
            has_min_spiffs=Path("min_spiffs.csv").exists())
        write_example_readme("README.md", name or sketch, description, block)

if __name__ == "__main__":
    main()
