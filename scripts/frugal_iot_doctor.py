#!/usr/bin/env python3
"""Frugal-IoT doctor - checks an Arduino IDE or PlatformIO setup for the problems that stop a
Frugal-IoT sketch building, and says what to do about each one.

Send this one file to whoever is stuck. They run it from their project (PlatformIO) or sketch
(Arduino IDE) folder and paste the whole output back:

    python3 frugal_iot_doctor.py                 # checks the current folder
    python3 frugal_iot_doctor.py path/to/project
    python3 frugal_iot_doctor.py --arduino       # force the Arduino IDE checks
    python3 frugal_iot_doctor.py --platformio    # force the PlatformIO checks

It only reads files, never changes anything, and needs nothing beyond Python 3.6.

Each check is a lesson from a real failure, noted beside it, so that a new failure becomes a new
check here rather than another round of guessing over chat.
"""

import configparser
import glob
import json
import os
import platform
import re
import subprocess
import sys
from pathlib import Path

# ── Output ────────────────────────────────────────────────────────────────────────────────────

counts = {"FAIL": 0, "WARN": 0}

# Raise this when a release changes something the examples depend on
OLDEST_SUPPORTED = "2.0.0"


def report(level, message, fix=None):
    """level is OK, INFO, WARN or FAIL. fix is what to do about it, printed underneath."""
    if level in counts:
        counts[level] += 1
    print(f"  {level:<4}  {message}")
    if fix:
        for line in fix.splitlines():
            print(f"        {line}")


def heading(text):
    print()
    print(f"== {text}")


# ── Helpers ───────────────────────────────────────────────────────────────────────────────────

def run(cmd):
    """First line of a command's output, or None if it is not installed or fails."""
    out = None
    try:
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                universal_newlines=True, timeout=30)
        if result.returncode == 0 and result.stdout.strip():
            out = result.stdout.strip().splitlines()[0]
    except (OSError, subprocess.SubprocessError):
        pass
    return out


def version_tuple(text):
    return tuple(int(n) for n in re.findall(r"\d+", text or "")[:3])


def satisfies(version, spec):
    """Does version meet a PlatformIO spec - ^x.y.z, ~x.y.z, >=x.y.z, x.y.z or empty?"""
    v = version_tuple(version)
    want = version_tuple(spec)
    ok = True
    if want and v:
        if spec.startswith("^"):
            ok = v >= want and v[0] == want[0]
        elif spec.startswith("~"):
            ok = v >= want and v[:2] == want[:2]
        elif spec.startswith(">="):
            ok = v >= want
        else:
            ok = v == want
    return ok


def read_library(folder):
    """{name, version, owner, depends, headers} for a library folder, or None if it is not one."""
    info = None
    lj = folder / "library.json"
    lp = folder / "library.properties"
    if lj.is_file() or lp.is_file():
        info = {"name": folder.name, "version": "", "owner": "", "depends": [], "dependencies": [],
                "path": folder}
        if lp.is_file():
            for line in lp.read_text(errors="replace").splitlines():
                key, _, value = line.partition("=")
                key = key.strip()
                if key == "name":
                    info["name"] = value.strip()
                elif key == "version":
                    info["version"] = value.strip()
                elif key == "depends":
                    # "RadioLib (>=7.1.2)" - the constraint is not needed to say what is missing
                    info["depends"] = [re.sub(r"\s*\(.*\)", "", d).strip()
                                       for d in value.split(",") if d.strip()]
        if lj.is_file():
            try:
                data = json.loads(lj.read_text(errors="replace"))
                info["name"] = data.get("name", info["name"])
                info["version"] = data.get("version", info["version"])
                owner = data.get("owner", "")
                if not owner:
                    repo = (data.get("repository") or {}).get("url", "")
                    m = re.search(r"github\.com/([^/]+)/", repo)
                    owner = m.group(1) if m else ""
                info["owner"] = owner
                deps = data.get("dependencies", [])
                if isinstance(deps, dict):  # the older {"name": "version"} form
                    deps = [{"name": k, "version": v} for k, v in deps.items()]
                info["dependencies"] = deps
            except (ValueError, AttributeError):
                report("WARN", f"{lj} is not valid JSON")
        src = folder / "src" if (folder / "src").is_dir() else folder
        info["headers"] = {p.name for p in src.glob("*.h")}
    return info


def libraries_in(folder):
    libs = []
    if folder.is_dir():
        for sub in sorted(folder.iterdir()):
            if sub.is_dir():
                lib = read_library(sub)
                if lib:
                    libs.append(lib)
    return libs


def norm(name):
    """Library Manager says "ESP Async WebServer", PlatformIO "ESPAsyncWebServer" - same thing."""
    return re.sub(r"[^a-z0-9]", "", name.lower())


def files_defining_setup(folder, recurse):
    """Source files with a top-level `void setup(`. Two of them is a duplicate-symbol link error."""
    found = []
    pattern = "**/*" if recurse else "*"
    for p in folder.glob(pattern):
        skip = any(part in ("lib", ".pio", "libraries", "examples", ".git") for part in
                   p.relative_to(folder).parts[:-1])
        if p.is_file() and not skip and p.suffix in (".ino", ".cpp", ".c"):
            if re.search(r"^\s*void\s+setup\s*\(\s*(void)?\s*\)", p.read_text(errors="replace"), re.M):
                found.append(p)
    return found


def lora_board_macros(frugal_src):
    """Arduino board macros that turn on SYSTEM_LORAMESHER_WANT, read from the library's own
    _settings.h so this never drifts from it. Same parse as generate_platform_h.py."""
    wanted = set()
    settings = frugal_src / "_settings.h"
    if settings.is_file():
        text = settings.read_text(errors="replace")
        derived = {}
        for cond, name in re.findall(r"^#if\s+(.*?)\s*\n\s*#define\s+(\w+)", text, re.M):
            derived[name] = set(re.findall(r"\b(ARDUINO_\w+)", cond))
            if name == "SYSTEM_LORAMESHER_WANT":
                wanted = derived[name]
        wanted = wanted.union(*(derived.get(w, set()) for w in wanted))
    return wanted


# ── Environment ───────────────────────────────────────────────────────────────────────────────

def check_environment():
    heading("Environment")
    report("INFO", f"{platform.system()} {platform.release()} ({platform.machine()}), "
                   f"Python {platform.python_version()}")
    pio = run(["pio", "--version"]) or run(["platformio", "--version"])
    cli = run(["arduino-cli", "version"])
    report("INFO", f"PlatformIO: {pio or 'not on PATH'}")
    report("INFO", f"arduino-cli: {cli or 'not on PATH (fine - the Arduino IDE has its own)'}")


# ── PlatformIO ────────────────────────────────────────────────────────────────────────────────

class PioConfig:
    """Just enough of PlatformIO's ini handling: extra_configs, ${section.key}, extends and [env]."""

    def __init__(self, project):
        self.cp = configparser.ConfigParser(interpolation=None, strict=False,
                                            inline_comment_prefixes=(";", "#"))
        ini = project / "platformio.ini"
        self.files = [ini]
        self.cp.read(ini)
        for pattern in self.get("platformio", "extra_configs", "").split():
            for extra in sorted(glob.glob(str(project / pattern))):
                self.files.append(Path(extra))
                self.cp.read(extra)

    def get(self, section, key, default=None):
        value = default
        if self.cp.has_option(section, key):
            value = self.expand(self.cp.get(section, key))
        return value

    def expand(self, value, depth=0):
        def sub(m):
            return self.get(m.group(1), m.group(2), "") if depth < 10 else ""
        return re.sub(r"\$\{([^.}]+)\.([^}]+)\}", sub, value)

    def envs(self):
        return [s[4:] for s in self.cp.sections() if s.startswith("env:")]

    def env_get(self, env, key, default=None):
        """Own value, then each `extends` in turn, then [env] - PlatformIO's order."""
        value = None
        seen = set()
        todo = [f"env:{env}"]
        while todo and value is None:
            section = todo.pop(0)
            if section not in seen:
                seen.add(section)
                value = self.get(section, key)
                if value is None:
                    todo += [s.strip() for s in self.get(section, "extends", "").split(",") if s.strip()]
        if value is None:
            value = self.get("env", key, default)
        return value


def lib_dep_entries(value):
    entries = []
    for line in (value or "").splitlines():
        for item in line.split(","):
            item = item.strip()
            if item and not item.startswith((";", "#")):
                entries.append(item)
    return entries


def family_of(platform_value):
    p = (platform_value or "").lower()
    family = "other"
    if "8266" in p:
        family = "espressif8266"
    elif "espressif32" in p or "pioarduino" in p:
        family = "espressif32"
    return family


def check_platformio(project):
    heading(f"PlatformIO project: {project}")
    cfg = PioConfig(project)
    report("INFO", "config files read: " + ", ".join(f.name for f in cfg.files))

    lib_dir = project / cfg.get("platformio", "lib_dir", "lib")
    local_libs = libraries_in(lib_dir)
    local_frugal = [l for l in local_libs if norm(l["name"]) == "frugaliot"]

    # src_dir = . is right for a standalone example with its .ino at the root, and in a project
    # with lib/ it compiles lib/Frugal-IoT/src as project source AND as a library - every symbol
    # in the library then fails to link as a "multiple definition". Seen in beta testing.
    src_dir_value = cfg.get("platformio", "src_dir", "src")
    src_dir = (project / src_dir_value).resolve()
    if src_dir == project.resolve():
        if local_libs:
            report("FAIL", f"src_dir = {src_dir_value} with libraries in {lib_dir.name}/ - every library "
                           "there is compiled twice (\"multiple definition of ...\" from ld)",
                   "Comment out `src_dir = .` in [platformio] and put your sketch in src/main.cpp.\n"
                   "It is only for an example built where it stands, with its .ino at the root.")
        else:
            report("OK", f"src_dir = {src_dir_value} (fine with nothing in {lib_dir.name}/)")
    else:
        report("OK", f"src_dir = {src_dir_value}")
        if (project / "src").resolve() != src_dir and (project / "src").is_dir():
            report("WARN", "there is a src/ folder, but src_dir points elsewhere, so src/ is not built")

    if src_dir.is_dir():
        setups = files_defining_setup(src_dir, recurse=True)  # skips lib/ and .pio/
        if len(setups) > 1:
            report("FAIL", "more than one file defines setup(): " +
                   ", ".join(str(p.relative_to(project)) for p in setups),
                   "Keep one. An example's .ino copied in beside main.cpp is the usual cause.")
        elif not setups:
            report("WARN", f"no file in {src_dir_value} defines setup() - is the sketch where src_dir says?")

    for f in cfg.files:
        if re.search(r"^\s*lib_extra_dirs\s*=", f.read_text(errors="replace"), re.M):
            report("WARN", f"{f.name} sets lib_extra_dirs - if it names {lib_dir.name}/ the libraries "
                           "there are found twice")

    if local_frugal:
        for lib in local_frugal:
            report("INFO", f"Frugal-IoT {lib['version']} in {lib['path'].relative_to(project)} (a local copy)")
    if len(local_frugal) > 1:
        report("FAIL", f"{len(local_frugal)} copies of Frugal-IoT in {lib_dir.name}/", "Keep one.")

    frugal_src = None
    if local_frugal:
        frugal_src = local_frugal[0]["path"] / "src"

    global_lib = Path.home() / ".platformio" / "lib"
    for lib in libraries_in(global_lib):
        if norm(lib["name"]) in ("frugaliot", "espasyncwebserver", "asynctcp", "espasynctcp"):
            report("WARN", f"{lib['name']} is installed globally in {global_lib} - it can be picked "
                           "up in place of the project's own", f"Remove {lib['path']} unless you meant it.")

    check_pio_local_files(project, cfg)

    envs = cfg.envs()
    if not envs:
        report("FAIL", "no [env:...] sections - nothing to build")
    libdeps_root = project / ".pio" / "libdeps"
    for env in envs:
        check_pio_env(cfg, env, project, libdeps_root / env, local_libs, local_frugal, frugal_src)


def check_pio_env(cfg, env, project, libdeps, local_libs, local_frugal, frugal_src):
    heading(f"env:{env}")
    family = family_of(cfg.env_get(env, "platform", ""))
    board = cfg.env_get(env, "board", "?")
    report("INFO", f"board = {board}, platform family = {family}")
    if family == "other":
        report("WARN", "not an espressif32 or espressif8266 platform - Frugal-IoT only supports those")

    deps_value = cfg.env_get(env, "lib_deps")
    entries = lib_dep_entries(deps_value)
    frugal_specs = [e for e in entries if "frugal-iot" in e.lower() or "frugal_iot" in e.lower()]

    # An env that sets lib_deps to nothing makes PlatformIO reinstall every library on every build
    if deps_value is not None and not entries:
        report("WARN", "lib_deps is set but empty - PlatformIO then reinstalls every library on each build",
               "Delete the lib_deps line, or list Frugal-IoT in it.")

    for spec in frugal_specs:
        if spec.startswith(("symlink://", "file://")) or "/" in spec.split("@")[0] and "://" in spec:
            report("WARN", f"lib_deps has `{spec}` - a symlink, path or git URL may not install "
                           "Frugal-IoT's own dependencies (ESPAsyncWebServer.h then goes missing)",
                   "Use `Frugal-IoT@^2.0.5` and, to work on the library, clone it into lib/.")
        elif "/" in spec.split("@")[0]:
            report("WARN", f"lib_deps has `{spec}` with an owner prefix - that installs a registry copy "
                           "even when lib/ has one", "Use the bare name: `Frugal-IoT@^2.0.5`.")
        elif local_frugal:
            want = spec.split("@", 1)[1] if "@" in spec else ""
            have = local_frugal[0]["version"]
            if satisfies(have, want):
                report("OK", f"lib_deps `{spec}` is satisfied by the copy in lib/ ({have})")
            else:
                report("FAIL", f"lib_deps `{spec}` is NOT satisfied by lib/'s Frugal-IoT {have} - "
                               "PlatformIO downloads a second copy and every symbol is defined twice",
                       f"Change the lib_deps line to `Frugal-IoT@^{have}`.")
    if frugal_specs and not local_frugal:
        report("OK", f"Frugal-IoT from the registry: `{frugal_specs[0]}`")
    if not frugal_specs and not local_frugal:
        report("FAIL", "Frugal-IoT is neither in lib_deps nor in lib/ - Frugal-IoT.h will not be found",
               "Add `Frugal-IoT@^2.0.5` to lib_deps.")

    lora_macros = lora_board_macros(frugal_src) if frugal_src else set()
    defines = cfg.env_get(env, "build_flags", "") or ""
    wants_lora = "SYSTEM_LORAMESHER_WANT" in defines or any(
        m.lower().endswith(board.lower().replace("-", "_")) for m in lora_macros)
    if wants_lora and not any("loramesher" in e.lower() for e in entries):
        report("WARN", "this looks like a LoRa board but lib_deps has no LoRaMesher - "
                       "loramesher.hpp will not be found",
               "Add https://github.com/loramesher/LoRaMesher.git to this env's lib_deps.")

    if (cfg.env_get(env, "lib_ldf_mode", "") or "").strip() == "off":
        report("WARN", "lib_ldf_mode = off - no libraries get linked at all")
    ignored = lib_dep_entries(cfg.env_get(env, "lib_ignore", ""))
    if ignored:
        report("INFO", "lib_ignore: " + ", ".join(ignored))

    if libdeps.is_dir():
        check_pio_libdeps(env, family, libdeps, local_libs, local_frugal)
    else:
        report("INFO", f"not built yet (no .pio/libdeps/{env}) - run `pio run -e {env}` and run this again "
                       "to have the installed libraries checked too")


def check_pio_libdeps(env, family, libdeps, local_libs, local_frugal):
    installed = libraries_in(libdeps)
    have = {norm(l["name"]): l for l in installed + local_libs}
    report("INFO", "installed: " + ", ".join(sorted(l["name"] for l in installed)))

    pio_frugal = [l for l in installed if norm(l["name"]) == "frugaliot"]
    if pio_frugal and local_frugal:
        report("FAIL", f"Frugal-IoT is in .pio/libdeps/{env} ({pio_frugal[0]['version']}) AND in lib/ "
                       f"({local_frugal[0]['version']}) - two copies, every symbol defined twice",
               f"Make the lib_deps version match lib/'s, then delete .pio/libdeps/{env} and rebuild.")

    # Dependencies are installed only when the library that needs them is first installed, so one
    # that failed to download then stays missing for good. Seen in beta testing as AsyncTCP.h.
    frugal = (local_frugal or pio_frugal or [None])[0]
    chain = [frugal] if frugal else []
    seen = set()
    missing = 0
    while chain:
        lib = chain.pop(0)
        for dep in lib.get("dependencies", []):
            name = dep.get("name", "").split("/")[-1]  # "paulstoffregen/OneWire" is allowed
            platforms = dep.get("platforms", "*")
            platforms = [platforms] if isinstance(platforms, str) else platforms
            relevant = "*" in platforms or family in platforms
            if relevant and norm(name) not in seen:
                seen.add(norm(name))
                if norm(name) in have:
                    chain.append(have[norm(name)])
                else:
                    missing += 1
                    report("FAIL", f"{name} is missing (needed by {lib['name']})",
                           f"Delete .pio/libdeps/{env} and rebuild - a dependency that failed to\n"
                           "install the first time is never retried.")

    web = have.get("espasyncwebserver")
    if web and web.get("owner") and web["owner"].lower() not in ("esp32async", ""):
        report("WARN", f"ESPAsyncWebServer comes from {web['owner']}, not ESP32Async - an older fork "
                       "with different dependencies", "Remove it from your own lib_deps; Frugal-IoT pulls in the right one.")
    tcp = {"espressif32": "asynctcp", "espressif8266": "espasynctcp"}.get(family)
    if web and tcp and tcp not in have and tcp not in seen:  # else already reported above
        report("FAIL", f"ESPAsyncWebServer is installed without {'AsyncTCP' if tcp == 'asynctcp' else 'ESPAsyncTCP'}",
               f"Delete .pio/libdeps/{env} and rebuild.")
    if frugal and not missing:
        report("OK", "every dependency Frugal-IoT declares is installed")


# ── Settings files: the enrolment secret and local overrides ─────────────────────────────────

TEMPLATES_URL = "https://github.com/mitra42/frugal-iot/tree/main/templates"
PLACEHOLDER = "paste-your-enrolment-secret-here"  # what the templates ship with


def find_template(name, *roots):
    """Where this person's own copy of the library keeps a template, else the GitHub page."""
    candidates = [Path(__file__).resolve().parent.parent / "templates" / name]
    for root in roots:
        candidates += sorted(Path(root).glob(f"*/templates/{name}"))     # lib/Frugal-IoT, libraries/
        candidates += sorted(Path(root).glob(f"*/*/templates/{name}"))   # .pio/libdeps/<env>/
    found = next((c for c in candidates if c.is_file()), None)
    return str(found) if found else f"{TEMPLATES_URL}/{name}"


def secret_state(text, header):
    """set, placeholder, empty or absent. Deliberately never returns the secret itself - this
    output gets pasted into chats and issues."""
    if header:
        m = re.search(r'^\s*#define\s+SYSTEM_MQTT_ENROL_SECRET\s+"([^"]*)"', text, re.M)
    else:
        lines = [l for l in text.splitlines() if not l.strip().startswith((";", "#"))]
        m = re.search(r'SYSTEM_MQTT_ENROL_SECRET=\\?"?([^"\'\\\s]*)', "\n".join(lines))
    state = "absent"
    if m:
        value = m.group(1)
        state = "placeholder" if value == PLACEHOLDER else ("set" if value else "empty")
    return state


def near_misses(folder, wanted):
    """Files that were plainly meant to be one of `wanted` but will not be found under the name
    they have - platformio-local.ini.txt (Windows hides the extension), a .template copied without
    renaming, an underscore for the hyphen."""
    misses = []
    for p in folder.iterdir() if folder.is_dir() else []:
        squashed = re.sub(r"[^a-z]", "", p.name.lower())
        for w in wanted:
            if p.name != w and squashed.startswith(re.sub(r"[^a-z]", "", w.lower())):
                misses.append((p.name, w))
    return misses


REFUSED = ("the node builds, but is refused when it first enrols - it shows as \"Failed\" on the "
           "dashboard's Nodes card until an administrator approves it")


def check_pio_local_files(project, cfg):
    heading("PlatformIO settings files (*-local.ini)")
    ini_text = (project / "platformio.ini").read_text(errors="replace")
    loads_local = "local.ini" in (cfg.get("platformio", "extra_configs", "") or "")
    secrets = project / "platformio-secrets-local.ini"
    local = project / "platformio-local.ini"
    roots = [project / cfg.get("platformio", "lib_dir", "lib"), project / ".pio" / "libdeps"]

    for name, meant in near_misses(project, [secrets.name, local.name]):
        report("WARN", f"{name} will not be loaded - it has to be named exactly {meant}",
               f"Rename it to {meant}. On Windows, turn on File name extensions in Explorer to see a hidden .txt.")

    if not loads_local:
        if secrets.is_file() or local.is_file():
            report("WARN", "platformio.ini has no `extra_configs = *-local.ini`, so the *-local.ini "
                           "files here are ignored", "Add it under [platformio], as in the examples.")
        else:
            report("INFO", "platformio.ini does not load *-local.ini files - skipping these checks")
    else:
        if "build_flags_secrets" not in ini_text:
            report("WARN", "platformio.ini never uses ${common.build_flags_secrets}, so the secret in "
                           "platformio-secrets-local.ini never reaches the build",
                   "Add ${common.build_flags_secrets} to build_flags, as in the examples.")

        if secrets.is_file():
            state = secret_state(secrets.read_text(errors="replace"), header=False)
            if state == "set":
                report("OK", f"{secrets.name} sets the enrolment secret (value not shown)")
            else:
                report("WARN", f"{secrets.name} has the enrolment secret {state} - {REFUSED}",
                       "Paste the line from the dashboard: gear icon -> Nodes -> Enrolment secret.")
        else:
            report("WARN", f"no {secrets.name} - {REFUSED}",
                   f"Copy {find_template(secrets.name + '.template', *roots)}\n"
                   f"to {secrets} and paste your enrolment secret into it.")

        if local.is_file():
            report("OK", f"{local.name} present")
        else:
            report("INFO", f"no {local.name} - optional, for your own debug flags and serial port",
                   f"Template: {find_template(local.name + '.template', *roots)}")

        # A value in a *-local.ini REPLACES rather than merges, so a key in both files silently
        # loses one of them - and a secret in platformio-local.ini never reaches an OTA build
        keys = {}
        for f in (secrets, local):
            if f.is_file():
                section = ""
                for line in f.read_text(errors="replace").splitlines():
                    m = re.match(r"^\[(.+)\]", line)
                    if m:
                        section = m.group(1)
                    m = re.match(r"^([A-Za-z_][\w.]*)\s*=", line)
                    if m:
                        keys.setdefault((section, m.group(1)), []).append(f.name)
        for (section, key), files in sorted(keys.items()):
            if len(files) > 1:
                report("WARN", f"[{section}] {key} is set in both {' and '.join(files)} - one silently "
                               "replaces the other", "Keep it in one file.")
        if local.is_file() and secret_state(local.read_text(errors="replace"), header=False) == "set":
            report("WARN", f"the enrolment secret is in {local.name}; it belongs in {secrets.name}",
                   "Move the line - an OTA build takes the secrets file and nothing else.")


# ── Arduino IDE ───────────────────────────────────────────────────────────────────────────────

def arduino_dirs():
    """(sketchbook, data dir) - from arduino-cli when it is there, else each OS's default."""
    home = Path.home()
    user = run(["arduino-cli", "config", "get", "directories.user"])
    data = run(["arduino-cli", "config", "get", "directories.data"])
    system = platform.system()
    if not user:
        user = str(home / "Arduino") if system == "Linux" else str(home / "Documents" / "Arduino")
    if not data:
        if system == "Darwin":
            data = str(home / "Library" / "Arduino15")
        elif system == "Windows":
            data = str(Path(os.environ.get("LOCALAPPDATA", home / "AppData" / "Local")) / "Arduino15")
        else:
            data = str(home / ".arduino15")
    return Path(user), Path(data)


def check_arduino(sketch, sketchbook_override):
    sketchbook, data = arduino_dirs()
    if sketchbook_override:
        sketchbook = Path(sketchbook_override)
    heading(f"Arduino IDE: sketchbook {sketchbook}")
    libdir = sketchbook / "libraries"
    if not libdir.is_dir():
        report("FAIL", f"no libraries folder at {libdir}",
               "If your sketchbook is elsewhere (File > Preferences > Sketchbook location), run again\n"
               "with --sketchbook <that folder>.")
    libs = libraries_in(libdir)
    by_name = {}
    for lib in libs:
        by_name.setdefault(norm(lib["name"]), []).append(lib)

    # A second copy - often a GitHub zip unpacked as frugal-iot-main - and the IDE picks one of them
    frugal = by_name.get("frugaliot", []) + [l for l in libs if norm(l["path"].name).startswith("frugaliot")
                                             and norm(l["name"]) != "frugaliot"]
    if not frugal:
        report("FAIL", "Frugal-IoT is not installed - the build stops at Frugal-IoT.h",
               "Tools > Manage Libraries, search for Frugal-IoT, Install, and choose Install All.")
    for lib in frugal:
        report("INFO", f"Frugal-IoT {lib['version']} in {lib['path']}")
        if version_tuple(lib["version"]) < version_tuple(OLDEST_SUPPORTED):
            report("WARN", f"Frugal-IoT {lib['version']} is older than {OLDEST_SUPPORTED} - the examples and "
                           "READMEs are written for the current release",
                   "Tools > Manage Libraries, find Frugal-IoT, Update.")
    if len(frugal) > 1:
        report("FAIL", f"{len(frugal)} copies of Frugal-IoT - the IDE uses one and you cannot tell which",
               "Delete all but one from " + str(libdir))

    if frugal:
        missing = [d for d in frugal[0]["depends"] if norm(d) not in by_name]
        for name in missing:
            report("FAIL", f"{name} is not installed (Frugal-IoT depends on it)",
                   f"Tools > Manage Libraries, search for \"{name}\", Install.")
        if frugal[0]["depends"] and not missing:
            report("OK", f"all {len(frugal[0]['depends'])} libraries Frugal-IoT depends on are installed")

    for name, needed in (("loramesher", "LoRaMesher"), ("radiolib", "RadioLib")):
        if name in by_name:
            report("OK", f"{needed} installed (LoRa boards)")
        else:
            report("INFO", f"{needed} not installed - only needed for LoRa boards (Heltec, TTGO, T-Beam, T3-S3)",
                   "See the example's README for how to install LoRaMesher, which is not in the Library Manager."
                   if name == "loramesher" else None)

    # The IDE takes the first library offering a header, so an old fork can shadow the right one
    for header in ("ESPAsyncWebServer.h", "AsyncTCP.h", "ESPAsyncTCP.h"):
        providers = [l for l in libs if header in l.get("headers", set())]
        if len(providers) > 1:
            report("WARN", f"{len(providers)} libraries provide {header}: " +
                   ", ".join(f"{l['name']} {l['version']} ({l['path'].name})" for l in providers),
                   "Keep the ESP32Async one (\"ESP Async WebServer\", \"Async TCP\", \"ESP Async TCP\").")

    check_arduino_cores(data)
    if sketch:
        check_arduino_sketch(sketch, frugal, libdir)


def check_arduino_cores(data):
    heading(f"Arduino IDE: board packages in {data}")
    found = False
    for package, arch, label in (("esp32", "esp32", "esp32 by Espressif Systems"),
                                 ("esp8266", "esp8266", "esp8266 by ESP8266 Community")):
        versions = sorted((p.name for p in (data / "packages" / package / "hardware" / arch).glob("*")
                           if p.is_dir()), key=version_tuple)
        if versions:
            found = True
            report("OK", f"{label}: {', '.join(versions)}")
            if package == "esp32" and version_tuple(versions[-1]) < (3,):
                report("WARN", f"esp32 core {versions[-1]} is older than 3.0 - Frugal-IoT is built against 3.x",
                       "Tools > Board > Boards Manager, update esp32.")
        else:
            report("INFO", f"{label}: not installed")
    if not found:
        report("FAIL", "neither the esp32 nor the esp8266 board package is installed",
               "Tools > Board > Boards Manager, install esp32 by Espressif Systems.")


def check_arduino_sketch(sketch, frugal, libdir):
    heading(f"Arduino IDE: sketch {sketch}")
    inos = sorted(sketch.glob("*.ino"))
    if not inos:
        report("WARN", "no .ino in this folder - run this from the sketch's folder to check the sketch")
    elif not (sketch / f"{sketch.name}.ino").is_file():
        report("FAIL", f"the folder is called {sketch.name} but no {sketch.name}.ino is in it - the IDE "
                       "will not open it as a sketch", "Rename the folder or the .ino so they match.")
    else:
        report("OK", f"{sketch.name}.ino")

    setups = files_defining_setup(sketch, recurse=False)
    if len(setups) > 1:
        report("FAIL", "more than one file defines setup(): " + ", ".join(p.name for p in setups),
               "Arduino compiles every .ino/.cpp in the sketch folder - keep one.")

    # build_opt.h is a list of compiler flags despite the .h - a #define stops the build
    opt = sketch / "build_opt.h"
    if opt.is_file():
        bad = [l for l in opt.read_text(errors="replace").splitlines() if l.strip().startswith("#")]
        if bad:
            report("FAIL", f"build_opt.h contains `{bad[0].strip()}` - that file holds compiler flags, not C",
                   "Write -DNAME instead of #define NAME.")
        else:
            report("OK", "build_opt.h holds flags")

    # The Arduino form of platformio-secrets-local.ini - _settings.h finds it in the sketch folder
    header = sketch / "platform-secrets-local.h"
    for name, meant in near_misses(sketch, [header.name]):
        report("WARN", f"{name} will not be found - it has to be named exactly {meant}",
               f"Rename it to {meant}. On Windows, turn on File name extensions in Explorer to see a hidden .txt.")
    globals_h = list(sketch.glob("*.ino.globals.h"))
    in_globals = any(secret_state(g.read_text(errors="replace"), header=True) == "set" for g in globals_h)
    if header.is_file():
        state = secret_state(header.read_text(errors="replace"), header=True)
        if state == "set":
            report("OK", f"{header.name} sets the enrolment secret (value not shown)")
        else:
            report("WARN", f"{header.name} has the enrolment secret {state} - {REFUSED}",
                   "Paste it from the dashboard: gear icon -> Nodes -> Enrolment secret.")
    elif in_globals:
        report("OK", "the ESP8266 globals file sets the enrolment secret (value not shown)")
    else:
        report("WARN", f"no {header.name} - {REFUSED}",
               f"ESP32: copy {find_template(header.name + '.template', libdir)}\n"
               f"into this folder as {header.name} and paste your secret into it.\n"
               "ESP8266: put the same #define in your <sketch>.ino.globals.h instead.")

    stray = sketch / "esp8266"
    if stray.is_dir() and any(stray.glob("*.ino.globals.h")):
        moved = any(sketch.glob("*.ino.globals.h"))
        report("INFO" if moved else "WARN",
               "ESP8266 globals file has been moved up beside the .ino" if moved else
               "esp8266/<sketch>.ino.globals.h has not been moved up beside the .ino - "
               "needed for ESP8266 boards only", None if moved else "See esp8266/README.md.")

    # Opened from a git clone: the IDE compiles the INSTALLED library, not the clone's src/
    clone = next((p for p in [sketch] + list(sketch.parents) if (p / "library.properties").is_file()
                  and (p / "src" / "_settings.h").is_file()), None)
    if clone and libdir not in clone.parents:
        clone_version = (read_library(clone) or {}).get("version", "?")
        installed = frugal[0]["version"] if frugal else None
        report("WARN", f"this sketch is inside a Frugal-IoT clone ({clone}, {clone_version}) - the IDE "
                       f"builds against the installed library ({installed or 'none'}), not this clone",
               "Open it from File > Examples > Frugal-IoT instead, or install the clone as the library.")


# ── Main ──────────────────────────────────────────────────────────────────────────────────────

def main():
    args = [a for a in sys.argv[1:]]
    force = None
    sketchbook = None
    if "--arduino" in args:
        force = "arduino"
        args.remove("--arduino")
    if "--platformio" in args:
        force = "platformio"
        args.remove("--platformio")
    if "--sketchbook" in args:
        i = args.index("--sketchbook")
        sketchbook = args[i + 1] if i + 1 < len(args) else None
        del args[i:i + 2]
    if any(a in ("-h", "--help") for a in args):
        print(__doc__)
    else:
        target = Path(args[0] if args else ".").resolve()
        print(f"Frugal-IoT doctor - checking {target}")
        check_environment()
        has_ini = (target / "platformio.ini").is_file()
        has_ino = any(target.glob("*.ino"))
        # An example folder has both, and nothing in it says which tool is being used - so both
        do_pio = force == "platformio" or (force is None and has_ini)
        do_arduino = force == "arduino" or (force is None and (has_ino or not has_ini))
        if do_pio:
            if has_ini:
                check_platformio(target)
            else:
                report("FAIL", f"no platformio.ini in {target}", "Run this from the project folder.")
        if do_arduino:
            check_arduino(target if has_ino or force == "arduino" else None, sketchbook)
        print()
        print(f"Done: {counts['FAIL']} problem(s), {counts['WARN']} warning(s). "
              "Please paste everything above when asking for help.")
        sys.exit(1 if counts["FAIL"] else 0)


if __name__ == "__main__":
    main()
