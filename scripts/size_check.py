#!/usr/bin/env python3
"""
Catch flash-size creep.

Builds the tracked envs, compares each against a recorded baseline, and complains if one has grown
more than the allowed margin. Run it before a release, or whenever a change might have pulled
something large in - which is the case this whole exercise exists to guard against, since
LoRaMesher's std::ostringstream cost 230,464 bytes without anyone noticing.

    lib/Frugal-IoT/scripts/size_check.py                 # check every env in the baseline
    lib/Frugal-IoT/scripts/size_check.py c3_pico         # just one
    lib/Frugal-IoT/scripts/size_check.py --margin 1      # stricter than the default 2%
    lib/Frugal-IoT/scripts/size_check.py --update        # re-record the baseline after an intended change
    lib/Frugal-IoT/scripts/size_check.py --list          # show the baseline without building

Baseline lives in size_baseline.json at the root of your project. Update it deliberately, with a
note saying why -
an unexplained jump in that file is exactly what this is meant to make visible.

Sizes are the ELF's allocated sections, not firmware.bin. The two differ by a constant ~150-byte
header so trends are identical, and going via the ELF avoids esptool, which breaks on this machine
often enough to be a nuisance.
"""

import argparse
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from size_report import elf_alloc, find_project                      # noqa: E402

PROJECT = find_project()
# Per project, not per library - it records this project's env names and their sizes, so it lives
# with the project even though the script itself ships with Frugal-IoT.
BASELINE = PROJECT / "size_baseline.json"


def image_bytes(elf: Path) -> int:
    return sum(size for size, nb in elf_alloc(elf).values() if not nb)


def build(env: str):
    """Build one env's ELF. Returns (bytes, error or None)."""
    elf = PROJECT / ".pio" / "build" / env / "firmware.elf"
    if elf.exists():
        elf.unlink()
    r = subprocess.run(["pio", "run", "-e", env, "-t", str(elf)],
                       cwd=PROJECT, capture_output=True, text=True)
    # Judged on the ELF, not the exit code: the .bin step that follows needs esptool and fails
    # independently of anything in the code.
    if not elf.exists():
        errs = [l for l in (r.stdout + r.stderr).splitlines() if "error" in l.lower()]
        return None, (errs[0][:160] if errs else "build produced no ELF")
    return image_bytes(elf), None


def load():
    if not BASELINE.exists():
        return {"envs": {}, "note": ""}
    return json.loads(BASELINE.read_text())


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("envs", nargs="*")
    ap.add_argument("--margin", type=float, default=2.0, help="percent growth allowed (default 2)")
    ap.add_argument("--update", action="store_true", help="re-record the baseline")
    ap.add_argument("--note", default=None, help="with --update, why it changed")
    ap.add_argument("--list", action="store_true")
    args = ap.parse_args()

    data = load()
    tracked = data["envs"]

    if args.list:
        print(f"baseline: {BASELINE}")
        if data.get("note"):
            print(f"note: {data['note']}")
        for env, rec in sorted(tracked.items()):
            cap = rec.get("partition")
            pct = f"   {100 * rec['bytes'] / cap:5.1f}% of {cap:,}" if cap else ""
            print(f"  {env:<26}{rec['bytes']:>12,}{pct}")
        return

    envs = args.envs or sorted(tracked) or ["c3_pico"]
    results, worst, failed = {}, 0.0, []

    for env in envs:
        print(f"{env:<26}", end="", flush=True)
        size, err = build(env)
        if size is None:
            print(f"BUILD FAILED - {err}")
            failed.append(env)
            continue
        rec = tracked.get(env)
        results[env] = size
        if not rec:
            print(f"{size:>12,}   (not in baseline)")
            continue
        was = rec["bytes"]
        delta = size - was
        pct = 100.0 * delta / was if was else 0.0
        worst = max(worst, pct)
        flag = "  <-- OVER MARGIN" if pct > args.margin else ""
        cap = rec.get("partition")
        head = f"   {100 * size / cap:5.1f}% of partition" if cap else ""
        print(f"{size:>12,}{delta:>+9,} ({pct:+.2f}%){head}{flag}")

    if args.update:
        for env, size in results.items():
            rec = tracked.setdefault(env, {})
            rec["bytes"] = size
        if args.note:
            data["note"] = args.note
        data["envs"] = tracked
        BASELINE.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n")
        print(f"\nbaseline updated: {BASELINE}")
        return

    if failed:
        sys.exit(f"\n{len(failed)} env(s) did not build: {', '.join(failed)}")
    if worst > args.margin:
        sys.exit(f"\nFlash grew more than {args.margin}% - find out what pulled it in before "
                 f"releasing.\nscripts/size_report.py <env> --top 30 will show you where it went.")
    print(f"\nAll within {args.margin}%.")


if __name__ == "__main__":
    main()
