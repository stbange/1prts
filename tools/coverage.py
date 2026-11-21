"""Measure unit coverage in isolation. Usage: python3 tools/coverage.py BUILD [--luac luac5.4]."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def run(command, **kwargs):
    return subprocess.run(command, check=True, **kwargs)

def native_coverage(build, temp):
    counts = {}
    for notes in build.rglob("*.gcno"):
        dest = temp / notes.relative_to(notes.anchor)
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(notes, dest)
        result = run(["gcov", "--json-format", "--stdout", str(dest)], capture_output=True)
        for item in json.loads(result.stdout)["files"]:
            source = Path(item["file"]).resolve()
            if source.parent != ROOT / "src":
                continue
            lines = counts.setdefault(source, {})
            for record in item["lines"]:
                line = record["line_number"]
                lines[line] = lines.get(line, False) or record["count"] > 0
    missing = set((ROOT / "src").glob("*.c")) - counts.keys()
    if missing:
        raise RuntimeError(f"Uninstrumented production sources: {missing}. Configure with ORRA_COVERAGE=ON.")
    return counts

def lua_coverage(luac, hitfile):
    hits = set(hitfile.read_text().splitlines())
    result = {}
    for script in (ROOT / "scripts").rglob("*.lua"):
        code = run([luac, "-p", "-l", str(script)], text=True, capture_output=True).stdout
        result[script] = {int(n): f"{script.relative_to(ROOT / 'scripts').as_posix()} {n}" in hits
                          for n in re.findall(r"^\s*\d+\s+\[(\d+)\]", code, re.MULTILINE)}
    return result

def threshold(label, inventory):
    total = sum(len(lines) for lines in inventory.values())
    hit = sum(sum(lines.values()) for lines in inventory.values())
    print(f"{label}: {hit}/{total} executable lines ({100*hit/max(1,total):.2f}%)")
    if not total or hit * 100 < total * 95:
        for src,lines in inventory.items():
            print(src.name, "missed", [n for n,b in lines.items() if not b])
        raise SystemExit(f"{label} is below 95%")

if __name__ == "__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("build",type=Path)
    parser.add_argument("--luac", default=shutil.which("luac5.4") or "luac")
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="orra-unit-") as folder:
        temp=Path(folder)
        env=dict(os.environ,GCOV_PREFIX=str(temp),GCOV_PREFIX_STRIP="0",ORRA_LUA_HITS=str(temp/"lua.hits"))
        run(["ctest","--test-dir",str(args.build.resolve()),"-R","^(cli|engine|lua|headless)_units$","--output-on-failure"],env=env)
        threshold("First-party C (compiled native configuration)",native_coverage(args.build.resolve(),temp))
        threshold("Shipped Lua",lua_coverage(args.luac,temp/"lua.hits"))
    run(["node","--test","--experimental-test-coverage","--test-coverage-include=web/*.js",
         "--test-coverage-lines=95","tests/web-unit.cjs"],cwd=ROOT)
