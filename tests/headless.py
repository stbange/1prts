"""Exercise the real simulator CLI, loose packs, both ferry landings and clock boundaries."""
import json
from pathlib import Path
import shutil
import subprocess
import struct
import sys
import tempfile

binary = Path(sys.argv[1]).resolve()
def hull_profile(name):
    data=(binary.parent/"assets/models"/(name+".mesh")).read_bytes()
    points=list(struct.iter_unpack("<6f4B",data[8:]))
    lo=min(v[2] for v in points);length=max(v[2] for v in points)-lo
    profile=[0.0]*20
    for vertex in points:
        column=min(19,int((vertex[2]-lo)*20/length))
        profile[column]=max(profile[column],vertex[1]/length)
    return length,profile

lintel_length,lintel=hull_profile("lintel");wake_length,wake=hull_profile("wake")
assert 11.8<lintel_length<12.2 and 14.8<wake_length<15.2
assert sum(abs(a-b)>.04 for a,b in zip(lintel,wake))>=7, "Armor silhouette depends on color"
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
    stress=temp/"large"
    stress.mkdir()
    for name in ("assets","scripts","mods"): shutil.copytree(root/name,stress/name)
    extension=stress/"mods"/"yard"
    extension.mkdir()
    (extension/"pack.json").write_text(json.dumps({"api":1,"entry":"yard.lua","files":["yard.lua"]}))
    (extension/"yard.lua").write_text("""local api=...
for i=1,90 do api.job("shift-"..i,"Shift "..i,function(s) s.blue.alloy=s.blue.alloy+1 return true end) end
api.mission{id="large-yard",name="Large yard",sector="westbound",build=function(ctx)
 for i=1,100 do ctx.add_entity("spare-"..i,"dray",{i*22,0,110}) end
 return {hold_ticks=30,actions={{"base","Base"}},new=function() return {tick=0,blue={queues={},alloy=8},red={queues={}},result="active",report="Ready"} end,step=function(s) s.tick=s.tick+1 end,command=function() return false end}
end}""")
    (stress/"content.json").write_text(json.dumps(dict(base,packs=["mods/yard"])))
    call("--data",stress,"--mission","large-yard","--replay","-",route="order shift-90\nalloy 9\norder crew:spare-100@spare-99\nwait 150\ncheckpoint large")
    assert "hold=2700" in call("--data",root)
    assert "hold=27000" in call("--data",root,"--mission","tide-window")
    call("--data",root,"--replay","-",route="order mill\nalloy 4\norder cancel\nalloy 8")
