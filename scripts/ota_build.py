#!/usr/bin/env python3
"""
Build firmware for a set of OTA keys, into a staging tree ready to upload.

  ./ota_build.py --key sht_d1_mini --key loadcell_supermini
  ./ota_build.py --manifest ota_keys.tsv --stage ~/ota-stage
  ./ota_build.py --key sht_d1_mini --lib registry
  ./ota_build.py --project ~/Documents/Arduino/frugal-iot-irrigation --all --lib registry

--project builds an application that lives in its own repository instead of the library's
examples/. It is the same harness, with three differences: the library is --library, or the
project's own lib/Frugal-IoT (usually a symlink), rather than the one this script is in; the
project's own git state is checked and recorded as well as the library's; and the project's
*-local.ini files are never staged - only --secrets is, which is how a release build keeps the
enrolment secret and drops a developer's debug flags and blanked lib_deps.

Output, laid out so the upload step is a straight walk:

  <stage>/firmware/<otakey>/firmware.bin    published to <org>/<project|+>/<otakey>/
  <stage>/firmware/<nodeid>/firmware.bin    published to <org>/<project|+>/<nodeid>/
  <stage>/firmware/<name>/build.json        provenance, and which of the two - build_record()

A manifest row with an otakey builds that key. A row with target_node set, plus example and env,
pins that build to one node instead - for a node reporting no ota/key, or one that should get
something different from every other node on its key.

WHY EACH BUILD IS STAGED, rather than run in examples/<name>/
─────────────────────────────────────────────────────────────
Four separate things go wrong building an example in place, and one staging step fixes all four.
Each of these was measured, not guessed:

  1. examples/ lives INSIDE the library, so examples/sht/lib/Frugal-IoT -> <library root> is a
     cycle: .../examples/sht/lib/Frugal-IoT/examples/sht/lib/Frugal-IoT resolves, forever. The
     Library Dependency Finder walks into it and never comes back - observed blocking for 57
     minutes on 1.3 seconds of CPU.

  2. lib_extra_dirs is NOT a way round that. It resolves Frugal-IoT locally, so PlatformIO
     considers the dependency satisfied and runs no Library Manager phase at all - none of the
     library's own transitive dependencies get installed, and the build dies on a missing
     ESPAsyncWebServer.h. The library has to be in the project's lib_dir.

  3. The staged library must be a PRUNED view, not the repo root. Linking the whole root into
     lib_dir exposes every OTHER example's leftover .pio/libdeps as candidate libraries: an
     ESP8266 build picked up examples/agri/.pio/libdeps/c3_pico/AsyncTCP and died on
     sdkconfig.h. Only src/ + the manifests are staged, which also makes (1) impossible.

  4. Building in place leaves <sketch>.ino.cpp in the sketch folder, and any <sketch>.ino* file
     there makes arduino-cli - and so the Arduino IDE - drop the example from File > Examples
     entirely.

Staging also happens to fix the "extra_configs = *-local.ini" glob hazard: the glob expands
against the process CWD, and a staging directory contains no stray *-local.ini for it to find.
"""

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time

from ota_keymap import reexec_under_platformio, scan, scan_projects, default_examples_dir

# What a Frugal-IoT library actually IS, for the pruned view described in (3) above. Deliberately
# not "everything except examples": a denylist would silently start staging any new top-level
# directory someone adds to the library.
LIBRARY_PARTS = ("src", "library.json", "library.properties", "keywords.txt")

# The name the secrets file must have in the staged project. Matches the "*-local.ini" glob that
# every example's platformio.ini already carries in extra_configs, so nothing needs editing.
SECRETS_NAME = "platformio-secrets-local.ini"


_FINGERPRINT = {}


def library_fingerprint(root):
    """A hash of the library's actual content, for the staged ini - see stage_project().

    PlatformIO does NOT reliably notice a library source change reached through the staged
    symlink: measured with the source three minutes NEWER than its object, containing an edit,
    and only the .ino was recompiled. The stale object then linked, so a fix looked like it had
    not worked - and in a fleet run, stale code would have shipped.

    So the content hash goes into the staged ini, where it feeds project.checksum (T2 measured
    that extra_configs contents do). Change the library and every staged build directory is
    invalidated; leave it alone and builds stay warm.
    """
    if root in _FINGERPRINT:
        return _FINGERPRINT[root]
    digest = hashlib.sha256()
    for part in LIBRARY_PARTS:
        path = os.path.join(root, part)
        if os.path.isfile(path):
            digest.update(open(path, "rb").read())
        elif os.path.isdir(path):
            for base, dirs, files in os.walk(path):
                dirs.sort()
                for name in sorted(files):
                    if not name.startswith("."):
                        full = os.path.join(base, name)
                        digest.update(os.path.relpath(full, root).encode())
                        digest.update(open(full, "rb").read())
    _FINGERPRINT[root] = digest.hexdigest()[:16]
    return _FINGERPRINT[root]


# Set by main() from --library / --project. None means "the library this script is in".
LIBRARY_ROOT = None

# example name -> its directory. Filled by main(); an example not in it is under examples/.
EXAMPLE_DIRS = {}


def library_root():
    if LIBRARY_ROOT:
        return LIBRARY_ROOT
    return os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir))


def example_dir_of(example):
    return EXAMPLE_DIRS.get(example) or os.path.join(default_examples_dir(), example)


def unstaged(name, path, stage):
    """Is this entry of a project's directory to be left out of its staged copy?

      .pio, .git and other dot-entries   another build's output, and the repository
      lib                                lib_dir is pointed elsewhere by the staged ini, so a
                                         lib/Frugal-IoT symlink here would only be walked as
                                         project source
      *-local.ini                        a developer's overrides - blanked lib_deps, debug flags.
                                         The one that belongs in a release is --secrets, staged
                                         separately under SECRETS_NAME.
      the staging tree itself            ./ota-stage inside a --project checkout. Linking it would
                                         put the staged project inside itself, and "src_dir = ."
                                         would then compile it all again.
    """
    real = os.path.realpath(path)
    return (name.startswith(".") or name == "lib" or name.endswith("-local.ini")
            or real == stage or stage.startswith(real + os.sep))


def pio_binary():
    """The pio next to the interpreter we are running under (we re-exec into PlatformIO's venv)."""
    candidate = os.path.join(os.path.dirname(sys.executable), "pio")
    if not os.path.exists(candidate):
        candidate = shutil.which("pio")
    return candidate


def git_state(repo):
    """(short sha, dirty) for the library, so a binary can be traced back to a tree.

    Note examples/temp/ is deliberately ignored via a global gitignore, so work in progress there
    does not make the tree look dirty.
    """
    sha, dirty = "unknown", False
    try:
        sha = subprocess.run(["git", "-C", repo, "rev-parse", "--short", "HEAD"],
                             capture_output=True, text=True, check=True).stdout.strip()
        status = subprocess.run(["git", "-C", repo, "status", "--porcelain"],
                                capture_output=True, text=True, check=True).stdout.strip()
        dirty = bool(status)
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass
    return sha, dirty


def link(target, linkname):
    """Symlink, but leave an already-correct link alone.

    Not just tidiness. project.checksum hashes the MERGED config, so a secrets file whose content
    changes invalidates every build directory for that example. Pointing at the master rather
    than copying it means the content only changes when the master really changes - so repeat
    runs stay incremental instead of triggering a full rebuild each time.
    """
    if os.path.islink(linkname):
        if os.readlink(linkname) == target:
            return
        os.unlink(linkname)
    elif os.path.exists(linkname):
        if os.path.isdir(linkname):
            shutil.rmtree(linkname)
        else:
            os.unlink(linkname)
    os.symlink(target, linkname)


def stage_project(stage, mode, example, secrets):
    """Create (or refresh) the staging project for one example, and return its path."""
    root = library_root()
    example_dir = example_dir_of(example)
    project = os.path.join(stage, "build", mode, example)
    os.makedirs(project, exist_ok=True)

    wanted = set()
    real_stage = os.path.realpath(stage)
    for name in sorted(os.listdir(example_dir)):
        source = os.path.join(example_dir, name)
        if not unstaged(name, source, real_stage):
            link(source, os.path.join(project, name))
            wanted.add(name)

    if secrets:
        link(os.path.abspath(secrets), os.path.join(project, SECRETS_NAME))
        wanted.add(SECRETS_NAME)

    # lib_dir goes OUTSIDE the project, as a sibling directory, and is pointed at by a staged ini.
    #
    # It cannot be the usual <project>/lib, because every example sets "src_dir = ." - so the
    # project root IS the source directory, and PlatformIO walks lib/ as project source. The
    # library then gets compiled TWICE: once as src/lib/Frugal-IoT/src/*.cpp.o and once as
    # lib<hash>/Frugal-IoT/*.cpp.o. On ESP32 that is a hard duplicate-symbol link failure. On
    # ESP8266 it LINKS - lib_archive defaults true there, so the second copy sits in an archive
    # the linker never opens - and quietly compiles 64 files for nothing. A green ESP8266 build
    # is therefore no evidence that this is right; check the ESP32 one.
    stale = os.path.join(project, "lib")  # where an earlier version of this script put it
    if os.path.isdir(stale) and not os.path.islink(stale):
        shutil.rmtree(stale)

    libdir = os.path.join(stage, "build", mode, example + ".lib")
    frugal = os.path.join(libdir, "Frugal-IoT")
    os.makedirs(libdir, exist_ok=True)
    if mode == "local":
        os.makedirs(frugal, exist_ok=True)
        for part in LIBRARY_PARTS:
            source = os.path.join(root, part)
            if os.path.exists(source):
                link(source, os.path.join(frugal, part))
    elif os.path.exists(frugal):
        # Switching to registry mode must actually remove the local library, or lib_deps is still
        # satisfied locally and "registry" silently means "working tree".
        shutil.rmtree(frugal)

    # Matches the "*-local.ini" glob every example already has in extra_configs, so this needs no
    # edit to any platformio.ini. Written only when it differs - it feeds project.checksum, and
    # rewriting it every run would wipe the build directory every run.
    stage_ini = os.path.join(project, "platformio-stage-local.ini")
    content = ("; Written by ota_build.py - see stage_project(). Do not edit.\n"
               "[platformio]\n"
               f"lib_dir = {libdir}\n")
    if mode == "local":
        # Not read by anything - it is here purely so that changing the library changes
        # project.checksum, which is what forces a rebuild. See library_fingerprint().
        content += ("\n[env]\n"
                    f"custom_frugal_lib_fingerprint = {library_fingerprint(root)}\n")
    if not (os.path.exists(stage_ini) and open(stage_ini).read() == content):
        with open(stage_ini, "w") as handle:
            handle.write(content)
    wanted.add(os.path.basename(stage_ini))

    # Drop links to example files that have since been deleted or renamed, so a stale source file
    # cannot go on being compiled.
    for name in os.listdir(project):
        if name not in wanted and name != ".pio":
            path = os.path.join(project, name)
            if os.path.islink(path):
                os.unlink(path)

    return project


def build_record(name, kind, entry, mode, sha, dirty, firmware, secrets):
    fingerprint = library_fingerprint(library_root()) if mode == "local" else "registry"
    data = open(firmware, "rb").read()
    record = {
        # "key" publishes to <org>/<project|+>/<otakey>/; "node" to <org>/<project|+>/<nodeid>/,
        # which the server checks first. ota_upload.py reads this to know which it is.
        "target": kind,
        "otakey": name if kind == "key" else "-",
        "target_node": name if kind == "node" else "-",
        "example": entry.example,
        "env": entry.env,
        "ota_prefix": entry.prefix,
        "ota_suffix": entry.suffix,
        "lib_mode": mode,
        # False means --secrets was omitted. Such a binary looks identical to a good one in the
        # staging tree, and the node it reaches cannot re-enrol after a factory reset - so record
        # it here where ota_upload.py can refuse to publish it by accident.
        "enrol_secret": bool(secrets),
        "lib_git_sha": sha + ("-dirty" if dirty else ""),
        # Which library CONTENT this was built from. lib_git_sha is not enough during a fix-and-
        # rerun cycle - it stays "<sha>-dirty" across every edit, so it cannot tell a binary built
        # before a fix from one built after. This can. See --skip-built.
        "lib_fingerprint": fingerprint,
        "md5": hashlib.md5(data).hexdigest(),
        "size": len(data),
        "built_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
    }
    if entry.example in EXAMPLE_DIRS:
        # A --project build: the application's own commit is what identifies it, and the library
        # sha above says little when the library came from the registry.
        project_sha, project_dirty = git_state(EXAMPLE_DIRS[entry.example])
        record["project_git_sha"] = project_sha + ("-dirty" if project_dirty else "")
    return record


def build_one(stage, mode, entry, targets, secrets, sha, dirty, verbose):
    """Build one (example, env) and publish it under every key/node that maps to it.

    targets is a set of (name, kind) where kind is "key" or "node".
    """
    project = stage_project(stage, mode, entry.example, secrets)
    shown = ", ".join(n if k == "key" else f"node:{n}" for n, k in sorted(targets))
    quiet = "" if verbose else "  [output suppressed until it finishes; --verbose to stream]"
    print(f"  building {entry.example}/{entry.env} -> {shown}{quiet}", flush=True)

    command = [pio_binary(), "run", "-e", entry.env]
    if verbose:
        result = subprocess.run(command, cwd=project)
        output = ""
    else:
        result = subprocess.run(command, cwd=project, capture_output=True, text=True)
        output = result.stdout + result.stderr

    built = None
    if result.returncode != 0:
        if output:
            print(output)
        print(f"FAILED {entry.example}/{entry.env}", file=sys.stderr)
    else:
        # Belt and braces for the above: if a registry copy got installed, the local library was
        # not used, whatever the mode says. Refusing here is the difference between shipping the
        # wrong firmware and being told about it.
        intruder = os.path.join(project, ".pio", "libdeps", entry.env, "Frugal-IoT")
        firmware = os.path.join(project, ".pio", "build", entry.env, "firmware.bin")
        if mode == "local" and os.path.exists(intruder):
            print(f"FAILED {entry.example}/{entry.env}: --lib local, but PlatformIO installed a "
                  f"registry copy at\n  {intruder}\nso the working tree was NOT used. Delete "
                  f"the staging tree and re-run.", file=sys.stderr)
        elif not os.path.exists(firmware):
            print(f"FAILED {entry.example}/{entry.env}: no firmware.bin at {firmware}",
                  file=sys.stderr)
        else:
            for name, kind in sorted(targets):
                record = build_record(name, kind, entry, mode, sha, dirty, firmware,
                                      secrets)
                out = os.path.join(stage, "firmware", name)
                os.makedirs(out, exist_ok=True)
                shutil.copy2(firmware, os.path.join(out, "firmware.bin"))
                with open(os.path.join(out, "build.json"), "w") as handle:
                    json.dump(record, handle, indent=2)
                    handle.write("\n")
            print(f"    ok {record['size']:,} bytes  md5 {record['md5'][:12]}...", flush=True)
            built = record
    return built


def read_manifest(path):
    """Pull the otakeys marked for building out of the step-1 TSV.

    Tolerant of column order and of extra columns, because the file is meant to be edited by hand
    (and in a spreadsheet, which likes to reorder things).
    """
    wanted = []
    with open(path) as handle:
        header = None
        for line in handle:
            line = line.rstrip("\n")
            if line.strip() and not line.lstrip().startswith("#"):
                fields = line.split("\t")
                if header is None:
                    header = [f.strip() for f in fields]
                else:
                    row = dict(zip(header, [f.strip() for f in fields]))
                    if row.get("build", "").lower() in ("y", "yes", "true", "1"):
                        wanted.append(row)
    return wanted


def report_stale(stage, mode):
    """Name any staged binary built from a different library than the one on disk now.

    The point of --skip-built is to not rebuild what already worked, which necessarily means some
    binaries predate the fix that made the later ones build. That is fine while iterating and NOT
    fine to upload, so it is reported every run rather than left to be remembered.
    """
    current = library_fingerprint(library_root()) if mode == "local" else "registry"
    stale = []
    firmware = os.path.join(stage, "firmware")
    if os.path.isdir(firmware):
        for name in sorted(os.listdir(firmware)):
            record = os.path.join(firmware, name, "build.json")
            if os.path.exists(record):
                try:
                    data = json.load(open(record))
                except ValueError:
                    continue
                if data.get("lib_fingerprint", "unknown") != current:
                    stale.append(name)
    if stale:
        print(f"\nWARNING: {len(stale)} staged binary/binaries were built from a DIFFERENT "
              f"library than the\none on disk now. Re-run without --skip-built before uploading:")
        for name in stale[:10]:
            print(f"  {name}")
        if len(stale) > 10:
            print(f"  ... and {len(stale) - 10} more")
    return stale


def main():
    parser = argparse.ArgumentParser(
        description="Build firmware for OTA keys into a staging tree.",
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--key", action="append", default=[], metavar="OTAKEY",
                        help="an OTA key to build; repeatable")
    parser.add_argument("--manifest", metavar="TSV",
                        help="ota_keys.tsv from step 1; builds every row marked build=y")
    parser.add_argument("--all", action="store_true",
                        help="build every OTA key found - for --project, every env it has")
    parser.add_argument("--project", action="append", default=[], metavar="DIR",
                        help="build this standalone project instead of the library's examples; "
                             "repeatable")
    parser.add_argument("--library", metavar="DIR",
                        help="the library for --lib local (default: the one this script is in, "
                             "or else the first --project's lib/Frugal-IoT)")
    parser.add_argument("--stage", default=os.path.join(os.getcwd(), "ota-stage"),
                        help="staging tree (default: ./ota-stage)")
    parser.add_argument("--lib", choices=("local", "registry"), default="local",
                        help="build against the working tree (default) or the published library")
    parser.add_argument("--secrets", metavar="INI",
                        help=f"master {SECRETS_NAME} to link into each build")
    parser.add_argument("--force", action="store_true",
                        help="build --lib local from a dirty working tree anyway")
    parser.add_argument("--verbose", action="store_true", help="stream pio output as it builds")
    parser.add_argument("--list", action="store_true", help="resolve and list, but do not build")
    parser.add_argument("--skip-built", action="store_true",
                        help="skip anything already in the staging tree. For the fix-and-rerun "
                             "cycle: a library fix invalidates every build dir, so without this "
                             "each failure costs a full rebuild of everything already done. The "
                             "skipped binaries are then built against the OLD library, so finish "
                             "with one plain run to reconcile - it reports what is stale.")
    args = parser.parse_args()
    # MUST be absolute. lib_dir goes into the staged ini, and PlatformIO resolves a relative one
    # against the PROJECT directory - so "--stage ./ota-stage" pointed lib_dir at nothing, no
    # local library was found, and lib_deps was quietly satisfied from the registry instead.
    # "--lib local" then silently built the PUBLISHED library.
    args.stage = os.path.abspath(args.stage)

    global LIBRARY_ROOT
    projects = [os.path.abspath(p) for p in args.project]
    if args.library:
        LIBRARY_ROOT = os.path.realpath(args.library)
    elif projects and not os.path.exists(os.path.join(library_root(), "library.json")):
        # This script is a copy inside an application, not in the library - so the library is
        # the one the application builds against locally.
        LIBRARY_ROOT = os.path.realpath(os.path.join(projects[0], "lib", "Frugal-IoT"))
    if args.lib == "local" and not os.path.exists(os.path.join(library_root(), "library.json")):
        sys.exit(f"--lib local, but there is no library at {library_root()}.\n"
                 "Give --library, symlink one at lib/Frugal-IoT, or use --lib registry.")

    if projects:
        for p in projects:
            EXAMPLE_DIRS[os.path.basename(p)] = p
        keymap, problems = scan_projects(projects, lint_names=False)
    else:
        keymap, problems = scan(default_examples_dir())
    if projects and args.lib == "registry":
        # The library came from the registry, and the sources from the project - so whatever
        # lib/Frugal-IoT happens to point at played no part, and recording its "-dirty" would
        # have ota_upload.py refuse a build that did not use it. project_git_sha is the record.
        sha, dirty = "registry", False
    else:
        sha, dirty = git_state(library_root())

    requested = [{"otakey": k} for k in args.key]
    if args.all:
        requested.extend({"otakey": k} for k in sorted(keymap))
    if args.manifest:
        requested.extend(read_manifest(args.manifest))
    if not requested:
        parser.error("nothing to build - give --key, --all and/or --manifest")

    # (example, env) -> Entry, so a row that names an example and env but no key can be resolved.
    # That is how a node gets firmware pinned to it: the server checks ['+', node] BEFORE
    # [project, attribs], so a node-targeted binary outranks whatever its OTA key would serve -
    # which is the only way to reach a node that reports no ota/key at all.
    by_example_env = {(e.example, e.env): e for e in keymap.values()}

    # Resolve all of it before building anything. A row that does not resolve is a question for a
    # human (renamed? missing alias?), and finding that out after a forty minute run is no use.
    groups = {}
    unresolved = []
    for row in requested:
        otakey = (row.get("otakey") or "-").strip()
        target = (row.get("target_node") or "-").strip()
        example = (row.get("example") or "-").strip()
        env = (row.get("env") or "-").strip()

        # The manifest's example/env columns are AUTHORITATIVE where present, because ota_keys.py
        # has already resolved any alias into them. An aliased row's otakey is deliberately the
        # OLD key - the one the node still asks for - and looking that up here would fail, since
        # by definition no example builds it any more. Alias knowledge lives in one place
        # (ota_keys.py + ota_aliases.tsv), not in both scripts.
        if example != "-" and env != "-":
            entry = by_example_env.get((example, env))
            if entry is None:
                unresolved.append(f"{example}/{env}: no such example and env"
                                  f"{f' (for key {otakey})' if otakey != '-' else ''}")
            elif otakey != "-":
                targets = {(otakey, "key")}
                # Also publish under the key this build actually reports, so a node that takes
                # the update - and therefore starts reporting the new key - is already served.
                # Only for key-targeted rows: doing it for a node-pinned row would push that
                # node's special firmware to every other node sharing its key.
                targets.add((f"{entry.prefix}_{entry.suffix}", "key"))
                groups.setdefault((entry.example, entry.env), (entry, set()))[1].update(targets)
            elif target != "-":
                groups.setdefault((entry.example, entry.env), (entry, set()))[1].add(
                    (target, "node"))
            else:
                unresolved.append(f"row with neither otakey nor target_node: {row}")
        elif otakey != "-":
            # No example/env - a bare --key, so look it up.
            entry = keymap.get(otakey)
            if entry is None:
                unresolved.append(f"{otakey} (no example builds this key)")
            else:
                groups.setdefault((entry.example, entry.env), (entry, set()))[1].add(
                    (otakey, "key"))
        elif target != "-":
            unresolved.append(
                f"node {target}: needs a valid example and env to build from"
                f" (got {example!r}/{env!r})")
        else:
            unresolved.append(f"row with neither otakey nor target_node: {row}")

    if unresolved:
        print("Nothing was built. These rows could not be resolved:", file=sys.stderr)
        for problem in unresolved:
            print(f"  {problem}", file=sys.stderr)
        print("\nAn OTA key that no example builds is usually a rename: add it to "
              "ota_aliases.tsv\nand re-run ota_keys.py. A row pinned to a node needs the example "
              "and env columns\nfilled in by hand, since there is no key to look them up from.",
              file=sys.stderr)
        sys.exit(2)

    # Both of these are about what gets COMPILED, so neither applies to --list, which only
    # resolves keys and touches nothing.
    if not args.list:
        if args.lib == "local" and dirty and not args.force:
            sys.exit(f"Library working tree {library_root()} is dirty.\n"
                     "A binary built from uncommitted changes cannot be reproduced later.\n"
                     "Commit, stash, or pass --force (which records the sha as '-dirty').")
        # Whatever --lib says: a project is always built from its working tree.
        for p in projects:
            if git_state(p)[1] and not args.force:
                sys.exit(f"Project working tree {p} is dirty.\n"
                         "A binary built from uncommitted changes cannot be reproduced later.\n"
                         "Commit, stash, or pass --force (which records the sha as '-dirty').")

        if not args.secrets:
            print("WARNING: no --secrets given, so the firmware carries no enrolment secret.\n"
                  "         A node that is already enrolled is fine; a factory-reset one cannot "
                  "re-enrol.", file=sys.stderr)

    # Skip whole groups whose outputs are all present. Per group, not per target, because one
    # build serves every key and node that maps to it.
    skipped = []
    if args.skip_built:
        keep = {}
        for slot, (entry, targets) in groups.items():
            done = all(os.path.exists(os.path.join(args.stage, "firmware", n, "firmware.bin"))
                       for n, _ in targets)
            if done:
                skipped.append(f"{entry.example}/{entry.env}")
            else:
                keep[slot] = (entry, targets)
        groups = keep

    print(f"{len(groups)} build(s), --lib {args.lib}, library {sha}"
          f"{'-dirty' if dirty else ''}")
    if skipped:
        print(f"  skipping {len(skipped)} already built: {', '.join(sorted(skipped))}")
    if problems:
        keymap_script = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ota_keymap.py")
        print(f"({len(problems)} keymap lint problem(s) - to list them: "
              f"{os.path.relpath(keymap_script)} --problems)")

    if args.list:
        for (example, env), (entry, targets) in sorted(groups.items()):
            shown = " ".join(n if k == "key" else f"node:{n}" for n, k in sorted(targets))
            print(f"  {example}/{env}\t{shown}")
    else:
        done = 0
        for (example, env), (entry, targets) in sorted(groups.items()):
            record = build_one(args.stage, args.lib, entry, targets, args.secrets, sha, dirty,
                               args.verbose)
            if record is None:
                # Stop at the first failure, by decision: while this harness is new, a clean halt
                # beats a plausible-looking summary that has to be audited afterwards.
                print(f"\nStopped after {done} successful build(s). "
                      f"Staging tree left at {args.stage}", file=sys.stderr)
                sys.exit(1)
            done += 1
        print(f"\n{done} build(s) ok -> {os.path.join(args.stage, 'firmware')}")
        report_stale(args.stage, args.lib)


if __name__ == "__main__":
    reexec_under_platformio(__file__)
    main()
