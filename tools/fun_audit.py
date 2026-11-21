"""Paired Gate Six playtest. Build fun_probe first. Requires Lua 5.4, Pillow and NumPy."""
import argparse
import csv
import json
import math
import os
import shutil
import statistics as st
import subprocess
from collections import Counter, defaultdict
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
HZ = 30

def band(value, low, high):
    if value < low:
        return value / low
    return min(1, high / value) if value else 1

def contrast(out):
    def pixels(name):
        return np.asarray(Image.open(out / (name + ".ppm")), dtype=float) / 255
    def luminance(rgb):
        return np.where(rgb <= .04045, rgb / 12.92, ((rgb + .055) / 1.055) ** 2.4) @ [.2126, .7152, .0722]
    front, back = pixels("enemy"), pixels("background")
    mask = np.abs(front-back).max(axis=2) > 15/255
    mask[:95] = False
    mask[-85:] = False
    if not mask.any():
        raise ValueError("No visible rival pixels in the contrast view")
    a, b = luminance(front[mask]), luminance(back[mask])
    ratios = (np.maximum(a,b)+.05)/(np.minimum(a,b)+.05)
    return float(np.median(ratios)), float(np.mean(ratios >= 3.5)), int(mask.sum())

def analyze(out):
    events = defaultdict(list)
    with (out/"runs.csv").open() as f:
        for seed, persona, tick, kind, *values in csv.reader(f):
            events[persona,int(seed)].append((int(tick),kind,values))
    readings = defaultdict(list)
    with (out/"render.csv").open() as f:
        for name,*vals in csv.reader(f):
            readings[name].append(vals)
    outcome, wins, stalled, seconds = defaultdict(list), Counter(), Counter(), defaultdict(list)
    choice = Counter()
    waves, fatigue, micro, meso, warning, probes = [], [], [], [], [], []
    samples = []
    for (persona,seed), run in events.items():
        if persona == "timing":
            probes.extend(int(v[2]) for _,kind,v in run if kind=="buffer")
            continue
        end,result,values = run[-1]
        assert result=="end"
        w=float(values[1])
        outcome[persona].append(w)
        wins[persona]+=values[0]=="victory"
        stalled[persona]+=values[0]=="active"
        seconds[persona].append(end/HZ)
        samples.append({"seed":seed,"persona":persona,"result":values[0],"seconds":end/HZ,"objective":w})
        if persona in ("human","oracle"):
            choice.update(v[0] for _,k,v in run if k=="input" and v[2]=="1")
        if persona=="spam":
            warned=next((t for t,k,_ in run if k=="warning"),None)
            room=next((t for t,k,_ in run if k=="enemy_room"),None)
            if room is not None:
                warning.append((room-warned)*1000/HZ if warned is not None else 0)
        if persona!="human":
            continue
        tension=[.55*(float(v[0])+float(v[1]))/2+.30*min(1,float(v[3])/4)+.15*float(v[2])
                 for _,k,v in run if k=="second"]
        mean=st.mean(tension)
        waves.append(st.pstdev(tension)/mean if mean else 0)
        high=longest=0
        for t in tension:
            high=high+1 if t>.9 else 0
            longest=max(longest,high)
        fatigue.append(longest)
        ticks=[0]+[t for t,k,_ in run if k=="reward"]+[end]
        micro.extend((b-a)/HZ for a,b in zip(ticks,ticks[1:]) if b>a)
        milestones=[0]+[t for t,k,_ in run if k=="claim"]+[end]
        meso.extend((b-a)/HZ for a,b in zip(milestones,milestones[1:]) if b>a)
    avg={p:st.mean(v) for p,v in outcome.items()}
    p95=lambda items:float(np.percentile(items,95))
    visual=[float(v[2]) for v in readings["visual"]]
    if len(visual)!=10: raise ValueError(f"Only {len(visual)} of ten visible order acknowledgments")
    motion=[abs(float(v[3])) for v in readings["motion"]]
    easing=sum(.15*max(motion)<v<.90*max(motion) for v in motion)
    feed={v[0]:sum(map(int,v[1:])) for v in readings["feedback"]}
    channels=st.mean(feed.values())
    missed=1-st.mean(probes)
    proportions=[n/choice.total() for n in choice.values()]
    entropy=-sum(p*math.log(p) for p in proportions)/math.log(10)
    dominance=max(proportions)
    spam=avg["spam"]/avg["human"] if avg["human"] else 1
    cv=st.mean(waves)
    warn=st.median(warning)
    ratio,readable,pixel_count=contrast(out)
    lifecycle={k:float(v) for k,v in readings["lifecycle"]}
    boot=lifecycle["first_interactive"]/1000
    retry=float(readings["retry"][0][0])/1000
    micro_med,meso_med=st.median(micro),st.median(meso)
    gap=avg["oracle"]-avg["novice"]
    variation=st.pstdev(outcome["human"])
    frame_p95=p95([float(v[1]) for v in readings["frame"][10:]])
    periods=[float(v[2]) for v in readings["frame"][10:]]
    pillars={
        "feel":st.mean([max(0,min(1,(67-p95(visual))/(67-33.4))),min(1,(1-missed)/.95),
                         min(1,easing/2),min(1,channels/2.5)]),
        "flow":st.mean([max(0,min(1,gap/.55)),min(1,(1-avg["novice"])/.65),min(1,avg["oracle"]/.70),
                         band(avg["human"],.40,.75),band(variation,.10,.25)]),
        "choice":st.mean([min(1,entropy/.70),min(1,.65/dominance),1 if spam<.80 else 0]),
        "pace":st.mean([band(cv,.25,.85),1 if max(fatigue)<=20 else 0]),
        "fair":st.mean([min(1,ratio/3.5),min(1,warn/300)]),
        "loop":st.mean([band(boot,0,3),band(retry,0,1),band(micro_med,1,5),band(meso_med,30,90)])}
    composite=100*pillars["feel"]**.25*pillars["flow"]**.25*pillars["fair"]**.20*(
        .40+.25*pillars["choice"]+.20*pillars["pace"]+.15*pillars["loop"])
    data=dict(pillars=pillars,composite=composite,episodes={k:len(v) for k,v in outcome.items()},
              wins=wins,unfinished=stalled,objective=avg,human_sd=variation,skill_gap=gap,
              mean_seconds={k:st.mean(v) for k,v in seconds.items()},latency_p95_ms=p95(visual),
              drop_rate=missed,distinct_timing_cases=6,feedback=feed,channels=channels,easing_ticks=easing,
              entropy=entropy,largest_share=dominance,spam_ratio=spam,usage=choice,cv=cv,overload_seconds=max(fatigue),
              near_misses_per_min=None,hurtbox_ratio=None,warning_ms=warn,contrast=ratio,contrast_pass_fraction=readable,
              foreground_pixels=pixel_count,ttfa_s=boot,retry_s=retry,reward_median_s=micro_med,
              reward_p95_s=p95(micro),milestone_median_s=meso_med,render_p95_ms=frame_p95,
              delivered_fps=1000/st.mean(periods),frame_interval_p95_ms=p95(periods),episodes_detail=samples)
    (out/"results.json").write_text(json.dumps(data,indent=2)+"\n")
    return data

def collect(build,out,episodes,seed,holdout,lua):
    out.mkdir(parents=True,exist_ok=True)
    root=build/"bin"
    with (out/"runs.csv").open("w") as rows:
        for start in (seed,holdout):
            args=[lua,str(ROOT/"tests/fun_personas.lua"),str(root),str(episodes),str(start)]
            if start==seed: args.append(str(out/"ticks.csv"))
            subprocess.run(args,stdout=rows,check=True)
    with (out/"render.csv").open("w") as frames:
        env=dict(os.environ,SDL_VIDEODRIVER="offscreen",SDL_AUDIODRIVER="dummy",LIBGL_ALWAYS_SOFTWARE="1",
                 MESA_SHADER_CACHE_DISABLE="true")
        subprocess.run([str(build/"fun_probe"),str(root),str(out)],check=True,env=env,stdout=frames)
    for name in ("boot","peak","game-over"):
        with Image.open(out/(name+".ppm")) as im: im.save(out/(name+".png"),optimize=True)

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build",type=Path)
    parser.add_argument("out",type=Path)
    parser.add_argument("--episodes",type=int,default=20,help="per persona per seed block")
    parser.add_argument("--seed",type=int,default=42)
    parser.add_argument("--holdout",type=int,default=1042)
    parser.add_argument("--lua",default=shutil.which("lua5.4") or "lua")
    parser.add_argument("--analyze-only",action="store_true")
    args=parser.parse_args()
    if args.episodes<1 or abs(args.seed-args.holdout)<args.episodes:
        parser.error("Use positive episode counts and disjoint seed blocks")
    if not args.analyze_only:
        collect(args.build.resolve(),args.out.resolve(),args.episodes,args.seed,args.holdout,args.lua)
    result=analyze(args.out)
    print(json.dumps({k:v for k,v in result.items() if k!="episodes_detail"},indent=2))
