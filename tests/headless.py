"""Exercise the real simulator CLI, loose packs, both ferry landings and clock boundaries."""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

binary = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as folder:
    temp = Path(folder)
    def call(*args, route=None, ok=True):
        result = subprocess.run([str(binary), *map(str, args)], input=route, text=True,
                                capture_output=True, cwd=temp)
        assert (result.returncode == 0) == ok, (args, result.stdout, result.stderr)
        return result.stdout
    assert len(call("--list").splitlines()) == 8
    call("--help")
    call("--unknown",ok=False)
    call("--mission", ok=False)
    call("--mission", "absent", ok=False)
    call("--replay", temp/"absent", ok=False)
    for instruction in ("wait -1","wait 120000","wait six","order bogus","invalid","result victory","alloy 0"):
        call("--replay","-",route=instruction,ok=False)
    for gear, landing, tick, alloy in [("hoist","quay",435*30,2),("ramp","slip",575*30,2),("load","quay",660*30,8)]:
        route=temp/"ferry.txt"
        route.write_text(f"# port dispatch\n\nreject slip\norder {gear}\norder {landing}\norder install\nwait {tick-1}\nresult active\nwait 1\nresult victory\nalloy {alloy}\n")
        assert f"tick={tick}" in call("--mission","tide-window","--replay",route)
    call("--mission","tide-window","--replay","-",route="wait 21600\nresult defeat")
    root=temp/"campaign"
    root.mkdir()
    for name in ("scripts","assets","mods"):
        shutil.copytree(binary.parent/name,root/name)
    base=json.loads((binary.parent/"content.json").read_text())
    base["packs"]=["mods/long-watch"]
    (root/"content.json").write_text(json.dumps(base))
    assert "hold=2700" in call("--data",root)
    assert "hold=27000" in call("--data",root,"--mission","tide-window")
    call("--data",root,"--replay","-",route="order mill\nalloy 4\norder cancel\nalloy 8")
