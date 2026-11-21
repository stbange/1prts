"""Compare the same ordinary input schedule, complete state and modded frames on one native executable."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

binary=Path(sys.argv[1]).resolve()
repo=Path(__file__).resolve().parent
def checksum(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
with tempfile.TemporaryDirectory() as directory:
    temp=Path(directory)
    def run(sector, rate, data=binary.parent, pack=False, capture=False, route_name=None):
        route_name=route_name or sector
        output=temp/(route_name+str(rate)+str(pack));output.mkdir(exist_ok=True)
        rows=["rate "+str(rate)]
        for line in (repo/(route_name+".route")).read_text().splitlines():
            if line.startswith("rate "): continue
            if line.startswith("frames "): line="frames "+str(rate*12//30)
            if line=="expect result victory" and pack:
                rows.extend(["expect result active","wait 4100"])
            rows.append(line)
            if line.startswith(("wait ","hold ","key ","frames ")): rows.append("checkpoint "+str(len(rows)))
        rows.append("checkpoint complete")
        route=output/"schedule.route";route.write_text("\n".join(rows))
        env=dict(os.environ,ORRA_COMPACT_REPLAY="1")
        env.pop("ORRA_CAPTURE_DIR",None)
        if capture: env["ORRA_CAPTURE_DIR"]=str(output)
        p=subprocess.run([str(binary),"--mission",sector,"--data",str(data),"--replay",str(route)],cwd=temp,env=env,text=True,capture_output=True)
        assert p.returncode==0,(sector,rate,p.stderr,p.stdout)
        checks=[line for line in p.stdout.splitlines() if line.startswith("checkpoint")]
        assert checks and "victory" in checks[-1],checks
        return checks,output
    executable=checksum(binary)
    base_frame=None
    for sector in ("gate-six","tide-window"):
        first,output=run(sector,30,capture=sector=="gate-six")
        second,_=run(sector,90)
        assert first==second,(sector,first,second)
        if sector=="gate-six":base_frame=checksum(output/"03-armor-yard.ppm")
    first,_=run("gate-six",30,route_name="armor-first")
    second,_=run("gate-six",90,route_name="armor-first")
    assert first==second
    data=temp/"loose";data.mkdir()
    for folder in ("assets","scripts","mods"):
        shutil.copytree(binary.parent/folder,data/folder)
    manifest=json.loads((binary.parent/"content.json").read_text())
    manifest["packs"]=["mods/long-watch"];(data/"content.json").write_text(json.dumps(manifest))
    checks,output=run("gate-six",30,data,True,True)
    assert checksum(output/"03-armor-yard.ppm")!=base_frame
    assert checksum(binary)==executable
    print("Gate Six and Tide Window agree at every checkpoint at 30 and 90 Hz. Loose pack verified.")
