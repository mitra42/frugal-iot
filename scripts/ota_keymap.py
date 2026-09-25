#!/usr/bin/env python3
"""
Map OTA keys to the (example, env) that builds them.

The key a node reports is SYSTEM_OTA_PREFIX "_" SYSTEM_OTA_SUFFIX - see system/ota.cpp, which
sends it as the retained ota/key. Neither half is reliably the name of the thing it comes from:
the suffix for [env:tbeam] was "ttgo-t-beam" until recently, and lilygohigrow's is "prebuilt".

Two rules follow, and both matter:

  1. The map is PARSED, never guessed. In particular it is never derived by splitting the key on
     "_", because both halves contain underscores - "sht_d1_mini" would split as ("sht",
     "d1_mini") only by luck, and "lcd_ht_c3_pico" would not split correctly at all.

  2. Parsing uses PlatformIO's own ProjectConfig, which resolves ${common.*} interpolation for us.
     A regex or awk parser reads "[env:d1_mini] ; also for d1_mini_pro v2..." as an env literally
     named "d1_mini] ; also for...". generate_platform_h.py and arduino_compile.bash both have
     that bug; this does not.

Importing platformio is deferred to scan(), so this module can be imported by any interpreter.
Callers that need scan() should call reexec_under_platformio() first.
"""

import os
import re
import sys
from collections import namedtuple

Entry = namedtuple("Entry", "example env prefix suffix")

PREFIX_RE = re.compile(r'SYSTEM_OTA_PREFIX="([^"]*)"')
SUFFIX_RE = re.compile(r'SYSTEM_OTA_SUFFIX="([^"]*)"')


def reexec_under_platformio(script=None):
    """Re-run this script under the PlatformIO python if the current one lacks platformio.

    ProjectConfig lives in PlatformIO's own virtualenv, not in the system python, so a plain
    `./ota_build.py` would fail on the import. Rather than make the caller remember
    `~/.platformio/penv/bin/python ./ota_build.py`, swap interpreters transparently.
    """
    try:
        import platformio  # noqa: F401
    except ImportError:
        candidate = os.environ.get("PLATFORMIO_PYTHON") or os.path.expanduser(
            "~/.platformio/penv/bin/python")
        script = os.path.abspath(script or sys.argv[0])
        if not os.path.exists(candidate):
            sys.exit(
                "This needs PlatformIO's python, which is not at\n"
                f"  {candidate}\n"
                "Set PLATFORMIO_PYTHON to it, or install PlatformIO Core.")
        if os.path.realpath(candidate) == os.path.realpath(sys.executable):
            sys.exit(f"{candidate} cannot import platformio - is the PlatformIO venv intact?")
        os.execv(candidate, [candidate, script] + sys.argv[1:])


def _flags_for(config, env):
    return " ".join(config.get("env:" + env, "build_flags", []))


def scan(examples_dir):
    """Return ({otakey: Entry}, [problem strings]) for every env under examples_dir.

    A problem is reported rather than raised: a single malformed example should not stop the
    other nineteen being buildable.
    """
    from platformio.project.config import ProjectConfig

    keymap = {}
    problems = []
    examples_dir = os.path.abspath(examples_dir)

    for name in sorted(os.listdir(examples_dir)):
        example_dir = os.path.join(examples_dir, name)
        ini = os.path.join(example_dir, "platformio.ini")
        if os.path.isfile(ini):
            # chdir first. "extra_configs = *-local.ini" is a GLOB, and PlatformIO expands it
            # against the process CWD rather than against the directory holding the ini. Parsing
            # from anywhere else silently folds in whatever *-local.ini happens to be in the
            # caller's directory - for the demo project that is the developer's debug flags and
            # real enrolment secret, attributed to an example that never asked for them.
            previous = os.getcwd()
            try:
                os.chdir(example_dir)
                config = ProjectConfig("platformio.ini")
                envs = config.envs()
            finally:
                os.chdir(previous)

            for env in envs:
                flags = _flags_for(config, env)
                prefix = PREFIX_RE.search(flags)
                suffix = SUFFIX_RE.search(flags)
                if not prefix:
                    problems.append(f"{name}/{env}: no SYSTEM_OTA_PREFIX - cannot be OTA'd")
                elif not suffix:
                    problems.append(f"{name}/{env}: no SYSTEM_OTA_SUFFIX - cannot be OTA'd")
                else:
                    prefix, suffix = prefix.group(1), suffix.group(1)
                    if prefix != name:
                        problems.append(
                            f"{name}/{env}: prefix {prefix!r} != directory name {name!r}")
                    if suffix != env.lower():
                        # A lint warning, not an error. lilygohigrow's suffix is "prebuilt"
                        # because the HiGrow is a consumer item rather than a known board, so
                        # naming it after the env would say less than naming what it is.
                        problems.append(
                            f"{name}/{env}: suffix {suffix!r} != env.lower() {env.lower()!r}")
                    key = f"{prefix}_{suffix}"
                    if key in keymap:
                        problems.append(
                            f"{key}: built by both {keymap[key].example}/{keymap[key].env} "
                            f"and {name}/{env} - ambiguous, refusing to guess")
                    else:
                        keymap[key] = Entry(name, env, prefix, suffix)

    return keymap, problems


def default_examples_dir():
    """examples/ as seen from this script, which lives in <library>/scripts/."""
    return os.path.normpath(
        os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, "examples"))


def main():
    import argparse
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[1])
    parser.add_argument("--examples", default=default_examples_dir(),
                        help="examples directory (default: alongside this script)")
    parser.add_argument("--problems", action="store_true",
                        help="list lint problems instead of the map")
    parser.add_argument("--json", action="store_true",
                        help="emit map and problems as JSON, for other tools to consume")
    args = parser.parse_args()

    keymap, problems = scan(args.examples)
    if args.json:
        import json
        json.dump({"keymap": {k: list(v) for k, v in keymap.items()},
                   "problems": problems}, sys.stdout)
        sys.stdout.write("\n")
    elif args.problems:
        for problem in problems:
            print(problem)
        print(f"# {len(problems)} problem(s)", file=sys.stderr)
    else:
        for key in sorted(keymap):
            entry = keymap[key]
            print(f"{key}\t{entry.example}\t{entry.env}")
        print(f"# {len(keymap)} keys, {len(problems)} problem(s) (--problems to list)",
              file=sys.stderr)


if __name__ == "__main__":
    reexec_under_platformio(__file__)
    main()
