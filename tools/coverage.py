"""Measure unit coverage in isolation. Usage: python3 tools/coverage.py BUILD [--luac luac5.4]."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def run(command, **kwargs):
    return subprocess.run(command, check=True, **kwargs)

def native_coverage(build, temp):
    counts = {}
    notes_files = set()
    for item in json.loads((build / "compile_commands.json").read_text()):
        if Path(item["file"]).resolve().parent != ROOT / "src":
            continue
        argv = item.get("arguments") or shlex.split(item["command"])
        obj = item.get("output") or argv[argv.index("-o") + 1]
        notes_files.add((Path(item["directory"]) / obj).with_suffix(".gcno"))
    for notes in notes_files:
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

def with_target_adapters(inventory):
    """Conservative union. Count even continued expressions in unmeasured target-only C blocks."""
    result={path:dict(lines) for path,lines in inventory.items()}
    for path,lines in result.items():
        fallback=False
        platforms=[]
        for n,raw in enumerate(path.read_text().splitlines(),1):
            line=raw.strip()
            if re.match(r"#if(?:def)?\b",line):
                target=bool(re.search(r"\b(?:__EMSCRIPTEN__|__APPLE__|_WIN32)\b",line))
                platforms.append((fallback,target))
                fallback=fallback or target
            elif line.startswith("#elif") and platforms:
                fallback=platforms[-1][0] or bool(re.search(r"__APPLE__|__EMSCRIPTEN__|_WIN32",line))
            elif line.startswith("#else") and platforms:
                fallback=platforms[-1][0]
            elif line.startswith("#endif") and platforms:
                fallback=platforms.pop()[0]
            elif fallback and line and not line.startswith(("#","//","/*","*")) and line not in ("{","}"):
                lines.setdefault(n,False)
    return result

def lua_coverage(luac, hitfile):
    hits = set(hitfile.read_text().splitlines())
    result = {}
    for script in [*(ROOT / "scripts").rglob("*.lua"), *(ROOT / "mods").rglob("*.lua")]:
        key = script.relative_to(ROOT / "scripts" if script.is_relative_to(ROOT / "scripts") else ROOT).as_posix()
        code = run([luac, "-p", "-l", str(script)], text=True, capture_output=True).stdout
        result[script] = {int(n): f"{key} {n}" in hits
                          for n in re.findall(r"^\s*\d+\s+\[(\d+)\]", code, re.MULTILINE)}
    return result

def threshold(label, inventory):
    total = sum(len(lines) for lines in inventory.values())
    hit = sum(sum(lines.values()) for lines in inventory.values())
    print(f"{label}: {hit}/{total} executable lines ({100*hit/max(1,total):.2f}%)")
    if not total or hit * 100 < total * 95:
        for src,lines in inventory.items():
            print(src.name, "missed", [n for n,b in lines.items() if not b])
        print(f"{label} is below 95%")
        return False
    return True

if __name__ == "__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("build",type=Path)
    parser.add_argument("--luac", default=shutil.which("luac5.4") or "luac")
    parser.add_argument("--report",type=Path,help="write per-source unit line counts and uncovered lines")
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="orra-unit-") as folder:
        temp=Path(folder)
        env=dict(os.environ,GCOV_PREFIX=str(temp),GCOV_PREFIX_STRIP="0",ORRA_LUA_HITS=str(temp/"lua.hits"))
        run(["ctest","--test-dir",str(args.build.resolve()),"-R","^(cli|engine|lua|headless)_units$","--output-on-failure"],env=env)
        native = native_coverage(args.build.resolve(),temp)
        threshold("C in the measured native configuration",native)
        union=with_target_adapters(native)
        native_ok=threshold("First-party C including unmeasured platform adapters",union)
        lua=lua_coverage(args.luac,temp/"lua.hits")
        lua_ok=threshold("Shipped Lua",lua)
        if args.report:
            def per_file(inventory):
                return {p.relative_to(ROOT).as_posix():{"hit":sum(lines.values()),"total":len(lines),
                    "uncovered":sorted(n for n,v in lines.items() if not v)} for p,lines in sorted(inventory.items())}
            args.report.write_text(json.dumps({"native":per_file(native),"all_c":per_file(union),
                "shipped_lua":per_file(lua)},indent=2)+"\n")
    browser = subprocess.run(["node","--test","--experimental-test-coverage","--test-coverage-include=web/*.js",
         "--test-coverage-lines=95","tests/web-unit.cjs"],cwd=ROOT)
    if not (native_ok and lua_ok and browser.returncode == 0):
        raise SystemExit(1)
