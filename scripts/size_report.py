#!/usr/bin/env python3
"""
Where the flash went.

Reads the linker map and the ELF PlatformIO just built and says, per library and per source file,
how many bytes each contributed - and, importantly, which source files contributed *nothing*
because --gc-sections threw them away.

That last part is the point. On ESP32 the pioarduino platform forces lib_archive = False (see its
builder/main.py: "This makes weak defs in framework and libs possible"), so every .cpp in every
library is handed to the linker as a loose object file rather than as an archive member pulled in
on demand. Nothing is lazily skipped. Unused code is kept out purely by -ffunction-sections /
-fdata-sections plus -Wl,--gc-sections, which is more fragile than archive laziness: anything
anchored by a file-scope constructor keeps its whole object alive. This script is how you check it
is actually working - a sensor you never instantiated should appear here with 0 bytes.

Needs -Wl,-Map=$BUILD_DIR/firmware.map in build_flags (${common.build_flags_map}).

Two things it deliberately does NOT pretend to know:

  * Merged string literals. ld pools identical SHF_MERGE|SHF_STRINGS sections across the entire
    link, and the map credits the whole pool to whichever input section it placed first - which
    blames one arbitrary object for everybody's strings. On c3_pico it put 137,557 bytes on
    MQTTClient.cpp.o for a section that is 1 byte in the object file. So pooled strings are
    counted as a total and attributed to nobody; --strings gives a per-library upper bound
    measured from the object files instead.
  * Those same string totals for archive members, since it does not unpack .a files.

Usage:
    lib/Frugal-IoT/scripts/size_report.py                      # every env that has been built
    lib/Frugal-IoT/scripts/size_report.py c3_pico
    lib/Frugal-IoT/scripts/size_report.py c3_pico --detail Frugal-IoT
    lib/Frugal-IoT/scripts/size_report.py c3_pico --strings    # per-library string-literal upper bound
    lib/Frugal-IoT/scripts/size_report.py c3_pico --top 40
    lib/Frugal-IoT/scripts/size_report.py --json c3_pico       # for diffing two builds
"""

import argparse
import json
import re
import struct
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

def find_project(start: Path = None) -> Path:
    """The PlatformIO project we are being run against.

    Walked up from the working directory rather than derived from this file's location, because
    these scripts live in the library (lib/Frugal-IoT/scripts/) and are meant to be run from
    whatever project has the library as a dependency - so their own path says nothing useful
    about which project's .pio to read.
    """
    here = (start or Path.cwd()).resolve()
    for d in (here, *here.parents):
        if (d / "platformio.ini").exists():
            return d
    raise SystemExit(f"no platformio.ini found in {here} or any parent - "
                     "run this from inside a PlatformIO project")


PROJECT = find_project()
BUILD_ROOT = PROJECT / ".pio" / "build"

FLASH_CODE = "flash code"
FLASH_DATA = "flash data"
RAM_CODE = "ram code (iram)"
RAM_DATA = "ram data"
ZERO = "zero-init"
OTHER = "other"
FLASH_BUCKETS = (FLASH_CODE, FLASH_DATA)


def classify(section: str) -> str:
    """Bucket an output section name. Order matters - .iram0.text is RAM code, not flash code."""
    s = section.lower()
    if "iram" in s:
        return RAM_CODE
    if "bss" in s or "noinit" in s:
        return ZERO
    if "rodata" in s or "appdesc" in s or "eh_frame" in s:
        return FLASH_DATA
    if "text" in s:
        return FLASH_CODE
    if "data" in s or "rtc" in s:
        return RAM_DATA
    return OTHER


# ---------------------------------------------------------------- ELF (pure Python, no toolchain)

SHF_ALLOC, SHF_MERGE, SHF_STRINGS = 0x2, 0x10, 0x20
SHT_NOBITS = 8


def elf_sections(path: Path):
    """[(name, size, flags, is_nobits)] for every section.

    Parsed here rather than shelled out to readelf so the script does not have to work out which
    of the several toolchains under ~/.platformio built this particular file.
    """
    data = path.read_bytes()
    if data[:4] != b"\x7fELF":
        raise ValueError(f"{path} is not an ELF file")
    if data[4] != 1:
        raise ValueError(f"{path} is not ELF32")
    end = "<" if data[5] == 1 else ">"
    e_shoff, = struct.unpack_from(end + "I", data, 0x20)
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from(end + "HHH", data, 0x2E)

    def shdr(i):
        return struct.unpack_from(end + "IIIIII", data, e_shoff + i * e_shentsize)

    strtab = shdr(e_shstrndx)[4]

    def name_at(idx):
        return data[strtab + idx: data.index(b"\0", strtab + idx)].decode("utf-8", "replace")

    return [(name_at(nm), size, flags, typ == SHT_NOBITS)
            for nm, typ, flags, _a, _o, size in (shdr(i) for i in range(e_shnum))]


def elf_alloc(path: Path):
    """{name: (size, is_nobits)} for ALLOC sections."""
    return {n: (s, nb) for n, s, f, nb in elf_sections(path) if f & SHF_ALLOC}


def elf_merge_string_bytes(path: Path) -> int:
    """Bytes of mergeable string literals in an object, BEFORE ld dedups them across the link.

    SHF_ALLOC matters as much as MERGE|STRINGS here: .debug_str is also a mergeable string
    section but is debug info that never reaches the device, and it dwarfs the real literals
    (38,163 vs ~350 bytes in sensor/aht.cpp.o), so leaving it in makes the number meaningless.
    """
    want = SHF_MERGE | SHF_STRINGS | SHF_ALLOC
    return sum(s for _n, s, f, nb in elf_sections(path) if (f & want) == want and not nb)


# ---------------------------------------------------------------- map file

# The origin is matched as ".+?" rather than "\S+" because library directory names can contain
# spaces - "Adafruit GFX Library", "Adafruit SSD1306". With \S+ those lines simply did not match
# and both libraries vanished from the report, which on t3_s3 is exactly the OLED cost being
# looked for. Two 0x fields are still what separates a section line from a symbol line (one).
SECTION_LINE = re.compile(
    r"^\s+(?P<name>\.\S+)\s+0x(?P<addr>[0-9a-f]+)\s+0x(?P<size>[0-9a-f]+)\s+(?P<origin>.+?)\s*$"
)
NAME_ONLY = re.compile(r"^\s+(?P<name>\.\S+)\s*$")          # long names wrap onto the next line
NUMBERS_ONLY = re.compile(
    r"^\s+0x(?P<addr>[0-9a-f]+)\s+0x(?P<size>[0-9a-f]+)\s+(?P<origin>.+?)\s*$"
)
OUTPUT_SECTION = re.compile(r"^(?P<name>\.\S+)")
FILL = re.compile(r"^\s+\*fill\*\s+0x(?P<addr>[0-9a-f]+)\s+0x(?P<size>[0-9a-f]+)")
ARCHIVE_MEMBER = re.compile(r"^(?P<archive>\S+\.a)\((?P<member>[^)]+)\)$")
# gcc names mergeable string sections ".rodata.<sym>.str1.4", ".rodata.str1.1" and so on
MERGED_STRINGS = re.compile(r"\.str\d+\.\d+$")


def read_entry(lines, i):
    """(name, size, origin, wrapped) for the map entry at line i, or None."""
    m = SECTION_LINE.match(lines[i])
    if m:
        return m.group("name"), int(m.group("size"), 16), m.group("origin"), False
    m2 = NAME_ONLY.match(lines[i])
    if m2 and i + 1 < len(lines):
        m3 = NUMBERS_ONLY.match(lines[i + 1])
        if m3:
            return m2.group("name"), int(m3.group("size"), 16), m3.group("origin"), True
    return None


class MapFile:
    def __init__(self, path: Path):
        self.inclusions = {}                # "lib.a(member.o)" -> (pulled in by, symbol)
        self.entries = []                   # (out_sec, in_sec, size, origin) - non-pooled only
        self.has_pooled = defaultdict(int)  # out_sec -> count of pooled-string entries seen
        self.fill = defaultdict(int)
        self.discarded = defaultdict(int)   # origin -> bytes --gc-sections threw away
        self._parse(path)

    def _parse(self, path):
        lines = path.read_text(errors="replace").splitlines()
        n = len(lines)
        i = 0

        # 1. which archive members were pulled in, and by which symbol
        if lines and lines[0].startswith("Archive member included"):
            i = 1
            pending = None
            while i < n and not lines[i].startswith("Discarded input sections"):
                line = lines[i]
                if line and not line[0].isspace():
                    pending = line.strip()
                elif pending and line.strip():
                    m = re.match(r"^\s+(?P<by>\S+)\s+\((?P<sym>.+)\)\s*$", line)
                    if m:
                        self.inclusions[pending] = (m.group("by"), m.group("sym"))
                    pending = None
                i += 1

        # 2. what --gc-sections removed
        if i < n and lines[i].startswith("Discarded input sections"):
            i += 1
            while i < n and not lines[i].startswith("Memory Configuration"):
                got = read_entry(lines, i)
                if got:
                    self.discarded[got[2]] += got[1]
                    if got[3]:
                        i += 1
                i += 1

        # 3. the memory map proper
        while i < n and not lines[i].startswith("Linker script and memory map"):
            i += 1
        i += 1
        current = None
        while i < n:
            line = lines[i]
            if line.startswith("OUTPUT("):
                break
            m = OUTPUT_SECTION.match(line)
            if m:
                current = m.group("name")
                i += 1
                continue
            if current:
                mf = FILL.match(line)
                if mf:
                    self.fill[current] += int(mf.group("size"), 16)
                    i += 1
                    continue
                got = read_entry(lines, i)
                if got:
                    name, size, origin, wrapped = got
                    if MERGED_STRINGS.search(name):
                        # The size here is the whole merged pool, not this object's share of it.
                        # Note only that pooling happened; the true total comes from the ELF.
                        self.has_pooled[current] += 1
                    else:
                        self.entries.append((current, name, size, origin))
                    if wrapped:
                        i += 1
            i += 1


# ---------------------------------------------------------------- attribution

def owner_of(origin: str, env: str):
    """(library, file) for an object path or an archive member."""
    m = ARCHIVE_MEMBER.match(origin)
    if m:
        archive, member = m.group("archive"), m.group("member")
        stem = Path(archive).stem
        if stem == "libFrameworkArduino":
            return "Arduino core", member
        if "framework-arduinoespressif32-libs" in archive or "/esp-idf/" in archive:
            return f"IDF {stem}", member
        if "toolchain-" in archive:
            return f"toolchain {stem}", member
        return stem, member

    parts = Path(origin).parts
    try:
        k = parts.index(env)
    except ValueError:
        return "unattributed", origin
    rest = parts[k + 1:]
    if not rest:
        return "unattributed", origin
    if rest[0] == "src":
        return "project src", str(Path(*rest[1:]))
    if rest[0] == "FrameworkArduino":
        return "Arduino core", str(Path(*rest[1:]))
    if rest[0].startswith("lib") and len(rest) >= 3:
        # libXXXX/<Library Name>/<path>.o - the libXXXX hash dir is noise, the name after it is not
        return rest[1], str(Path(*rest[2:]))
    return "unattributed", str(Path(*rest))


def demangle(names):
    """Best-effort c++filt; gives up quietly if no toolchain is found."""
    cands = list(Path.home().glob(".platformio/packages/toolchain-*/bin/*-c++filt"))
    if not cands or not names:
        return {}
    try:
        out = subprocess.run([str(cands[0])], input="\n".join(names),
                             capture_output=True, text=True, timeout=30)
        pretty = out.stdout.splitlines()
        if len(pretty) == len(names):
            return dict(zip(names, pretty))
    except (OSError, subprocess.SubprocessError):
        pass
    return {}


def human(n):
    return f"{n:,}"


# ---------------------------------------------------------------- analysis

def analyse(env: str, want_strings=False):
    bd = BUILD_ROOT / env
    elf, mapf, binf = bd / "firmware.elf", bd / "firmware.map", bd / "firmware.bin"
    if not elf.exists():
        raise FileNotFoundError(f"no firmware.elf in {bd} - build the env first")
    if not mapf.exists():
        raise FileNotFoundError(
            f"no firmware.map in {bd} - add ${{common.build_flags_map}} to this env's build_flags")

    mp = MapFile(mapf)
    sections = elf_alloc(elf)
    # Only sections that are ALLOC *and* occupy file bytes. The NOBITS ones - .bss, plus the
    # .flash_rodata_dummy / .dram0.dummy placeholders that reserve MMU address space - hold
    # nothing, and counting their alignment fill invented 1.1 MB of flash that does not exist.
    real = {n for n, (_s, nb) in sections.items() if not nb}

    by_owner = defaultdict(lambda: defaultdict(int))
    by_detail = defaultdict(lambda: defaultdict(lambda: defaultdict(int)))
    attributed = defaultdict(int)
    biggest = []
    for out_sec, in_sec, size, origin in mp.entries:
        if out_sec not in real:
            continue
        attributed[out_sec] += size
        if size == 0:
            continue
        bucket = classify(out_sec)
        owner, detail = owner_of(origin, env)
        by_owner[owner][bucket] += size
        by_detail[owner][detail][bucket] += size
        biggest.append((size, in_sec, owner, detail, bucket))
    biggest.sort(reverse=True)

    # The merged-string pool, derived from the ELF rather than believed from the map:
    #   pool = what the section really is - what could be attributed - alignment fill
    pool = {}
    for name in real:
        if mp.has_pooled.get(name):
            pool[name] = max(0, sections[name][0] - attributed[name] - mp.fill.get(name, 0))

    # What was compiled at all, so "compiled but contributed nothing" can be spotted. Loose
    # objects only - which on ESP32 is everything bar the Arduino core and the prebuilt IDF libs.
    compiled = defaultdict(set)
    strings = defaultdict(int)
    for obj in bd.rglob("*.o"):
        owner, detail = owner_of(str(obj.relative_to(PROJECT)), env)
        compiled[owner].add(detail)
        # Only objects that survived --gc-sections; a discarded one's literals are not in the
        # image either, and counting them turned an upper bound into a meaningless one.
        if want_strings and detail in by_detail.get(owner, {}):
            try:
                strings[owner] += elf_merge_string_bytes(obj)
            except (ValueError, OSError, struct.error, IndexError):
                pass

    return {
        "env": env,
        "bin_bytes": binf.stat().st_size if binf.exists() else None,
        "elf_real": {n: s for n, (s, nb) in sections.items() if not nb},
        "by_owner": {k: dict(v) for k, v in by_owner.items()},
        "by_detail": {k: {d: dict(b) for d, b in v.items()} for k, v in by_detail.items()},
        "biggest": biggest,
        "compiled": {k: sorted(v) for k, v in compiled.items()},
        "strings": dict(strings),
        "inclusions": mp.inclusions,
        "fill": {k: v for k, v in mp.fill.items() if k in real},
        "pool": pool,
    }


# ---------------------------------------------------------------- report

def reconcile(a, quiet=False):
    """Prove the arithmetic before showing any of it.

    The map is a text file ld writes for humans, not an API. Merged string pools, relaxation and
    alignment fill all make naive summing wrong, and a wrong number here would send a whole
    investigation down the garden path. So check it against the ELF's own section headers and say
    so, loudly, when it does not add up.
    """
    parsed, poolb, fillb, truth = (defaultdict(int) for _ in range(4))
    for buckets in a["by_owner"].values():
        for b, v in buckets.items():
            parsed[b] += v
    for name, v in a["pool"].items():
        poolb[classify(name)] += v
    for name, v in a["fill"].items():
        fillb[classify(name)] += v
    for name, size in a["elf_real"].items():
        truth[classify(name)] += size

    worst = max(abs(parsed[b] + poolb[b] + fillb[b] - truth[b])
                for b in (FLASH_CODE, FLASH_DATA, RAM_CODE, RAM_DATA))
    if quiet:
        return worst

    print("\nReconciliation - parsed map vs ELF section headers:")
    print(f"  {'':<16}{'attributed':>12}{'+pooled str':>13}{'+fill':>8}{'= ELF':>12}{'delta':>8}")
    for b in (FLASH_CODE, FLASH_DATA, RAM_CODE, RAM_DATA):
        delta = parsed[b] + poolb[b] + fillb[b] - truth[b]
        print(f"  {b:<16}{human(parsed[b]):>12}{human(poolb[b]):>13}{human(fillb[b]):>8}"
              f"{human(truth[b]):>12}{delta:>+8,}")
    if worst > 2048:
        print(f"  !! off by {human(worst)} bytes - do NOT quote the breakdown below until that is")
        print("     explained. Something in the map is not being read correctly.")
    else:
        print("  ok - within alignment noise, so the breakdown below can be trusted.")
    return worst


def report(a, detail_for=None, top=25, want_strings=False):
    print("=" * 98)
    print(f"  {a['env']}")
    print("=" * 98)

    if a["bin_bytes"] is not None:
        print(f"\nfirmware.bin {human(a['bin_bytes'])} bytes - the real figure. PlatformIO's")
        print('"Flash: nn%" line omits .eh_frame, so on RISC-V it reads about 64k low.')

    reconcile(a)

    print("\nSections in the image:")
    for name, size in sorted(a["elf_real"].items(), key=lambda kv: -kv[1]):
        if size:
            print(f"  {name:<28}{human(size):>12}")

    pool_total = sum(a["pool"].values())
    if pool_total:
        print(f"\nOf which {human(pool_total)} bytes are merged string literals, charged to no")
        print("library: ld pools identical strings across the whole link, so they genuinely do not")
        print("belong to any single one. --strings gives a per-library upper bound instead.")

    rows = sorted(((sum(b.get(x, 0) for x in FLASH_BUCKETS), owner, b)
                   for owner, b in a["by_owner"].items()), reverse=True)
    print(f"\n{'-' * 98}\nFlash by library, excluding pooled strings\n{'-' * 98}")
    print(f"{'library':<36}{'flash code':>13}{'flash data':>13}{'iram':>9}{'ram':>9}{'FLASH':>13}")
    for flash, owner, b in rows:
        print(f"{owner[:35]:<36}{human(b.get(FLASH_CODE, 0)):>13}{human(b.get(FLASH_DATA, 0)):>13}"
              f"{human(b.get(RAM_CODE, 0)):>9}{human(b.get(RAM_DATA, 0)):>9}{human(flash):>13}")

    if want_strings and a["strings"]:
        print(f"\n{'-' * 98}\nString literals declared per library - upper bound, before ld dedups"
              f"\n{'-' * 98}")
        print("Objects that survived gc only, from the object files - so archive members (Arduino")
        print(f"core, IDF) cannot be shown. Sums to more than the {human(pool_total)} actually in")
        print("the image, because ld keeps only one copy of each distinct string.")
        for owner, n in sorted(a["strings"].items(), key=lambda kv: -kv[1]):
            if n:
                print(f"  {owner[:40]:<42}{human(n):>12}")

    # --- the question this script exists to answer
    print(f"\n{'-' * 98}\nCompiled but contributed ZERO bytes - --gc-sections earning its keep"
          f"\n{'-' * 98}")
    for owner in sorted(a["compiled"]):
        allf = set(a["compiled"][owner])
        if not allf:
            continue
        dead = sorted(allf - set(a["by_detail"].get(owner, {})))
        if not dead:
            continue
        verdict = "ALL DROPPED" if len(dead) == len(allf) else \
            f"{len(allf) - len(dead)} of {len(allf)} used"
        print(f"\n  {owner}  [{verdict}]")
        for d in dead:
            print(f"      0 bytes  {d}")

    if detail_for:
        match = [o for o in a["by_detail"] if o.lower() == detail_for.lower()]
        if not match:
            print(f"\n(no library {detail_for!r}; have: {', '.join(sorted(a['by_detail']))})")
        else:
            owner = match[0]
            print(f"\n{'-' * 98}\nPer-file detail: {owner}\n{'-' * 98}")
            print(f"{'file':<50}{'flash code':>13}{'flash data':>13}{'FLASH':>13}")
            for flash, d, b in sorted(((sum(x.get(y, 0) for y in FLASH_BUCKETS), d, x)
                                       for d, x in a["by_detail"][owner].items()), reverse=True):
                print(f"{d[:49]:<50}{human(b.get(FLASH_CODE, 0)):>13}"
                      f"{human(b.get(FLASH_DATA, 0)):>13}{human(flash):>13}")

    print(f"\n{'-' * 98}\nBiggest {top} input sections\n{'-' * 98}")
    names = [s[1].split(".", 2)[-1] for s in a["biggest"][:top]]
    pretty = demangle(names)
    for size, in_sec, owner, _det, bucket in a["biggest"][:top]:
        sym = in_sec.split(".", 2)[-1]
        print(f"{human(size):>9}  {bucket:<12} {owner[:20]:<21} {pretty.get(sym, sym)[:60]}")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("envs", nargs="*", help="env names; default is every env already built")
    ap.add_argument("--detail", help="per-source-file breakdown for this library")
    ap.add_argument("--strings", action="store_true", help="per-library string-literal upper bound")
    ap.add_argument("--top", type=int, default=25)
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()

    envs = args.envs or sorted(
        p.name for p in BUILD_ROOT.iterdir() if p.is_dir() and (p / "firmware.elf").exists())
    if not envs:
        sys.exit(f"nothing built under {BUILD_ROOT}")

    out = []
    for env in envs:
        try:
            a = analyse(env, want_strings=args.strings or args.json)
        except (FileNotFoundError, ValueError) as e:
            print(f"{env}: {e}", file=sys.stderr)
            continue
        if args.json:
            out.append({k: v for k, v in a.items() if k not in ("biggest", "inclusions")})
        else:
            report(a, detail_for=args.detail, top=args.top, want_strings=args.strings)
            print()
    if args.json:
        print(json.dumps(out, indent=1, default=str))


if __name__ == "__main__":
    main()
